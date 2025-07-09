/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#ifndef SELECTION_API_EVENT_REPORTER_H
#define SELECTION_API_EVENT_REPORTER_H

#include <memory>
#include <mutex>
#include <random>
#include <string>

namespace OHOS {
namespace SelectionFwk {
class RandomGenerator {
public:
    RandomGenerator() {
        engine_ = std::make_unique<std::mt19937>(rd_());
    }

    int Generate() {
        std::uniform_int_distribution<int> dist;
        std::lock_guard<std::mutex> lock(mtx_);
        return dist(*engine_);
    }

private:
    std::random_device rd_;
    std::unique_ptr<std::mt19937> engine_;
    std::mutex mtx_;
};

class SelectionApiEventReporter {
public:
    explicit SelectionApiEventReporter(const std::string& apiName);
    ~SelectionApiEventReporter() = default;
    void WriteEndEvent(const int result, const int32_t errCode);

public:
    constexpr static int32_t API_SUCCESS = 0;
    constexpr static int32_t API_FAIL = 1;

private:
    std::string GenerateTransId() const;

private:
    static RandomGenerator randomGenerator_;

    std::string transId_;
    std::string apiName_;
    int64_t beginTime_;
};
} // SelectionFwk
} // OHOS
#endif  // SELECTION_API_EVENT_REPORTER_H
