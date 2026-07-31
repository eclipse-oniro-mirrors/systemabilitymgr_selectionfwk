/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// 包含事件总线、对象池、速率限制器、LRU缓存、状态机、熔断器等工具类

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <set>
#include <string>
#include <thread>
#include <tuple>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "selection_log.h"
#include "selection_errors.h"

namespace OHOS {
namespace SelectionFwk {

// ============================================================================
// SelectionEventBus — 线程安全的事件总线，支持类型分发和优先级订阅
// ============================================================================

enum class SelectionEventPriority : int32_t {
    LOWEST = 0,
    LOW = 25,
    NORMAL = 50,
    HIGH = 75,
    HIGHEST = 100,
};

struct SelectionSubscriptionHandle {
    uint64_t id = 0;
    std::type_index eventType;
    SelectionSubscriptionHandle() : eventType(typeid(void)) {}
    SelectionSubscriptionHandle(uint64_t idVal, std::type_index type) : id(idVal), eventType(type) {}
    bool IsValid() const { return id != 0; }
};

class ISelectionEventHandler {
public:
    virtual ~ISelectionEventHandler() = default;
    virtual std::type_index GetEventType() const = 0;
    virtual void Invoke(void* event) = 0;
    virtual SelectionEventPriority GetPriority() const = 0;
    virtual uint64_t GetId() const = 0;
};

template<typename EventT>
class SelectionEventHandler final : public ISelectionEventHandler {
public:
    using Callback = std::function<void(const EventT&)>;
    SelectionEventHandler(uint64_t id, Callback cb, SelectionEventPriority pri)
        : id_(id), callback_(std::move(cb)), priority_(pri) {}
    std::type_index GetEventType() const override { return std::type_index(typeid(EventT)); }
    void Invoke(void* event) override
    {
        if (event != nullptr && callback_) {
            callback_(*static_cast<EventT*>(event));
        }
    }
    SelectionEventPriority GetPriority() const override { return priority_; }
    uint64_t GetId() const override { return id_; }
private:
    uint64_t id_;
    Callback callback_;
    SelectionEventPriority priority_;
};

class SelectionEventBus {
public:
    static SelectionEventBus& GetInstance()
    {
        static SelectionEventBus instance;
        return instance;
    }

    template<typename EventT>
    SelectionSubscriptionHandle Subscribe(
        typename SelectionEventHandler<EventT>::Callback callback,
        SelectionEventPriority priority = SelectionEventPriority::NORMAL)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        uint64_t id = ++nextHandleId_;
        auto handler = std::make_shared<SelectionEventHandler<EventT>>(id, std::move(callback), priority);
        std::type_index typeIdx(typeid(EventT));
        handlers_[typeIdx].push_back(std::move(handler));
        SortHandlers(typeIdx);
        return SelectionSubscriptionHandle(id, typeIdx);
    }

    template<typename EventT>
    bool Unsubscribe(const SelectionSubscriptionHandle& handle)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        std::type_index typeIdx(typeid(EventT));
        auto it = handlers_.find(typeIdx);
        if (it == handlers_.end()) {
            return false;
        }
        auto& list = it->second;
        auto hIt = std::find_if(list.begin(), list.end(),
            [&handle](const auto& h) { return h->GetId() == handle.id; });
        if (hIt == list.end()) {
            return false;
        }
        list.erase(hIt);
        if (list.empty()) {
            handlers_.erase(it);
        }
        return true;
    }

    template<typename EventT>
    void Publish(const EventT& event)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        auto it = handlers_.find(std::type_index(typeid(EventT)));
        if (it == handlers_.end()) {
            return;
        }
        for (const auto& handler : it->second) {
            handler->Invoke(const_cast<void*>(static_cast<const void*>(&event)));
        }
    }

    void ClearAll()
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        handlers_.clear();
    }

private:
    SelectionEventBus() = default;
    SelectionEventBus(const SelectionEventBus&) = delete;
    SelectionEventBus& operator=(const SelectionEventBus&) = delete;

    void SortHandlers(std::type_index typeIdx)
    {
        auto it = handlers_.find(typeIdx);
        if (it == handlers_.end()) {
            return;
        }
        auto& list = it->second;
        std::stable_sort(list.begin(), list.end(),
            [](const auto& a, const auto& b) {
                return static_cast<int32_t>(a->GetPriority()) > static_cast<int32_t>(b->GetPriority());
            });
    }

    mutable std::recursive_mutex mutex_;
    std::unordered_map<std::type_index, std::vector<std::shared_ptr<ISelectionEventHandler>>> handlers_;
    std::atomic<uint64_t> nextHandleId_ {0};
};

