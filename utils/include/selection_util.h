/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef SELECTION_UTIL_H
#define SELECTION_UTIL_H

#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include "selection_common.h"

namespace OHOS {
namespace SelectionFwk {

namespace SelectionSysParam {
constexpr const char* SWITCH = "sys.selection.switch";
constexpr const char* TRIGGER = "sys.selection.trigger";
constexpr const char* APP = "sys.selection.app";
constexpr const char* TIMEOUT = "sys.selection.timeout";
constexpr const char* UID = "sys.selection.uid";
} // namespace SelectionSysParam

namespace SelectionDefaultValue {
constexpr const char* SWITCH_ON = "on";
constexpr const char* SWITCH_OFF = "off";
constexpr const char* TRIGGER_CTRL = "ctrl";
constexpr const char* TRIGGER_IMMEDIATE = "immediate";
constexpr int32_t DEFAULT_TIMEOUT_MS = 5000;
constexpr int32_t INVALID_USER_ID = -1;
constexpr int32_t DEFAULT_USER_ID = 100;
} // namespace SelectionDefaultValue

class SelectionUtil {
public:
    static size_t CopyNulstr(char *dest, size_t size, const char *src);

    static bool StartWith(const char *str, const char *prefix);
    static bool StartWith(const std::string &str, const std::string &prefix);
    static bool EndsWith(const std::string &str, const std::string &suffix);
    static bool Contains(const std::string &str, const std::string &sub);

    static void RemoveTrailingChars(char c, char *path);
    static void RemoveTrailingChars(const std::string &toRemoved, std::string &path);
    static void RemoveSpace(std::string &str);
    static std::string StringTrim(const std::string &str);
    static std::string ToLower(const std::string &str);
    static std::string ToUpper(const std::string &str);
    static std::string ReplaceAll(const std::string &str, const std::string &from, const std::string &to);
    static size_t CountOccurrences(const std::string &str, const std::string &sub);

    static bool IsEmpty(const char *str) noexcept;
    static bool IsEqual(const char *s1, const char *s2) noexcept;
    static bool IsNumber(const std::string &str);
    static bool IsInteger(const std::string &str);
    static bool IsHexString(const std::string &str);

    static size_t StringSplit(const std::string &str, const std::string &sep,
        std::vector<std::string> &vecList);
    static std::vector<std::string> StringSplit(const std::string &str, char delim);
    static std::string JoinString(const std::vector<std::string> &parts, const std::string &delimiter);
    static bool SplitKeyValue(const std::string &input, const std::string &separator,
        std::string &key, std::string &value);

    static std::string StringPrintf(const char *format, ...);

    static bool StrToUint32(const std::string &str, uint32_t &value);
    static bool StrToInt32(const std::string &str, int32_t &value);
    static bool StrToUint64(const std::string &str, uint64_t &value);
    static bool StrToInt64(const std::string &str, int64_t &value);

    static std::string Anonymize(const char *id);
    static std::string Anonymize(const std::string &id);
    static std::string DFXRadarAnonymize(const char *id);
    static std::string GetAnonyString(const std::string &value);

    static std::string ToHexString(uint64_t value);
    static uint64_t FNV1aHash(const std::string &data);
    static std::string Base64Encode(const std::string &input);
    static std::string Base64Decode(const std::string &input);

    static int64_t GetCurrentTimeMillis();
    static int64_t GetCurrentTimeMicros();
    static void GetTimeStamp(std::string &timestamp);
    static int64_t GetSysClockTime();
    static int64_t GetSysClockTimeMilli(int64_t timeDT);

    static bool IsValidPath(const std::string &rootDir, const std::string &filePath);
    static bool IsValidJsonPath(const std::string &filePath);
    static bool IsFileExists(const std::string &fileName);
    static bool DoesFileExist(const char *path);
    static ssize_t GetFileSize(const char *path);
    static ssize_t GetFileSize(const std::string &filePath);
    static bool CheckFileExtendName(const std::string &filePath, const std::string &checkExtension);
    static std::string ReadFile(const std::string &filePath);
    static std::string ReadJsonFile(const std::string &filePath);
    static void ShowFileAttributes(const char *path);
    static void ShowUserAndGroup();

