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

// 选区事件限流器：对高频输入事件（选区触发、手势识别等）做节流防抖。
// 本文件当前为预留实现，未接入 service 的 BUILD.gn，待统一替换散落的散点节流后启用。

#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "selection_errors.h"
#include "selection_log.h"

namespace OHOS {
namespace SelectionFwk {

namespace {
// 默认限流参数
constexpr uint32_t DEFAULT_WINDOW_MS = 1000;        // 默认窗口 1s
constexpr uint32_t DEFAULT_CAPACITY = 10;           // 默认桶容量 / 窗口内最大通过次数
constexpr uint32_t DEFAULT_REFILL_PER_SEC = 10;     // 默认每秒补充 10 个令牌
constexpr uint64_t MS_PER_SECOND = 1000;
constexpr uint32_t MAX_KEY_LEN = 128;               // key 长度上限，防止恶意长 key
constexpr size_t MAX_CONTEXT_COUNT = 256;          // 同时维护的限流上下文数量上限
constexpr uint32_t DEFAULT_DEBOUNCE_MS = 200;       // 默认防抖间隔 200ms
constexpr double TOKEN_COST_PER_ACQUIRE = 1.0;      // 单次获取消耗的令牌数
constexpr uint64_t TIMESTAMP_UNSET = 0;             // 时间戳未初始化哨兵值
constexpr uint32_t CONFIG_DISABLED = 0;             // 配置项禁用哨兵（0 表示无效 / 不启用）
constexpr uint32_t WINDOW_COUNT_RESET = 0;          // 固定窗口计数重置值
constexpr uint32_t QUOTA_NONE = 0;                   // 无剩余配额
} // namespace

// 限流策略
enum class LimitStrategy {
    SLIDING_WINDOW = 0,  // 滑动窗口：精确但占用随事件数线性增长
    TOKEN_BUCKET = 1,    // 令牌桶：允许突发，匀速补充
    FIXED_WINDOW = 2,    // 固定窗口：实现最简，存在窗口边界突发
};

// 限流配置
struct RateLimitConfig {
    uint32_t windowMs = DEFAULT_WINDOW_MS;
    uint32_t capacity = DEFAULT_CAPACITY;
    uint32_t refillPerSec = DEFAULT_REFILL_PER_SEC;
    LimitStrategy strategy = LimitStrategy::SLIDING_WINDOW;
};

// 限流统计，便于 DFX 上报
struct RateLimitStat {
    uint64_t totalAcquired = 0;     // 累计放行次数
    uint64_t totalRejected = 0;     // 累计拒绝次数
    uint64_t lastAcquireMs = 0;     // 最近一次放行时间
    uint64_t lastRejectMs = 0;      // 最近一次拒绝时间
};

// 单个 key 的统计快照，ExportStats 返回
struct RateLimitSnapshot {
    std::string key;
    RateLimitStat stat;
};

class SelectionRateLimiter {
public:
    static SelectionRateLimiter& GetInstance();

    // 按指定配置尝试获取一次访问许可，返回 true 表示放行
    bool TryAcquire(std::string_view key, const RateLimitConfig& config);

    // 使用默认配置尝试获取
    bool TryAcquire(std::string_view key);

    // 多级串联限流：所有级别均放行才返回 true；任一级别拒绝即短路拒绝
    bool TryAcquireMulti(std::string_view key, const std::vector<RateLimitConfig>& configs);

    // 防抖：距上次触发不足 delayMs 即丢弃（返回 true 表示应被防抖丢弃）
    bool ShouldDebounce(std::string_view key, uint32_t delayMs);

    // 使用默认防抖间隔
    bool ShouldDebounce(std::string_view key);

    // 预热某 key 的令牌桶至满配额（仅对 TOKEN_BUCKET 有意义，其余策略无副作用）
    void Warmup(std::string_view key, const RateLimitConfig& config);

    // 重置某个 key 的限流状态与统计
    void Reset(std::string_view key);

    // 清理全部 key（用于用户切换 / 配置切换 / 服务卸载）
    void ClearAll();

    // 拦截名单：名单内的 key 一律拒绝（业界通用术语 blocklist，避免歧义）
    void AddBlocklist(std::string_view key);
    void RemoveBlocklist(std::string_view key);
    bool IsBlocked(std::string_view key) const;
    void ClearBlocklist();