// ============================================================================
// SelectionObjectPool — 通用对象池，支持预分配与自动回收
// ============================================================================

template<typename T>
class SelectionObjectPool {
public:
    using Factory = std::function<std::shared_ptr<T>()>;
    using Resetter = std::function<void(std::shared_ptr<T>&)>;

    explicit SelectionObjectPool(Factory factory, Resetter resetter = nullptr,
        size_t maxIdleSize = 16, size_t preAllocSize = 0)
        : factory_(std::move(factory)), resetter_(std::move(resetter)), maxIdleSize_(maxIdleSize)
    {
        for (size_t i = 0; i < preAllocSize; ++i) {
            auto obj = factory_();
            if (obj) {
                idlePool_.push_back(std::move(obj));
            }
        }
    }

    ~SelectionObjectPool()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        idlePool_.clear();
        activeObjs_.clear();
    }

    std::shared_ptr<T> Acquire()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::shared_ptr<T> obj;
        if (!idlePool_.empty()) {
            obj = idlePool_.back();
            idlePool_.pop_back();
        } else {
            obj = factory_();
            if (!obj) {
                return nullptr;
            }
        }
        activeObjs_.insert(obj.get());
        return std::shared_ptr<T>(obj.get(), [this, obj](T*) { Release(obj); });
    }

    size_t IdleCount() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return idlePool_.size();
    }

    size_t ActiveCount() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return activeObjs_.size();
    }

    void ShrinkIdlePool(size_t targetSize = 0)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        while (idlePool_.size() > targetSize) {
            idlePool_.pop_back();
        }
    }

private:
    void Release(std::shared_ptr<T> obj)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        activeObjs_.erase(obj.get());
        if (resetter_) {
            resetter_(obj);
        }
        if (idlePool_.size() < maxIdleSize_) {
            idlePool_.push_back(std::move(obj));
        }
    }

    Factory factory_;
    Resetter resetter_;
    size_t maxIdleSize_;
    std::vector<std::shared_ptr<T>> idlePool_;
    std::unordered_set<T*> activeObjs_;
    mutable std::mutex mutex_;
};

// ============================================================================
// SelectionRateLimiter — 速率限制器（滑动窗口 + 令牌桶）
// ============================================================================

enum class SelectionRateLimitStrategy { SLIDING_WINDOW, TOKEN_BUCKET };

class SelectionRateLimiter {
public:
    static std::unique_ptr<SelectionRateLimiter> Create(
        SelectionRateLimitStrategy strategy, uint32_t maxRequests, uint32_t windowMs);
    virtual ~SelectionRateLimiter() = default;
    virtual bool Allow() = 0;
    virtual void Reset() = 0;
    virtual uint32_t GetRemainingQuota() const = 0;
};

class SelectionSlidingWindowLimiter : public SelectionRateLimiter {
public:
    SelectionSlidingWindowLimiter(uint32_t maxRequests, uint32_t windowMs)
        : maxRequests_(maxRequests), windowMs_(windowMs) {}

    bool Allow() override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        int64_t windowStart = nowMs - static_cast<int64_t>(windowMs_);
        while (!timestamps_.empty() && timestamps_.front() <= windowStart) {
            timestamps_.pop_front();
        }
        if (timestamps_.size() < maxRequests_) {
            timestamps_.push_back(nowMs);
            return true;
        }
        return false;
    }

    void Reset() override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        timestamps_.clear();
    }

    uint32_t GetRemainingQuota() const override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        size_t active = 0;
        for (auto ts : timestamps_) {
            if (ts > nowMs - static_cast<int64_t>(windowMs_)) {
                active++;
            }
        }
        return static_cast<uint32_t>(maxRequests_ > active ? maxRequests_ - active : 0);
    }

private:
    uint32_t maxRequests_;
    uint32_t windowMs_;
    std::deque<int64_t> timestamps_;
    mutable std::mutex mutex_;
};

class SelectionTokenBucketLimiter : public SelectionRateLimiter {
public:
    SelectionTokenBucketLimiter(uint32_t maxTokens, uint32_t refillMs)
        : maxTokens_(maxTokens), refillMs_(refillMs), tokens_(static_cast<double>(maxTokens)),
          lastRefillTime_(std::chrono::steady_clock::now()) {}

