/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <string>
#include "selection_common.h"
#include "selection_log.h"
namespace OHOS {
namespace SelectionFwk {
static const unsigned int UTF8_2BYTE_LEN = 2;
static const unsigned int UTF8_3BYTE_LEN = 3;
static const unsigned int UTF8_4BYTE_LEN = 4;

bool AbilityRuntimeInfo::operator==(const AbilityRuntimeInfo& other) const
{
    if (this == &other) {
        return true;
    }
    return (userId == other.userId && bundleName == other.bundleName && abilityName == other.abilityName);
}

bool IsNumber(const std::string& str)
{
    if (str.empty()) {
        return false;
    }

    size_t i = 0;
    if (str[i] == '+' || str[i] == '-') {
        i++;
    }

    if (i != 0 && str.length() == 1) {
        return false;
    }

    if (std::all_of(str.begin() + i, str.end(), [](unsigned char c) { return std::isdigit(c); })) {
        return true;
    }

    return false;
}

bool IsAllWhitespace(const std::string &str)
{
    static const std::string invisibleChars =
        " \t\n\r\f\v"
        "\u00A0\u1680\u180E"
        "\u2000\u2001\u2002\u2003\u2004\u2005\u2006\u2007\u2008\u2009\u200A"
        "\u200B\u200C\u200D\u200E\u200F"
        "\u2028\u2029\u202A\u202B\u202C\u202D\u202E"
        "\u205F\u2060\u2061\u2062\u2063\u2064"
        "\u3000\uFEFF";

    for (size_t i = 0; i < str.size();) {
        if (static_cast<unsigned char>(str[i]) < 0x80) {
            if (invisibleChars.find(str[i]) == std::string::npos) {
                return false;
            }
            ++i;
        } else {
            uint32_t len = 0;
            if ((str[i] & 0xE0) == 0xC0) {
                len = UTF8_2BYTE_LEN;
            } else if ((str[i] & 0xF0) == 0xE0) {
                len = UTF8_3BYTE_LEN;
            } else if ((str[i] & 0xF8) == 0xF0) {
                len = UTF8_4BYTE_LEN;
            }

            std::string utf8Char = str.substr(i, len);
            if (invisibleChars.find(utf8Char) == std::string::npos) {
                return false;
            }

            i += len;
        }
    }

    return true;
}

std::optional<std::tuple<std::string, std::string>> ParseAppInfo(const std::string& appInfo)
{
    auto pos = appInfo.find('/');
    if (pos == std::string::npos) {
        SELECTION_HILOGE("app info: %{public}s is invalid!", appInfo.c_str());
        return std::nullopt;
    }
    const std::string bundleName = appInfo.substr(0, pos);
    const std::string extName = appInfo.substr(pos + 1);
    if (bundleName.empty() || extName.empty()) {
        SELECTION_HILOGE("bundleName or extName is empty");
        return std::nullopt;
    }
    return std::make_tuple(bundleName, extName);
}
} // namespace SelectionFwk
} // namespace OHOS