    static int32_t GetPid();
    static const char* GetProgramName();
    static uint64_t GetThisThreadId();
    static void SetThreadName(const std::string &name);

    static bool ValidateUserId(int32_t userId);
    static bool ValidateNonNegativeInt32(int32_t value);

    static std::optional<std::tuple<std::string, std::string>> ParseAppInfo(const std::string &appInfo);

    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    static std::string ToString(T value)
    {
        return std::to_string(value);
    }

    template<typename T>
    static bool AddInt(T op1, T op2, T minValue, T maxValue, T &res)
    {
        if (op1 >= 0) {
            if (op2 > maxValue - op1) {
                return false;
            }
        } else {
            if (op2 < minValue - op1) {
                return false;
            }
        }
        res = op1 + op2;
        return true;
    }

    static bool AddInt32(int32_t op1, int32_t op2, int32_t &res);
    static bool AddInt64(int64_t op1, int64_t op2, int64_t &res);

    template<typename T>
    static bool MultiplyInt(T op1, T op2, T minVal, T maxVal, T &res)
    {
        if (op1 > 0) {
            if (op2 > 0) {
                if (op1 > maxVal / op2) {
                    return false;
                }
            } else {
                if (op2 < minVal / op1) {
                    return false;
                }
            }
        } else {
            if (op2 > 0) {
                if (op1 < minVal / op2) {
                    return false;
                }
            } else {
                if (op1 != 0 && op2 < maxVal / op1) {
                    return false;
                }
            }
        }
        res = op1 * op2;
        return true;
    }

    static bool MultiplyInt32(int32_t op1, int32_t op2, int32_t &res);
    static bool MultiplyInt64(int64_t op1, int64_t op2, int64_t &res);

    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    static bool IsInRange(T value, T minVal, T maxVal)
    {
        return (value >= minVal) && (value <= maxVal);
    }

    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    static T Clamp(T value, T minVal, T maxVal)
    {
        if (value < minVal) {
            return minVal;
        }
        if (value > maxVal) {
            return maxVal;
        }
        return value;
    }
};

inline bool SelectionUtil::IsEmpty(const char *str) noexcept
{
    return ((str == nullptr) || (str[0] == '\0'));
}

inline bool SelectionUtil::IsEqual(const char *s1, const char *s2) noexcept
{
    if (IsEmpty(s1)) {
        return IsEmpty(s2);
    } else if (IsEmpty(s2)) {
        return false;
    }
    return (std::strcmp(s1, s2) == 0);
}

inline bool SelectionUtil::AddInt32(int32_t op1, int32_t op2, int32_t &res)
{
    return AddInt(op1, op2, std::numeric_limits<int32_t>::min(),
        std::numeric_limits<int32_t>::max(), res);
}

inline bool SelectionUtil::AddInt64(int64_t op1, int64_t op2, int64_t &res)
{
    return AddInt(op1, op2, std::numeric_limits<int64_t>::min(),
        std::numeric_limits<int64_t>::max(), res);
}

inline bool SelectionUtil::MultiplyInt32(int32_t op1, int32_t op2, int32_t &res)
{
    return MultiplyInt(op1, op2, std::numeric_limits<int32_t>::min(),
        std::numeric_limits<int32_t>::max(), res);
}

inline bool SelectionUtil::MultiplyInt64(int64_t op1, int64_t op2, int64_t &res)
{
    return MultiplyInt(op1, op2, std::numeric_limits<int64_t>::min(),
        std::numeric_limits<int64_t>::max(), res);
}

} // namespace SelectionFwk
} // namespace OHOS
#endif // SELECTION_UTIL_H