    bool Allow() override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        Refill();
        static constexpr double singleTokenCost = 1.0;
        if (tokens_ >= singleTokenCost) {
            tokens_ -= singleTokenCost;
            return true;
        }
        return false;
    }

    void Reset() override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tokens_ = static_cast<double>(maxTokens_);
        lastRefillTime_ = std::chrono::steady_clock::now();
    }

    uint32_t GetRemainingQuota() const override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return static_cast<uint32_t>(tokens_);
    }

private:
    void Refill()
    {
        auto now = std::chrono::steady_clock::now();
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRefillTime_).count();
        tokens_ = std::min(static_cast<double>(maxTokens_),
            tokens_ + static_cast<double>(elapsedMs) / static_cast<double>(refillMs_));
        lastRefillTime_ = now;
    }

    uint32_t maxTokens_;
    uint32_t refillMs_;
    double tokens_;
    std::chrono::steady_clock::time_point lastRefillTime_;
    mutable std::mutex mutex_;
};

std::unique_ptr<SelectionRateLimiter> SelectionRateLimiter::Create(
    SelectionRateLimitStrategy strategy, uint32_t maxRequests, uint32_t windowMs)
{
    switch (strategy) {
        case SelectionRateLimitStrategy::SLIDING_WINDOW:
            return std::make_unique<SelectionSlidingWindowLimiter>(maxRequests, windowMs);
        case SelectionRateLimitStrategy::TOKEN_BUCKET:
            return std::make_unique<SelectionTokenBucketLimiter>(maxRequests, windowMs);
        default:
            return nullptr;
    }
}

// ============================================================================
// SelectionLruCache — 线程安全的LRU缓存
// ============================================================================

template<typename Key, typename Value>
class SelectionLruCache {
public:
    static constexpr size_t defaultCacheCapacity_ = 64;
    explicit SelectionLruCache(size_t capacity) : capacity_(capacity == 0 ? defaultCacheCapacity_ : capacity) {}

    void Put(const Key& key, const Value& value)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = indexMap_.find(key);
        if (it != indexMap_.end()) {
            it->second->second = value;
            items_.splice(items_.begin(), items_, it->second);
            return;
        }
        if (items_.size() >= capacity_) {
            indexMap_.erase(items_.back().first);
            items_.pop_back();
            evictCount_++;
        }
        items_.push_front({key, value});
        indexMap_[key] = items_.begin();
    }

    std::optional<Value> Get(const Key& key)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = indexMap_.find(key);
        if (it == indexMap_.end()) {
            missCount_++;
            return std::nullopt;
        }
        items_.splice(items_.begin(), items_, it->second);
        hitCount_++;
        return it->second->second;
    }

    bool Erase(const Key& key)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = indexMap_.find(key);
        if (it == indexMap_.end()) {
            return false;
        }
        items_.erase(it->second);
        indexMap_.erase(it);
        return true;
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        items_.clear();
        indexMap_.clear();
    }

    size_t Size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return items_.size();
    }

    double HitRate() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        uint64_t total = hitCount_ + missCount_;
        return total == 0 ? 0.0 : static_cast<double>(hitCount_) / static_cast<double>(total);
    }

private:
    size_t capacity_;
    std::list<std::pair<Key, Value>> items_;
    std::unordered_map<Key, typename std::list<std::pair<Key, Value>>::iterator> indexMap_;
    mutable std::mutex mutex_;
    mutable uint64_t hitCount_ = 0;
    mutable uint64_t missCount_ = 0;
    uint64_t evictCount_ = 0;
};

// ============================================================================
// SelectionStateMachine — 通用有限状态机，支持守卫条件和动作
// ============================================================================

template<typename State, typename Event>
class SelectionStateMachine {
public:
    using Guard = std::function<bool()>;
    using Action = std::function<void()>;
    struct Transition { State from; Event trigger; State to; Guard guard; Action action; };

    explicit SelectionStateMachine(State initial) : currentState_(initial) {}

    void AddTransition(State from, Event trigger, State to,
        Guard guard = nullptr, Action action = nullptr)
    {
        transitions_.push_back({from, trigger, to, guard, action});
    }

    std::optional<State> HandleEvent(Event event)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& t : transitions_) {
            if (t.from == currentState_ && t.trigger == event) {
                if (t.guard && !t.guard()) {
                    continue;
                }
                State prev = currentState_;
                currentState_ = t.to;
                if (t.action) {
                    t.action();
                }
                history_.push_back({prev, event, currentState_});
                if (history_.size() > maxHistorySize_) {
                    history_.pop_front();
                }
                return currentState_;
            }
        }
        return std::nullopt;
    }

    State GetCurrentState() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return currentState_;
    }

    void ForceState(State state)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        currentState_ = state;
    }

    std::vector<std::tuple<State, Event, State>> GetHistory() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return {history_.begin(), history_.end()};
    }