    // 放行名单：名单内的 key 直接放行，跳过限流（业界通用术语 allowlist，避免歧义）
    void AddAllowlist(std::string_view key);
    void RemoveAllowlist(std::string_view key);
    bool IsAllowed(std::string_view key) const;
    void ClearAllowlist();

    // 查询某个 key 当前剩余可用配额（估算值，令牌桶返回 floor(tokens)）
    uint32_t GetRemainingQuota(std::string_view key, const RateLimitConfig& config);

    // 查询某个 key 的累计统计
    RateLimitStat GetStat(std::string_view key) const;

    // 导出全部 key 的统计快照
    std::vector<RateLimitSnapshot> ExportStats() const;

    // 查询当前维护的 key 数量
    size_t GetContextCount() const;

    // 策略名转字符串，便于日志输出
    static std::string GetStrategyName(LimitStrategy strategy);

private:
    SelectionRateLimiter() = default;
    ~SelectionRateLimiter() = default;
    SelectionRateLimiter(const SelectionRateLimiter&) = delete;
    SelectionRateLimiter& operator=(const SelectionRateLimiter&) = delete;

    struct SlidingWindowContext {
        std::deque<uint64_t> timestamps;  // 窗口内各次放行时间戳
    };

    struct TokenBucketContext {
        double tokens = 0.0;             // 当前令牌数（可为小数）
        uint64_t lastRefillMs = TIMESTAMP_UNSET;  // 最近一次补充令牌时间
    };

    struct FixedWindowContext {
        uint64_t windowStartMs = TIMESTAMP_UNSET;  // 当前窗口起点
        uint32_t count = 0;              // 当前窗口内已通过次数
    };

    struct DebounceContext {
        uint64_t lastTriggerMs = TIMESTAMP_UNSET;  // 最近一次防抖触发时间
    };

    struct LimitContext {
        SlidingWindowContext sliding;
        TokenBucketContext bucket;
        FixedWindowContext fixed;
        DebounceContext debounce;
        RateLimitStat stat;
    };

    bool TryAcquireSliding(LimitContext& ctx, const RateLimitConfig& config, uint64_t nowMs);
    bool TryAcquireToken(LimitContext& ctx, const RateLimitConfig& config, uint64_t nowMs);
    bool TryAcquireFixed(LimitContext& ctx, const RateLimitConfig& config, uint64_t nowMs);
    void RefillToken(TokenBucketContext& bucket, const RateLimitConfig& config, uint64_t nowMs) const;
    uint64_t NowMs() const;
    void EvictIfNeeded();
    static bool IsValidKey(std::string_view key);
    static bool IsValidConfig(const RateLimitConfig& config);