private:
    State currentState_;
    std::vector<Transition> transitions_;
    std::deque<std::tuple<State, Event, State>> history_;
    static constexpr size_t defaultMaxHistorySize_ = 128;
    size_t maxHistorySize_ = defaultMaxHistorySize_;
    mutable std::mutex mutex_;
};

// ============================================================================
// SelectionCircuitBreaker — 熔断器，防止级联故障
// ============================================================================

enum class SelectionCircuitState { CLOSED, OPEN, HALF_OPEN };

class SelectionCircuitBreaker {
public:
    struct Config {
        static constexpr uint32_t defaultFailureThreshold = 5;
        static constexpr uint32_t defaultResetTimeoutMs = 30000;
        static constexpr uint32_t defaultHalfOpenMaxRequests = 1;
        static constexpr uint32_t defaultSuccessThreshold = 3;
        uint32_t failureThreshold = defaultFailureThreshold;
        uint32_t resetTimeoutMs = defaultResetTimeoutMs;
        uint32_t halfOpenMaxRequests = defaultHalfOpenMaxRequests;
        uint32_t successThreshold = defaultSuccessThreshold;
    };

    explicit SelectionCircuitBreaker(const Config& cfg) : config_(cfg) {}

    bool AllowRequest()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        switch (state_) {
            case SelectionCircuitState::CLOSED:
                return true;
            case SelectionCircuitState::OPEN:
                if (HasResetTimeoutExpired()) {
                    state_ = SelectionCircuitState::HALF_OPEN;
                    halfOpenSuccessCount_ = 0;
                    halfOpenRequestCount_ = 0;
                    return true;
                }
                return false;
            case SelectionCircuitState::HALF_OPEN:
                if (halfOpenRequestCount_ < config_.halfOpenMaxRequests) {
                    halfOpenRequestCount_++;
                    return true;
                }
                return false;
            default:
                return false;
        }
    }

    void RecordSuccess()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        consecutiveFailures_ = 0;
        if (state_ == SelectionCircuitState::HALF_OPEN) {
            halfOpenSuccessCount_++;
            if (halfOpenSuccessCount_ >= config_.successThreshold) {
                state_ = SelectionCircuitState::CLOSED;
                failureCount_ = 0;
            }
        }
    }

    void RecordFailure()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        consecutiveFailures_++;
        failureCount_++;
        if (state_ == SelectionCircuitState::HALF_OPEN) {
            TripToOpen();
        } else if (state_ == SelectionCircuitState::CLOSED &&
                   consecutiveFailures_ >= config_.failureThreshold) {
            TripToOpen();
        }
    }

    SelectionCircuitState GetState() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }

    void Reset()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = SelectionCircuitState::CLOSED;
        failureCount_ = 0;
        consecutiveFailures_ = 0;
    }

private:
    void TripToOpen()
    {
        state_ = SelectionCircuitState::OPEN;
        openedAt_ = std::chrono::steady_clock::now();
    }

    bool HasResetTimeoutExpired() const
    {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - openedAt_).count();
        return static_cast<uint32_t>(elapsed) >= config_.resetTimeoutMs;
    }

    Config config_;
    SelectionCircuitState state_ = SelectionCircuitState::CLOSED;
    uint32_t failureCount_ = 0;
    uint32_t consecutiveFailures_ = 0;
    uint32_t halfOpenSuccessCount_ = 0;
    uint32_t halfOpenRequestCount_ = 0;
    std::chrono::steady_clock::time_point openedAt_;
    mutable std::mutex mutex_;
};

// ============================================================================
// SelectionRetryPolicy — 重试策略，支持指数退避和抖动
// ============================================================================

class SelectionRetryPolicy {
public:
    struct Config {
        static constexpr uint32_t defaultMaxRetries = 3;
        static constexpr uint32_t defaultInitialDelayMs = 100;
        static constexpr uint32_t defaultMaxDelayMs = 5000;
        static constexpr double defaultBackoffMultiplier = 2.0;
        uint32_t maxRetries = defaultMaxRetries;
        uint32_t initialDelayMs = defaultInitialDelayMs;
        uint32_t maxDelayMs = defaultMaxDelayMs;
        double backoffMultiplier = defaultBackoffMultiplier;
    };

    explicit SelectionRetryPolicy(const Config& cfg) : config_(cfg) {}

    uint32_t GetDelayForAttempt(uint32_t attempt) const
    {
        if (attempt == 0) {
            return 0;
        }
        double delay = static_cast<double>(config_.initialDelayMs);
        for (uint32_t i = 1; i < attempt; ++i) {
            delay *= config_.backoffMultiplier;
        }
        return static_cast<uint32_t>(std::min(delay, static_cast<double>(config_.maxDelayMs)));
    }

    bool ShouldRetry(uint32_t attempt) const
    {
        return attempt < config_.maxRetries;
    }

private:
    Config config_;
};

// ============================================================================
// SelectionContentTrimmer — 选中文本裁剪器，处理字节边界和UTF-8截断
// ============================================================================

class SelectionContentTrimmer {
public:
    static constexpr uint32_t maxSelectionBytes_ = 6000;

    static std::string TrimToLimit(const std::string& content, uint32_t maxBytes = maxSelectionBytes_)
    {
        if (content.size() <= maxBytes) {
            return content;
        }
        return content.substr(0, FindSafeUtf8Boundary(content, maxBytes));
    }

    static bool IsWithinLimit(const std::string& content, uint32_t maxBytes = maxSelectionBytes_)
    {
        return content.size() <= maxBytes;
    }

    static uint32_t FindSafeUtf8Boundary(const std::string& content, uint32_t maxBytes)
    {
        if (content.empty() || maxBytes == 0) {
            return 0;
        }
        uint32_t len = static_cast<uint32_t>(content.size());
        if (maxBytes >= len) {
            return len;
        }
        uint32_t pos = maxBytes;
        while (pos > 0 && IsUtf8ContinuationByte(static_cast<uint8_t>(content[pos]))) {
            pos--;
        }
        if (pos == 0) {
            return 0;
        }
        uint32_t expectedLen = Utf8CharExpectedLen(static_cast<uint8_t>(content[pos]));
        return (pos + expectedLen > maxBytes) ? pos : maxBytes;
    }

    static constexpr uint8_t printableCharMin_ = 0x20;

    static std::string SanitizeContent(const std::string& content)
    {
        std::string result;
        result.reserve(content.size());
        for (unsigned char c : content) {
            if (c >= printableCharMin_ || c == '\t' || c == '\n' || c == '\r') {
                result.push_back(static_cast<char>(c));
            }
        }
        return result;
    }

private:
    static constexpr uint32_t utf8OneByteMask_ = 0x80;
    static constexpr uint32_t utf8OneByteVal_ = 0x00;
    static constexpr uint32_t utf8TwoByteMask_ = 0xE0;
    static constexpr uint32_t utf8TwoByteVal_ = 0xC0;
    static constexpr uint32_t utf8ThreeByteMask_ = 0xF0;
    static constexpr uint32_t utf8ThreeByteVal_ = 0xE0;
    static constexpr uint32_t utf8FourByteMask_ = 0xF8;
    static constexpr uint32_t utf8FourByteVal_ = 0xF0;
    static constexpr uint32_t utf8OneByteLen_ = 1;
    static constexpr uint32_t utf8TwoByteLen_ = 2;
    static constexpr uint32_t utf8ThreeByteLen_ = 3;
    static constexpr uint32_t utf8FourByteLen_ = 4;
    static constexpr uint8_t utf8ContinuationMask_ = 0xC0;
    static constexpr uint8_t utf8ContinuationVal_ = 0x80;

    static bool IsUtf8ContinuationByte(uint8_t byte)
    {
        return (byte & utf8ContinuationMask_) == utf8ContinuationVal_;
    }
    static uint32_t Utf8CharExpectedLen(uint8_t first)
    {
        if ((first & utf8OneByteMask_) == utf8OneByteVal_) {
            return utf8OneByteLen_;
        }
        if ((first & utf8TwoByteMask_) == utf8TwoByteVal_) {
            return utf8TwoByteLen_;
        }
        if ((first & utf8ThreeByteMask_) == utf8ThreeByteVal_) {
            return utf8ThreeByteLen_;
        }
        if ((first & utf8FourByteMask_) == utf8FourByteVal_) {
            return utf8FourByteLen_;
        }
        return utf8OneByteLen_;
    }
};

// ============================================================================
// SelectionMetricsCollector — 指标收集器 (Counter/Gauge/Histogram)
// ============================================================================