    mutable std::mutex mtx_;
    std::unordered_map<std::string, LimitContext> contexts_;
    std::unordered_set<std::string> blocklist_;
    std::unordered_set<std::string> allowlist_;
};

SelectionRateLimiter& SelectionRateLimiter::GetInstance()
{
    static SelectionRateLimiter instance;
    return instance;
}

uint64_t SelectionRateLimiter::NowMs() const
{
    using namespace std::chrono;
    auto sinceEpoch = steady_clock::now().time_since_epoch();
    return static_cast<uint64_t>(duration_cast<milliseconds>(sinceEpoch).count());
}

bool SelectionRateLimiter::IsValidKey(std::string_view key)
{
    if (key.empty() || key.size() > MAX_KEY_LEN) {
        return false;
    }
    return true;
}

bool SelectionRateLimiter::IsValidConfig(const RateLimitConfig& config)
{
    if (config.windowMs == CONFIG_DISABLED || config.capacity == CONFIG_DISABLED) {
        return false;
    }
    if (config.strategy == LimitStrategy::TOKEN_BUCKET && config.refillPerSec == CONFIG_DISABLED) {
        return false;
    }
    return true;
}

std::string SelectionRateLimiter::GetStrategyName(LimitStrategy strategy)
{
    switch (strategy) {
        case LimitStrategy::SLIDING_WINDOW:
            return "SLIDING_WINDOW";
        case LimitStrategy::TOKEN_BUCKET:
            return "TOKEN_BUCKET";
        case LimitStrategy::FIXED_WINDOW:
            return "FIXED_WINDOW";
        default:
            return "UNKNOWN";
    }
}

void SelectionRateLimiter::EvictIfNeeded()
{
    // 超出上下文数量上限时，淘汰最久未访问的 key，避免内存无界增长
    if (contexts_.size() <= MAX_CONTEXT_COUNT) {
        return;
    }
    uint64_t oldest = UINT64_MAX;
    std::string oldestKey;
    for (const auto& pair : contexts_) {
        uint64_t last = std::max(pair.second.stat.lastAcquireMs, pair.second.stat.lastRejectMs);
        if (last < oldest) {
            oldest = last;
            oldestKey = pair.first;
        }
    }
    if (!oldestKey.empty()) {
        contexts_.erase(oldestKey);
        SELECTION_HILOGD("evict rate-limit context, key=%{public}s", oldestKey.c_str());
    }
}

bool SelectionRateLimiter::TryAcquireSliding(LimitContext& ctx, const RateLimitConfig& config, uint64_t nowMs)
{
    auto& ts = ctx.sliding.timestamps;
    uint64_t windowStart = (nowMs >= config.windowMs) ? (nowMs - config.windowMs) : TIMESTAMP_UNSET;
    // 移除窗口外的过期时间戳
    while (!ts.empty() && ts.front() <= windowStart) {
        ts.pop_front();
    }
    if (ts.size() >= config.capacity) {
        return false;
    }
    ts.push_back(nowMs);
    return true;
}

void SelectionRateLimiter::RefillToken(TokenBucketContext& bucket, const RateLimitConfig& config, uint64_t nowMs) const
{
    if (bucket.lastRefillMs == TIMESTAMP_UNSET) {
        // 首次访问，预填满令牌
        bucket.tokens = static_cast<double>(config.capacity);
        bucket.lastRefillMs = nowMs;
        return;
    }
    if (nowMs <= bucket.lastRefillMs) {
        return;
    }
    double elapsedSec = static_cast<double>(nowMs - bucket.lastRefillMs) / static_cast<double>(MS_PER_SECOND);
    double refill = elapsedSec * static_cast<double>(config.refillPerSec);
    bucket.tokens = std::min(static_cast<double>(config.capacity), bucket.tokens + refill);
    bucket.lastRefillMs = nowMs;
}

bool SelectionRateLimiter::TryAcquireToken(LimitContext& ctx, const RateLimitConfig& config, uint64_t nowMs)
{
    RefillToken(ctx.bucket, config, nowMs);
    if (ctx.bucket.tokens < TOKEN_COST_PER_ACQUIRE) {
        return false;
    }
    ctx.bucket.tokens -= TOKEN_COST_PER_ACQUIRE;
    return true;
}

bool SelectionRateLimiter::TryAcquireFixed(LimitContext& ctx, const RateLimitConfig& config, uint64_t nowMs)
{
    auto& fw = ctx.fixed;
    if (fw.windowStartMs == TIMESTAMP_UNSET || nowMs - fw.windowStartMs >= config.windowMs) {
        // 进入新窗口
        fw.windowStartMs = nowMs;
        fw.count = WINDOW_COUNT_RESET;
    }
    if (fw.count >= config.capacity) {
        return false;
    }
    ++fw.count;
    return true;
}

bool SelectionRateLimiter::TryAcquire(std::string_view key, const RateLimitConfig& config)
{
    if (!IsValidKey(key) || !IsValidConfig(config)) {
        SELECTION_HILOGE("invalid rate-limit param, keyLen=%{public}zu", key.size());
        return false;
    }
    std::string k(key);
    uint64_t now = NowMs();
    std::lock_guard<std::mutex> lock(mtx_);
    // 放行名单优先放行，拦截名单直接拒绝
    if (allowlist_.find(k) != allowlist_.end()) {
        return true;
    }
    if (blocklist_.find(k) != blocklist_.end()) {
        return false;
    }
    EvictIfNeeded();
    auto it = contexts_.find(k);
    if (it == contexts_.end()) {
        it = contexts_.emplace(k, LimitContext{}).first;
    }
    LimitContext& ctx = it->second;

    bool acquired = false;
    switch (config.strategy) {
        case LimitStrategy::SLIDING_WINDOW:
            acquired = TryAcquireSliding(ctx, config, now);
            break;
        case LimitStrategy::TOKEN_BUCKET:
            acquired = TryAcquireToken(ctx, config, now);
            break;
        case LimitStrategy::FIXED_WINDOW:
            acquired = TryAcquireFixed(ctx, config, now);
            break;
        default:
            acquired = false;
            break;
    }

    if (acquired) {
        ++ctx.stat.totalAcquired;
        ctx.stat.lastAcquireMs = now;
    } else {
        ++ctx.stat.totalRejected;
        ctx.stat.lastRejectMs = now;
    }
    return acquired;
}

bool SelectionRateLimiter::TryAcquire(std::string_view key)
{
    RateLimitConfig defaultConfig;
    return TryAcquire(key, defaultConfig);
}

bool SelectionRateLimiter::TryAcquireMulti(std::string_view key, const std::vector<RateLimitConfig>& configs)
{
    if (!IsValidKey(key) || configs.empty()) {
        return false;
    }
    for (const auto& cfg : configs) {
        if (!IsValidConfig(cfg)) {
            return false;
        }
    }
    // 多级限流：逐级判定，任一级别拒绝即短路返回 false
    for (const auto& cfg : configs) {
        if (!TryAcquire(key, cfg)) {
            return false;
        }
    }
    return true;
}

bool SelectionRateLimiter::ShouldDebounce(std::string_view key, uint32_t delayMs)
{
    if (!IsValidKey(key) || delayMs == CONFIG_DISABLED) {
        return false;
    }
    std::string k(key);
    uint64_t now = NowMs();
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = contexts_.find(k);
    if (it == contexts_.end()) {
        it = contexts_.emplace(k, LimitContext{}).first;
    }
    LimitContext& ctx = it->second;
    uint64_t last = ctx.debounce.lastTriggerMs;
    ctx.debounce.lastTriggerMs = now;
    if (last == TIMESTAMP_UNSET) {
        // 首次触发，不防抖
        return false;
    }
    return (now - last) < delayMs;
}

bool SelectionRateLimiter::ShouldDebounce(std::string_view key)
{
    return ShouldDebounce(key, DEFAULT_DEBOUNCE_MS);
}

void SelectionRateLimiter::Warmup(std::string_view key, const RateLimitConfig& config)
{
    if (!IsValidKey(key) || !IsValidConfig(config)) {
        return;
    }
    std::string k(key);
    uint64_t now = NowMs();
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = contexts_.find(k);
    if (it == contexts_.end()) {
        it = contexts_.emplace(k, LimitContext{}).first;
    }
    LimitContext& ctx = it->second;
    if (config.strategy == LimitStrategy::TOKEN_BUCKET) {
        ctx.bucket.tokens = static_cast<double>(config.capacity);
        ctx.bucket.lastRefillMs = now;
        SELECTION_HILOGD("warmup token bucket, key=%{public}s tokens=%{public}u", k.c_str(), config.capacity);
    }
}

void SelectionRateLimiter::Reset(std::string_view key)
{
    if (!IsValidKey(key)) {
        return;
    }
    std::string k(key);
    std::lock_guard<std::mutex> lock(mtx_);
    contexts_.erase(k);
    SELECTION_HILOGI("reset rate-limit context, key=%{public}s", k.c_str());
}

void SelectionRateLimiter::ClearAll()
{
    std::lock_guard<std::mutex> lock(mtx_);
    size_t n = contexts_.size();
    contexts_.clear();
    blocklist_.clear();
    allowlist_.clear();
    SELECTION_HILOGI("clear all rate-limit contexts, count=%{public}zu", n);
}

void SelectionRateLimiter::AddBlocklist(std::string_view key)
{
    if (!IsValidKey(key)) {
        return;
    }
    std::string k(key);
    std::lock_guard<std::mutex> lock(mtx_);
    blocklist_.insert(k);
    SELECTION_HILOGI("add blocklist, key=%{public}s", k.c_str());
}

void SelectionRateLimiter::RemoveBlocklist(std::string_view key)
{
    if (!IsValidKey(key)) {
        return;
    }
    std::string k(key);
    std::lock_guard<std::mutex> lock(mtx_);
    blocklist_.erase(k);
    SELECTION_HILOGI("remove blocklist, key=%{public}s", k.c_str());
}

bool SelectionRateLimiter::IsBlocked(std::string_view key) const
{
    if (!IsValidKey(key)) {
        return false;
    }
    std::string k(key);
    std::lock_guard<std::mutex> lock(mtx_);
    return blocklist_.find(k) != blocklist_.end();
}

void SelectionRateLimiter::ClearBlocklist()
{
    std::lock_guard<std::mutex> lock(mtx_);
    size_t n = blocklist_.size();
    blocklist_.clear();
    SELECTION_HILOGI("clear blocklist, count=%{public}zu", n);
}

void SelectionRateLimiter::AddAllowlist(std::string_view key)
{
    if (!IsValidKey(key)) {
        return;
    }
    std::string k(key);
    std::lock_guard<std::mutex> lock(mtx_);
    allowlist_.insert(k);
    SELECTION_HILOGI("add allowlist, key=%{public}s", k.c_str());
}

void SelectionRateLimiter::RemoveAllowlist(std::string_view key)
{
    if (!IsValidKey(key)) {
        return;
    }
    std::string k(key);
    std::lock_guard<std::mutex> lock(mtx_);
    allowlist_.erase(k);
    SELECTION_HILOGI("remove allowlist, key=%{public}s", k.c_str());
}

bool SelectionRateLimiter::IsAllowed(std::string_view key) const
{
    if (!IsValidKey(key)) {
        return false;
    }
    std::string k(key);
    std::lock_guard<std::mutex> lock(mtx_);
    return allowlist_.find(k) != allowlist_.end();
}

void SelectionRateLimiter::ClearAllowlist()
{
    std::lock_guard<std::mutex> lock(mtx_);
    size_t n = allowlist_.size();
    allowlist_.clear();
    SELECTION_HILOGI("clear allowlist, count=%{public}zu", n);
}

uint32_t SelectionRateLimiter::GetRemainingQuota(std::string_view key, const RateLimitConfig& config)
{
    if (!IsValidKey(key) || !IsValidConfig(config)) {
        return QUOTA_NONE;
    }
    std::string k(key);
    uint64_t now = NowMs();
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = contexts_.find(k);
    if (it == contexts_.end()) {
        // 尚未访问过的 key，按满配额返回
        return config.capacity;
    }
    LimitContext& ctx = it->second;
    uint32_t remaining = QUOTA_NONE;
    switch (config.strategy) {
        case LimitStrategy::SLIDING_WINDOW: {
            auto& ts = ctx.sliding.timestamps;
            uint64_t windowStart = (now >= config.windowMs) ? (now - config.windowMs) : TIMESTAMP_UNSET;
            while (!ts.empty() && ts.front() <= windowStart) {
                ts.pop_front();
            }
            remaining = (ts.size() < config.capacity) ? static_cast<uint32_t>(config.capacity - ts.size()) : QUOTA_NONE;
            break;
        }
        case LimitStrategy::TOKEN_BUCKET: {
            RefillToken(ctx.bucket, config, now);
            remaining = static_cast<uint32_t>(ctx.bucket.tokens);
            break;
        }
        case LimitStrategy::FIXED_WINDOW: {
            auto& fw = ctx.fixed;
            if (fw.windowStartMs == TIMESTAMP_UNSET || now - fw.windowStartMs >= config.windowMs) {
                remaining = config.capacity;
            } else {
                remaining = (fw.count < config.capacity) ? (config.capacity - fw.count) : QUOTA_NONE;
            }
            break;
        }
        default:
            remaining = QUOTA_NONE;
            break;
    }
    return remaining;
}

RateLimitStat SelectionRateLimiter::GetStat(std::string_view key) const
{
    std::string k(key);
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = contexts_.find(k);
    if (it == contexts_.end()) {
        return RateLimitStat{};
    }
    return it->second.stat;
}

std::vector<RateLimitSnapshot> SelectionRateLimiter::ExportStats() const
{
    std::lock_guard<std::mutex> lock(mtx_);
    std::vector<RateLimitSnapshot> snapshots;
    snapshots.reserve(contexts_.size());
    for (const auto& pair : contexts_) {
        RateLimitSnapshot snap;
        snap.key = pair.first;
        snap.stat = pair.second.stat;
        snapshots.push_back(std::move(snap));
    }
    return snapshots;
}

size_t SelectionRateLimiter::GetContextCount() const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return contexts_.size();
}

} // namespace SelectionFwk
} // namespace OHOS