class SelectionMetricsCollector {
public:
    enum class MetricType { COUNTER, GAUGE, HISTOGRAM };

    struct MetricEntry {
        std::string name;
        MetricType type;
        int64_t intValue = 0;
        double doubleValue = 0.0;
        std::vector<int64_t> samples;
    };

    static SelectionMetricsCollector& GetInstance()
    {
        static SelectionMetricsCollector instance;
        return instance;
    }

    void RegisterCounter(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (metrics_.find(name) == metrics_.end()) {
            metrics_[name] = {name, MetricType::COUNTER};
        }
    }

    void RegisterGauge(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (metrics_.find(name) == metrics_.end()) {
            metrics_[name] = {name, MetricType::GAUGE};
        }
    }

    void RegisterHistogram(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (metrics_.find(name) == metrics_.end()) {
            metrics_[name] = {name, MetricType::HISTOGRAM};
        }
    }

    void IncrementCounter(const std::string& name, int64_t delta = 1)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = metrics_.find(name);
        if (it != metrics_.end() && it->second.type == MetricType::COUNTER) {
            it->second.intValue += delta;
        }
    }

    void SetGauge(const std::string& name, double value)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = metrics_.find(name);
        if (it != metrics_.end() && it->second.type == MetricType::GAUGE) {
            it->second.doubleValue = value;
        }
    }

    void RecordHistogramSample(const std::string& name, int64_t sample)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = metrics_.find(name);
        if (it != metrics_.end() && it->second.type == MetricType::HISTOGRAM) {
            it->second.samples.push_back(sample);
            if (it->second.samples.size() > maxHistogramSamples_) {
                it->second.samples.erase(it->second.samples.begin());
            }
        }
    }

    std::optional<int64_t> GetCounterValue(const std::string& name) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = metrics_.find(name);
        if (it != metrics_.end() && it->second.type == MetricType::COUNTER) {
            return it->second.intValue;
        }
        return std::nullopt;
    }

    struct HistogramStats {
        int64_t min = 0;
        int64_t max = 0;
        double mean = 0.0;
        double p50 = 0.0;
        double p95 = 0.0;
        double p99 = 0.0;
    };

    std::optional<HistogramStats> GetHistogramStats(const std::string& name) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = metrics_.find(name);
        if (it == metrics_.end() || it->second.type != MetricType::HISTOGRAM || it->second.samples.empty()) {
            return std::nullopt;
        }
        auto sorted = it->second.samples;
        std::sort(sorted.begin(), sorted.end());
        HistogramStats stats;
        stats.min = sorted.front();
        stats.max = sorted.back();
        int64_t sum = 0;
        for (auto s : sorted) {
            sum += s;
        }
        stats.mean = static_cast<double>(sum) / static_cast<double>(sorted.size());
        stats.p50 = Percentile(sorted, percentileP50);
        stats.p95 = Percentile(sorted, percentileP95);
        stats.p99 = Percentile(sorted, percentileP99);
        return stats;
    }

    void ResetAll()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [name, entry] : metrics_) {
            entry.intValue = 0;
            entry.doubleValue = 0.0;
            entry.samples.clear();
        }
    }

private:
    SelectionMetricsCollector() = default;
    SelectionMetricsCollector(const SelectionMetricsCollector&) = delete;
    SelectionMetricsCollector& operator=(const SelectionMetricsCollector&) = delete;

    static double Percentile(const std::vector<int64_t>& sorted, double p)
    {
        if (sorted.empty()) {
            return 0.0;
        }
        double idx = p * static_cast<double>(sorted.size() - 1);
        size_t lo = static_cast<size_t>(std::floor(idx));
        size_t hi = static_cast<size_t>(std::ceil(idx));
        if (lo == hi || hi >= sorted.size()) {
            return static_cast<double>(sorted[lo]);
        }
        double frac = idx - static_cast<double>(lo);
        return static_cast<double>(sorted[lo]) * (1.0 - frac) + static_cast<double>(sorted[hi]) * frac;
    }

    static constexpr size_t maxHistogramSamples_ = 1024;
    static constexpr double percentileP50 = 0.50;
    static constexpr double percentileP95 = 0.95;
    static constexpr double percentileP99 = 0.99;
    std::unordered_map<std::string, MetricEntry> metrics_;
    mutable std::mutex mutex_;
};

// ============================================================================
// SelectionScopedTimer — RAII 作用域计时器
// ============================================================================

class SelectionScopedTimer {
public:
    explicit SelectionScopedTimer(const std::string& tag)
        : tag_(tag), startTime_(std::chrono::steady_clock::now()) {}

    SelectionScopedTimer(const std::string& tag, std::function<void(const std::string&, uint64_t)> cb)
        : tag_(tag), callback_(std::move(cb)), startTime_(std::chrono::steady_clock::now()) {}

    ~SelectionScopedTimer()
    {
        auto ms = ElapsedMs();
        if (callback_) {
            callback_(tag_, ms);
        } else {
            SELECTION_HILOGI("ScopedTimer [%{public}s]: %{public}" PRIu64 " ms", tag_.c_str(), ms);
        }
    }

    uint64_t ElapsedMs() const
    {
        return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime_).count());
    }

    SelectionScopedTimer(const SelectionScopedTimer&) = delete;
    SelectionScopedTimer& operator=(const SelectionScopedTimer&) = delete;

private:
    std::string tag_;
    std::function<void(const std::string&, uint64_t)> callback_;
    std::chrono::steady_clock::time_point startTime_;
};

// ============================================================================
// SelectionDeferredExecutor — 延迟执行器（defer 模式）
// ============================================================================

class SelectionDeferredExecutor {
public:
    SelectionDeferredExecutor() = default;
    ~SelectionDeferredExecutor()
    {
        for (auto it = actions_.rbegin(); it != actions_.rend(); ++it) {
            if (*it) {
                (*it)();
            }
        }
    }
    void Defer(std::function<void()> action)
    {
        if (action) {
            actions_.push_back(std::move(action));
        }
    }
    void CancelAll()
    {
        actions_.clear();
    }
    SelectionDeferredExecutor(const SelectionDeferredExecutor&) = delete;
    SelectionDeferredExecutor& operator=(const SelectionDeferredExecutor&) = delete;
private:
    std::vector<std::function<void()>> actions_;
};

// ============================================================================
// SelectionPluginRegistry — 插件注册表，管理动态加载的插件信息
// ============================================================================

struct SelectionPluginInfo {
    std::string name;
    std::string soPath;
    uint32_t version = 0;
    bool isLoaded = false;
    void* handle = nullptr;
};

class SelectionPluginRegistry {
public:
    static SelectionPluginRegistry& GetInstance()
    {
        static SelectionPluginRegistry instance;
        return instance;
    }

    bool RegisterPlugin(const std::string& name, const std::string& soPath, uint32_t version)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (plugins_.count(name) > 0) {
            return false;
        }
        SelectionPluginInfo info{name, soPath, version, false, nullptr};
        plugins_[name] = std::move(info);
        return true;
    }

    bool UnregisterPlugin(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = plugins_.find(name);
        if (it == plugins_.end() || it->second.isLoaded) {
            return false;
        }
        plugins_.erase(it);
        return true;
    }

    std::optional<SelectionPluginInfo> GetPluginInfo(const std::string& name) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = plugins_.find(name);
        return it != plugins_.end() ? std::optional{it->second} : std::nullopt;
    }

    bool MarkLoaded(const std::string& name, void* handle)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = plugins_.find(name);
        if (it == plugins_.end()) {
            return false;
        }
        it->second.isLoaded = true;
        it->second.handle = handle;
        return true;
    }

    bool MarkUnloaded(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = plugins_.find(name);
        if (it == plugins_.end()) {
            return false;
        }
        it->second.isLoaded = false;
        it->second.handle = nullptr;
        return true;
    }

    std::vector<SelectionPluginInfo> GetAllPlugins() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<SelectionPluginInfo> result;
        for (const auto& [n, info] : plugins_) {
            result.push_back(info);
        }
        return result;
    }

    void ClearAll()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        plugins_.clear();
    }

private:
    SelectionPluginRegistry() = default;
    SelectionPluginRegistry(const SelectionPluginRegistry&) = delete;
    SelectionPluginRegistry& operator=(const SelectionPluginRegistry&) = delete;

    std::unordered_map<std::string, SelectionPluginInfo> plugins_;
    mutable std::mutex mutex_;
};

// ============================================================================
// SelectionUserIdMapper — 用户ID映射表，支持多用户场景
// ============================================================================

class SelectionUserIdMapper {
public:
    static SelectionUserIdMapper& GetInstance()
    {
        static SelectionUserIdMapper instance;
        return instance;
    }

    void MapUidToUserId(int32_t uid, int32_t userId)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        uidToUserId_[uid] = userId;
        userIdToUids_[userId].insert(uid);
    }

    void UnmapUid(int32_t uid)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = uidToUserId_.find(uid);
        if (it == uidToUserId_.end()) {
            return;
        }
        int32_t userId = it->second;
        uidToUserId_.erase(it);
        auto uit = userIdToUids_.find(userId);
        if (uit != userIdToUids_.end()) {
            uit->second.erase(uid);
            if (uit->second.empty()) {
                userIdToUids_.erase(uit);
            }
        }
    }

    std::optional<int32_t> GetUserId(int32_t uid) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = uidToUserId_.find(uid);
        return it != uidToUserId_.end() ? std::optional{it->second} : std::nullopt;
    }

    std::set<int32_t> GetUidsForUser(int32_t userId) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = userIdToUids_.find(userId);
        return it != userIdToUids_.end() ? it->second : std::set<int32_t>{};
    }

    void RemoveUser(int32_t userId)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = userIdToUids_.find(userId);
        if (it == userIdToUids_.end()) {
            return;
        }
        for (auto uid : it->second) {
            uidToUserId_.erase(uid);
        }
        userIdToUids_.erase(it);
    }

    void ClearAll()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        uidToUserId_.clear();
        userIdToUids_.clear();
    }

private:
    SelectionUserIdMapper() = default;
    SelectionUserIdMapper(const SelectionUserIdMapper&) = delete;
    SelectionUserIdMapper& operator=(const SelectionUserIdMapper&) = delete;

    std::unordered_map<int32_t, int32_t> uidToUserId_;
    std::unordered_map<int32_t, std::set<int32_t>> userIdToUids_;
    mutable std::mutex mutex_;
};

// ============================================================================
// SelectionExtensionPanelRegistry — 选区扩展面板注册表
// ============================================================================

struct SelectionPanelCapability {
    static constexpr uint32_t defaultMaxContentSize = 6000;
    bool supportsTextSelection = false;
    bool supportsImageSelection = false;
    uint32_t maxContentSize = defaultMaxContentSize;
};

struct SelectionPanelEntry {
    std::string bundleName;
    std::string abilityName;
    int32_t userId = -1;
    SelectionPanelCapability capability;
    bool isActive = false;
};

class SelectionExtensionPanelRegistry {
public:
    static SelectionExtensionPanelRegistry& GetInstance()
    {
        static SelectionExtensionPanelRegistry instance;
        return instance;
    }

    bool RegisterPanel(const std::string& bundleName, const std::string& abilityName,
        int32_t userId, const SelectionPanelCapability& cap)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string key = MakeKey(bundleName, abilityName, userId);
        if (panels_.count(key) > 0) {
            return false;
        }
        SelectionPanelEntry entry{bundleName, abilityName, userId, cap, true};
        panels_[key] = std::move(entry);
        return true;
    }

    bool UnregisterPanel(const std::string& bundleName, const std::string& abilityName, int32_t userId)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return panels_.erase(MakeKey(bundleName, abilityName, userId)) > 0;
    }

    std::optional<SelectionPanelEntry> GetPanel(
        const std::string& bundleName, const std::string& abilityName, int32_t userId) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = panels_.find(MakeKey(bundleName, abilityName, userId));
        return it != panels_.end() ? std::optional{it->second} : std::nullopt;
    }

    std::vector<SelectionPanelEntry> GetPanelsForUser(int32_t userId) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<SelectionPanelEntry> result;
        for (const auto& [k, v] : panels_) {
            if (v.userId == userId) {
                result.push_back(v);
            }
        }
        return result;
    }

    bool SetPanelActive(const std::string& bundleName, const std::string& abilityName,
        int32_t userId, bool active)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = panels_.find(MakeKey(bundleName, abilityName, userId));
        if (it == panels_.end()) {
            return false;
        }
        it->second.isActive = active;
        return true;
    }

    void ClearAll()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        panels_.clear();
    }

private:
    SelectionExtensionPanelRegistry() = default;
    SelectionExtensionPanelRegistry(const SelectionExtensionPanelRegistry&) = delete;
    SelectionExtensionPanelRegistry& operator=(const SelectionExtensionPanelRegistry&) = delete;

    static std::string MakeKey(const std::string& bn, const std::string& an, int32_t uid)
    {
        return bn + "/" + an + "/" + std::to_string(uid);
    }

    std::unordered_map<std::string, SelectionPanelEntry> panels_;
    mutable std::mutex mutex_;
};
} // namespace SelectionFwk
} // namespace OHOS