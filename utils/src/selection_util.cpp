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

#include "selection_util.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <grp.h>
#include <limits>
#include <map>
#include <pwd.h>
#include <regex>
#include <sstream>
#include <unistd.h>
#include <vector>

#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include "securec.h"
#include "selection_log.h"

#undef LOG_TAG
#define LOG_TAG "SelectionUtil"

namespace OHOS {
namespace SelectionFwk {
namespace {
constexpr size_t SUBSTR_ID_LENGTH { 5 };
constexpr int32_t MULTIPLES { 2 };
constexpr size_t DFX_RADAR_MASK_SIZE { 2 };
constexpr int64_t TIME_CONVERSION_UNIT { 1000 };
constexpr int64_t TIME_ROUND_UP { 999 };
constexpr size_t BUF_TID_SIZE { 10 };
constexpr size_t PROGRAM_NAME_SIZE { 256 };
constexpr size_t BUF_CMD_SIZE { 512 };
constexpr uint32_t BASE_YEAR { 1900 };
constexpr uint32_t BASE_MON { 1 };
constexpr uint32_t MS_NS { 1000000 };
constexpr int32_t FILE_SIZE_MAX { 0x5000 };
constexpr size_t SHORT_KEY_LENGTH { 20 };
constexpr size_t PLAINTEXT_LENGTH { 4 };
constexpr size_t PRINTF_BUF_SIZE { 1024 };
constexpr size_t READ_FILE_BUF_SIZE { 256 };
constexpr int32_t MAX_THREAD_NAME_LEN { 15 };
constexpr int32_t BASE64_PAD_CHAR { '=' };
constexpr int32_t BASE64_INVALID_CHAR { -1 };
constexpr uint64_t FNV_OFFSET_BASIS { 14695981039346656037ULL };
constexpr uint64_t FNV_PRIME { 1099511628211ULL };
constexpr int32_t INVALID_FILE_SIZE { -1 };

const char BASE64_TABLE[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

const std::string SELECTION_PATH_ROOT = "/system/etc/selection/";
const std::string JSON_PATH_ROOT = "/system/etc/selectionfwk/";
} // namespace

size_t SelectionUtil::CopyNulstr(char *dest, size_t size, const char *src)
{
    SELECTION_CHECK(dest != nullptr, return 0, "dest is nullptr");
    SELECTION_CHECK(src != nullptr, return 0, "src is nullptr");

    size_t len = strlen(src);
    if (len >= size) {
        if (size > 1) {
            len = size - 1;
        } else {
            len = 0;
        }
    }
    if (len > 0) {
        errno_t ret = memcpy_s(dest, size, src, len);
        if (ret != EOK) {
            SELECTION_HILOGW("memcpy_s:bounds checking failed");
        }
    }
    if (size > 0) {
        dest[len] = '\0';
    }
    return len;
}

bool SelectionUtil::StartWith(const char *str, const char *prefix)
{
    if (str == nullptr || prefix == nullptr) {
        return false;
    }
    size_t prefixlen = strlen(prefix);
    return (prefixlen > 0 ? (strncmp(str, prefix, strlen(prefix)) == 0) : false);
}

bool SelectionUtil::StartWith(const std::string &str, const std::string &prefix)
{
    if (str.size() < prefix.size()) {
        return false;
    }
    return (str.compare(0, prefix.size(), prefix) == 0);
}

bool SelectionUtil::EndsWith(const std::string &str, const std::string &suffix)
{
    if (str.size() < suffix.size()) {
        return false;
    }
    return (str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0);
}

bool SelectionUtil::Contains(const std::string &str, const std::string &sub)
{
    return str.find(sub) != std::string::npos;
}

void SelectionUtil::RemoveTrailingChars(char c, char *path)
{
    SELECTION_CHECK(path != nullptr, return, "path is nullptr");
    size_t len = strlen(path);
    while (len > 0 && path[len - 1] == c) {
        path[--len] = '\0';
    }
}

void SelectionUtil::RemoveTrailingChars(const std::string &toRemoved, std::string &path)
{
    while (!path.empty() && (toRemoved.find(path.back()) != std::string::npos)) {
        path.pop_back();
    }
}

void SelectionUtil::RemoveSpace(std::string &str)
{
    str.erase(remove_if(str.begin(), str.end(), [](unsigned char c) { return std::isspace(c);}), str.end());
}

std::string SelectionUtil::StringTrim(const std::string &str)
{
    if (str.empty()) {
        return "";
    }
    size_t start = 0;
    while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start]))) {
        start++;
    }
    if (start >= str.size()) {
        return "";
    }
    size_t end = str.size() - 1;
    while (end > start && std::isspace(static_cast<unsigned char>(str[end]))) {
        end--;
    }
    return str.substr(start, end - start + 1);
}

std::string SelectionUtil::ToLower(const std::string &str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

std::string SelectionUtil::ToUpper(const std::string &str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return result;
}

std::string SelectionUtil::ReplaceAll(const std::string &str, const std::string &from, const std::string &to)
{
    if (from.empty()) {
        return str;
    }
    std::string result = str;
    size_t pos = 0;
    while ((pos = result.find(from, pos)) != std::string::npos) {
        result.replace(pos, from.length(), to);
        pos += to.length();
    }
    return result;
}

size_t SelectionUtil::CountOccurrences(const std::string &str, const std::string &sub)
{
    if (sub.empty()) {
        return 0;
    }
    size_t count = 0;
    size_t pos = 0;
    while ((pos = str.find(sub, pos)) != std::string::npos) {
        count++;
        pos += sub.length();
    }
    return count;
}

bool SelectionUtil::IsNumber(const std::string &str)
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
    if (i >= str.length()) {
        return false;
    }
    return std::all_of(str.begin() + i, str.end(), [](unsigned char c) { return std::isdigit(c); });
}

bool SelectionUtil::IsInteger(const std::string &str)
{
    std::regex pattern("^\\s*-?(0|([1-9]\\d*))\\s*$");
    return std::regex_match(str, pattern);
}

bool SelectionUtil::IsHexString(const std::string &str)
{
    if (str.empty()) {
        return false;
    }
    size_t start = 0;
    if (str.size() > 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        start = 2;
    }
    if (start >= str.size()) {
        return false;
    }
    return std::all_of(str.begin() + start, str.end(), [](unsigned char c) {
        return std::isxdigit(c) != 0;
    });
}

size_t SelectionUtil::StringSplit(const std::string &str, const std::string &sep,
    std::vector<std::string> &vecList)
{
    auto strs = str;
    auto StringToken = [&sep](std::string &s, std::string &token) -> size_t {
        token = "";
        if (s.empty()) {
            return s.npos;
        }
        size_t seat = s.npos;
        size_t temp = 0;
        for (auto &item : sep) {
            temp = s.find(item);
            if (s.npos != temp) {
                seat = (std::min)(seat, temp);
            }
        }
        if (s.npos != seat) {
            token = s.substr(0, seat);
            if (s.npos != seat + 1) {
                s = s.substr(seat + 1, s.npos);
            }
        } else {
            token = s;
            s = "";
        }
        return token.size();
    };

    size_t size = 0;
    std::string token;
    while (str.npos != (size = StringToken(strs, token))) {
        vecList.push_back(token);
    }
    return vecList.size();
}

std::vector<std::string> SelectionUtil::StringSplit(const std::string &str, char delim)
{
    std::vector<std::string> elems;
    std::size_t previous = 0;
    std::size_t current = str.find(delim);
    while (current != std::string::npos) {
        if (current > previous) {
            elems.push_back(str.substr(previous, current - previous));
        }
        previous = current + 1;
        current = str.find(delim, previous);
    }
    if (previous != str.size()) {
        elems.push_back(str.substr(previous));
    }
    return elems;
}

std::string SelectionUtil::JoinString(const std::vector<std::string> &parts, const std::string &delimiter)
{
    if (parts.empty()) {
        return "";
    }
    std::ostringstream ss;
    ss << parts[0];
    for (size_t i = 1; i < parts.size(); ++i) {
        ss << delimiter << parts[i];
    }
    return ss.str();
}

bool SelectionUtil::SplitKeyValue(const std::string &input, const std::string &separator,
    std::string &key, std::string &value)
{
    size_t pos = input.find(separator);
    if (pos == std::string::npos) {
        return false;
    }
    key = StringTrim(input.substr(0, pos));
    value = StringTrim(input.substr(pos + separator.length()));
    return !key.empty();
}

std::string SelectionUtil::StringPrintf(const char *format, ...)
{
    char space[PRINTF_BUF_SIZE] { 0 };
    va_list ap;
    va_start(ap, format);
    std::string result;
    int32_t ret = vsnprintf_s(space, sizeof(space), sizeof(space) - 1, format, ap);
    if (ret >= 0 && static_cast<size_t>(ret) < sizeof(space)) {
        result = space;
    } else {
        SELECTION_HILOGE("The buffer is overflow");
    }
    va_end(ap);
    return result;
}

bool SelectionUtil::StrToUint32(const std::string &str, uint32_t &value)
{
    if (str.empty()) {
        return false;
    }
    if (!IsNumber(str)) {
        SELECTION_HILOGE("not a valid number: %{public}s", str.c_str());
        return false;
    }
    errno = 0;
    char *endPtr = nullptr;
    unsigned long temp = strtoul(str.c_str(), &endPtr, 10);
    if (errno != 0 || *endPtr != '\0') {
        SELECTION_HILOGE("strtoul conversion failed: %{public}s", str.c_str());
        return false;
    }
    if (temp > std::numeric_limits<uint32_t>::max()) {
        SELECTION_HILOGE("value exceeds uint32 range: %{public}s", str.c_str());
        return false;
    }
    value = static_cast<uint32_t>(temp);
    return true;
}

bool SelectionUtil::StrToInt32(const std::string &str, int32_t &value)
{
    if (str.empty()) {
        return false;
    }
    if (!IsNumber(str)) {
        SELECTION_HILOGE("not a valid number: %{public}s", str.c_str());
        return false;
    }
    errno = 0;
    char *endPtr = nullptr;
    long temp = strtol(str.c_str(), &endPtr, 10);
    if (errno != 0 || *endPtr != '\0') {
        SELECTION_HILOGE("strtol conversion failed: %{public}s", str.c_str());
        return false;
    }
    if (temp < std::numeric_limits<int32_t>::min() ||
        temp > std::numeric_limits<int32_t>::max()) {
        SELECTION_HILOGE("value exceeds int32 range: %{public}s", str.c_str());
        return false;
    }
    value = static_cast<int32_t>(temp);
    return true;
}

bool SelectionUtil::StrToUint64(const std::string &str, uint64_t &value)
{
    if (str.empty()) {
        return false;
    }
    if (!IsNumber(str)) {
        SELECTION_HILOGE("not a valid number: %{public}s", str.c_str());
        return false;
    }
    errno = 0;
    char *endPtr = nullptr;
    unsigned long long temp = strtoull(str.c_str(), &endPtr, 10);
    if (errno != 0 || *endPtr != '\0') {
        SELECTION_HILOGE("strtoull conversion failed: %{public}s", str.c_str());
        return false;
    }
    value = static_cast<uint64_t>(temp);
    return true;
}

bool SelectionUtil::StrToInt64(const std::string &str, int64_t &value)
{
    if (str.empty()) {
        return false;
    }
    if (!IsNumber(str)) {
        SELECTION_HILOGE("not a valid number: %{public}s", str.c_str());
        return false;
    }
    errno = 0;
    char *endPtr = nullptr;
    long long temp = strtoll(str.c_str(), &endPtr, 10);
    if (errno != 0 || *endPtr != '\0') {
        SELECTION_HILOGE("strtoll conversion failed: %{public}s", str.c_str());
        return false;
    }
    value = static_cast<int64_t>(temp);
    return true;
}

std::string SelectionUtil::Anonymize(const char *id)
{
    if (id == nullptr) {
        return std::string(MULTIPLES * SUBSTR_ID_LENGTH, '*');
    }
    std::string idStr(id);
    if (idStr.empty() || idStr.length() < SUBSTR_ID_LENGTH) {
        return std::string(MULTIPLES * SUBSTR_ID_LENGTH, '*');
    }
    return idStr.substr(0, SUBSTR_ID_LENGTH) + std::string(SUBSTR_ID_LENGTH, '*') +
        idStr.substr(idStr.length() - SUBSTR_ID_LENGTH);
}

std::string SelectionUtil::Anonymize(const std::string &id)
{
    return Anonymize(id.c_str());
}

std::string SelectionUtil::DFXRadarAnonymize(const char *id)
{
    if (id == nullptr) {
        return std::string(MULTIPLES * SUBSTR_ID_LENGTH, '*');
    }
    std::string idStr(id);
    if (idStr.empty() || idStr.length() < SUBSTR_ID_LENGTH) {
        return std::string(MULTIPLES * SUBSTR_ID_LENGTH, '*');
    }
    return idStr.substr(0, SUBSTR_ID_LENGTH) + std::string(DFX_RADAR_MASK_SIZE, '*') +
        idStr.substr(idStr.length() - SUBSTR_ID_LENGTH);
}

std::string SelectionUtil::GetAnonyString(const std::string &value)
{
    if (value.empty()) {
        return "empty";
    }
    std::string anonyStr = "******";
    std::string str;
    size_t strLen = value.length();
    if (strLen == 0) {
        SELECTION_HILOGE("strLen is 0, value will overflow");
        return "empty";
    } else if (strLen <= SHORT_KEY_LENGTH) {
        str += value[0];
        str += anonyStr;
        str += value[strLen - 1];
    } else {
        str.append(value, 0, PLAINTEXT_LENGTH);
        str += anonyStr;
        str.append(value, strLen - PLAINTEXT_LENGTH, PLAINTEXT_LENGTH);
    }
    return str;
}

std::string SelectionUtil::ToHexString(uint64_t value)
{
    std::ostringstream ss;
    ss << "0x" << std::hex << value;
    return ss.str();
}

uint64_t SelectionUtil::FNV1aHash(const std::string &data)
{
    uint64_t hash = FNV_OFFSET_BASIS;
    for (size_t i = 0; i < data.size(); ++i) {
        hash ^= static_cast<uint64_t>(static_cast<unsigned char>(data[i]));
        hash *= FNV_PRIME;
    }
    return hash;
}

std::string SelectionUtil::Base64Encode(const std::string &input)
{
    std::string output;
    size_t inLen = input.size();
    if (inLen == 0) {
        return output;
    }
    const unsigned char *src = reinterpret_cast<const unsigned char *>(input.data());
    size_t i = 0;
    while (i < inLen) {
        uint32_t triple = 0;
        int32_t padding = 0;
        for (int32_t j = 0; j < 3; ++j) {
            triple <<= 8;
            if (i < inLen) {
                triple |= src[i++];
            } else {
                ++padding;
            }
        }
        for (int32_t j = 3; j >= 0; --j) {
            if (padding > 0 && j < padding) {
                output.push_back(static_cast<char>(BASE64_PAD_CHAR));
            } else {
                uint32_t index = (triple >> (j * 6)) & 0x3F;
                output.push_back(BASE64_TABLE[index]);
            }
        }
    }
    return output;
}

static int32_t DecodeBase64Char(char c)
{
    if (c >= 'A' && c <= 'Z') {
        return c - 'A';
    }
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 26;
    }
    if (c >= '0' && c <= '9') {
        return c - '0' + 52;
    }
    if (c == '+') {
        return 62;
    }
    if (c == '/') {
        return 63;
    }
    return BASE64_INVALID_CHAR;
}

std::string SelectionUtil::Base64Decode(const std::string &input)
{
    std::string output;
    size_t inLen = input.size();
    if (inLen == 0 || inLen % 4 != 0) {
        SELECTION_HILOGE("invalid base64 input length: %{public}zu", inLen);
        return output;
    }
    for (size_t i = 0; i < inLen; i += 4) {
        int32_t vals[4];
        int32_t padCount = 0;
        for (int32_t j = 0; j < 4; ++j) {
            char c = input[i + j];
            if (c == BASE64_PAD_CHAR) {
                vals[j] = 0;
                ++padCount;
            } else {
                vals[j] = DecodeBase64Char(c);
                if (vals[j] == BASE64_INVALID_CHAR) {
                    SELECTION_HILOGE("invalid base64 character at position %{public}zu", i + j);
                    return "";
                }
            }
        }
        uint32_t triple = (static_cast<uint32_t>(vals[0]) << 18) |
                          (static_cast<uint32_t>(vals[1]) << 12) |
                          (static_cast<uint32_t>(vals[2]) << 6) |
                          static_cast<uint32_t>(vals[3]);
        output.push_back(static_cast<char>((triple >> 16) & 0xFF));
        if (padCount < 2) {
            output.push_back(static_cast<char>((triple >> 8) & 0xFF));
        }
        if (padCount < 1) {
            output.push_back(static_cast<char>(triple & 0xFF));
        }
    }
    return output;
}

int64_t SelectionUtil::GetCurrentTimeMillis()
{
    auto timeNow = std::chrono::time_point_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now());
    auto tmp = std::chrono::duration_cast<std::chrono::milliseconds>(timeNow.time_since_epoch());
    return tmp.count();
}

int64_t SelectionUtil::GetCurrentTimeMicros()
{
    auto timeNow = std::chrono::time_point_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now());
    auto tmp = std::chrono::duration_cast<std::chrono::microseconds>(timeNow.time_since_epoch());
    return tmp.count();
}

void SelectionUtil::GetTimeStamp(std::string &timestamp)
{
    timespec curTime;
    clock_gettime(CLOCK_REALTIME, &curTime);
    struct tm timeinfo;
    localtime_r(&(curTime.tv_sec), &timeinfo);
    timestamp.append(std::to_string(timeinfo.tm_year + BASE_YEAR)).append("-")
        .append(std::to_string(timeinfo.tm_mon + BASE_MON)).append("-")
        .append(std::to_string(timeinfo.tm_mday)).append(" ")
        .append(std::to_string(timeinfo.tm_hour)).append(":")
        .append(std::to_string(timeinfo.tm_min)).append(":")
        .append(std::to_string(timeinfo.tm_sec)).append(".")
        .append(std::to_string(curTime.tv_nsec / MS_NS));
}

int64_t SelectionUtil::GetSysClockTime()
{
    struct timespec ts = { 0, 0 };
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        SELECTION_HILOGE("clock_gettime failed:%{public}d", errno);
        return 0;
    }
    if (static_cast<uint64_t>(ts.tv_sec) >
        static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) /
        (TIME_CONVERSION_UNIT * TIME_CONVERSION_UNIT)) {
        SELECTION_HILOGE("Integer overflow detected!");
        return 0;
    }
    uint64_t totalMicroSeconds = static_cast<uint64_t>(ts.tv_sec) *
        TIME_CONVERSION_UNIT * TIME_CONVERSION_UNIT +
        static_cast<uint64_t>(ts.tv_nsec) / TIME_CONVERSION_UNIT;
    if (totalMicroSeconds > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
        SELECTION_HILOGE("Total time value integer overflow detected!");
        return 0;
    }
    return static_cast<int64_t>(totalMicroSeconds);
}

int64_t SelectionUtil::GetSysClockTimeMilli(int64_t timeDT)
{
    return (timeDT + TIME_ROUND_UP) / TIME_CONVERSION_UNIT;
}

bool SelectionUtil::IsValidPath(const std::string &rootDir, const std::string &filePath)
{
    if (rootDir.empty() || filePath.empty()) {
        return false;
    }
    return (filePath.compare(0, rootDir.size(), rootDir) == 0);
}

bool SelectionUtil::IsValidJsonPath(const std::string &filePath)
{
    return IsValidPath(JSON_PATH_ROOT, filePath);
}

bool SelectionUtil::IsFileExists(const std::string &fileName)
{
    if (fileName.empty()) {
        return false;
    }
    return (access(fileName.c_str(), F_OK) == 0);
}

bool SelectionUtil::DoesFileExist(const char *path)
{
    if (path == nullptr) {
        return false;
    }
    return (access(path, F_OK) == 0);
}

ssize_t SelectionUtil::GetFileSize(const std::string &filePath)
{
    return GetFileSize(filePath.c_str());
}

ssize_t SelectionUtil::GetFileSize(const char *path)
{
    if (path == nullptr) {
        return INVALID_FILE_SIZE;
    }
    struct stat buf {};
    ssize_t sz { 0 };
    if (stat(path, &buf) == 0) {
        if (S_ISREG(buf.st_mode)) {
            sz = buf.st_size;
        } else {
            SELECTION_HILOGE("Not regular file");
        }
    } else {
        SELECTION_HILOGE("stat failed:%{public}d", errno);
    }
    return sz;
}

bool SelectionUtil::CheckFileExtendName(const std::string &filePath, const std::string &checkExtension)
{
    std::string::size_type pos = filePath.find_last_of('.');
    if (pos == std::string::npos) {
        SELECTION_HILOGE("File is not found extension");
        return false;
    }
    return (filePath.substr(pos + 1, filePath.npos) == checkExtension);
}

std::string SelectionUtil::ReadFile(const std::string &filePath)
{
    if (filePath.empty()) {
        SELECTION_HILOGE("filePath is empty");
        return "";
    }
    FILE *fp = fopen(filePath.c_str(), "r");
    if (fp == nullptr) {
        SELECTION_HILOGE("fopen failed for %{public}s", filePath.c_str());
        return "";
    }
    std::string dataStr;
    char buf[READ_FILE_BUF_SIZE] = {};
    while (fgets(buf, sizeof(buf), fp) != nullptr) {
        dataStr += buf;
    }
    if (fclose(fp) != 0) {
        SELECTION_HILOGW("Close file failed");
    }
    return dataStr;
}

std::string SelectionUtil::ReadJsonFile(const std::string &filePath)
{
    if (filePath.empty()) {
        SELECTION_HILOGE("FilePath is empty");
        return "";
    }
    char realPath[PATH_MAX] = {};
    if (realpath(filePath.c_str(), realPath) == nullptr) {
        SELECTION_HILOGE("Realpath return nullptr");
        return "";
    }
    if (!IsValidJsonPath(realPath)) {
        SELECTION_HILOGE("File path is error");
        return "";
    }
    if (!CheckFileExtendName(realPath, "json")) {
        SELECTION_HILOGE("Unable to parse files other than json format");
        return "";
    }
    if (!IsFileExists(realPath)) {
        SELECTION_HILOGE("File is not existent");
        return "";
    }
    ssize_t fileSize = GetFileSize(realPath);
    if (fileSize <= 0 || fileSize > FILE_SIZE_MAX) {
        SELECTION_HILOGE("File size out of read range, size:%{public}zd", fileSize);
        return "";
    }
    return ReadFile(realPath);
}

void SelectionUtil::ShowFileAttributes(const char *path)
{
    SELECTION_CHECK(path != nullptr, return, "path is nullptr");
    SELECTION_HILOGD("======================= File Attributes ========================");
    SELECTION_HILOGD("%{public}20s:%{public}s", "FILE NAME", path);

    struct stat buf {};
    if (stat(path, &buf) != 0) {
        SELECTION_HILOGE("stat failed:%{public}d", errno);
        return;
    }
    if (S_ISDIR(buf.st_mode)) {
        SELECTION_HILOGD("%{public}20s: directory", "TYPE");
    } else if (S_ISCHR(buf.st_mode)) {
        SELECTION_HILOGD("%{public}20s: character special file", "TYPE");
    } else if (S_ISREG(buf.st_mode)) {
        SELECTION_HILOGD("%{public}20s: regular file", "TYPE");
    }

    std::ostringstream ss;
    std::map<mode_t, std::string> modes {{S_IRUSR, "U+R "}, {S_IWUSR, "U+W "}, {S_IXUSR, "U+X "},
        {S_IRGRP, "G+R "}, {S_IWGRP, "G+W "}, {S_IXGRP, "G+X "}, {S_IROTH, "O+R "},
        {S_IWOTH, "O+W "}, {S_IXOTH, "O+X "}};
    for (const auto &element : modes) {
        if (buf.st_mode & element.first) {
            ss << element.second;
            break;
        }
    }
    SELECTION_HILOGD("%{public}20s:%{public}s", "PERMISSIONS", ss.str().c_str());
}

void SelectionUtil::ShowUserAndGroup()
{
    static constexpr size_t BUFSIZE { 1024 };
    char buffer[BUFSIZE];
    struct passwd buf;
    struct passwd *pbuf = nullptr;
    struct group grp;
    struct group *pgrp = nullptr;

    SELECTION_HILOGD("======================= Users and Groups =======================");
    uid_t uid = getuid();
    if (getpwuid_r(uid, &buf, buffer, sizeof(buffer), &pbuf) != 0) {
        SELECTION_HILOGE("getpwuid_r failed:%{public}d", errno);
    } else {
        SELECTION_HILOGD("%{public}20s:%{public}10u", "USER", uid);
    }

    gid_t gid = getgid();
    if (getgrgid_r(gid, &grp, buffer, sizeof(buffer), &pgrp) != 0) {
        SELECTION_HILOGE("getgrgid_r failed:%{public}d", errno);
    } else {
        SELECTION_HILOGD("%{public}20s:%{public}10u", "GROUP", gid);
    }

    uid = geteuid();
    if (getpwuid_r(uid, &buf, buffer, sizeof(buffer), &pbuf) != 0) {
        SELECTION_HILOGE("getpwuid_r failed:%{public}d", errno);
    } else {
        SELECTION_HILOGD("%{public}20s:%{public}10u", "EFFECTIVE USER", uid);
    }

    gid = getegid();
    if (getgrgid_r(gid, &grp, buffer, sizeof(buffer), &pgrp) != 0) {
        SELECTION_HILOGE("getgrgid_r failed:%{public}d", errno);
    } else {
        SELECTION_HILOGD("%{public}20s:%{public}10u", "EFFECTIVE GROUP", gid);
    }

    gid_t groups[NGROUPS_MAX + 1];
    int32_t ngrps = getgroups(sizeof(groups), groups);
    for (int32_t i = 0; i < ngrps; ++i) {
        if (getgrgid_r(groups[i], &grp, buffer, sizeof(buffer), &pgrp) != 0) {
            SELECTION_HILOGE("getgrgid_r failed:%{public}d", errno);
        } else {
            SELECTION_HILOGD("%{public}20s:%{public}10u", "SUPPLEMENTARY GROUP", groups[i]);
        }
    }
}

int32_t SelectionUtil::GetPid()
{
    return static_cast<int32_t>(getpid());
}

const char* SelectionUtil::GetProgramName()
{
    static char programName[PROGRAM_NAME_SIZE] = { 0 };
    if (programName[0] != '\0') {
        return programName;
    }

    char buf[BUF_CMD_SIZE] = { 0 };
    int32_t ret = sprintf_s(buf, BUF_CMD_SIZE, "/proc/%d/cmdline", static_cast<int32_t>(getpid()));
    if (ret == -1) {
        SELECTION_HILOGE("GetProcessInfo sprintf_s cmdline error");
        return "";
    }
    FILE *fp = fopen(buf, "rb");
    if (fp == nullptr) {
        SELECTION_HILOGE("The fp is nullptr, filename:%{public}s", buf);
        return "";
    }
    static constexpr size_t bufLineSize = 512;
    char bufLine[bufLineSize] = { 0 };
    if ((fgets(bufLine, bufLineSize, fp) == nullptr)) {
        SELECTION_HILOGE("fgets failed");
        if (fclose(fp) != 0) {
            SELECTION_HILOGW("Close file failed");
        }
        return "";
    }
    if (fclose(fp) != 0) {
        SELECTION_HILOGW("Close file failed");
    }

    std::string tempName(bufLine);
    size_t nPos = tempName.find_last_of('/');
    if (nPos != std::string::npos) {
        tempName = tempName.substr(nPos + 1);
    }
    if (tempName.empty()) {
        SELECTION_HILOGE("tempName is empty");
        return "";
    }
    size_t copySize = std::min(tempName.size(), PROGRAM_NAME_SIZE - 1);
    if (copySize == 0) {
        SELECTION_HILOGE("The copySize is 0");
        return "";
    }
    errno_t result = memcpy_s(programName, PROGRAM_NAME_SIZE, tempName.c_str(), copySize);
    if (result != EOK) {
        SELECTION_HILOGE("memcpy_s failed");
        return "";
    }
    SELECTION_HILOGI("Get program name success, programName:%{public}s", programName);
    return programName;
}

uint64_t SelectionUtil::GetThisThreadId()
{
    thread_local std::string threadLocalId;
    if (threadLocalId.empty()) {
        long tid = syscall(SYS_gettid);
        char buf[BUF_TID_SIZE] = { 0 };
        int32_t ret = sprintf_s(buf, BUF_TID_SIZE, "%06d", static_cast<int32_t>(tid));
        if (ret < 0) {
            SELECTION_HILOGE("Call sprintf_s failed, ret:%{public}d", ret);
            return 0;
        }
        buf[BUF_TID_SIZE - 1] = '\0';
        threadLocalId = buf;
    }
    errno = 0;
    char *endPtr = nullptr;
    unsigned long long tid = strtoull(threadLocalId.c_str(), &endPtr, 10);
    if (errno != 0 || *endPtr != '\0') {
        SELECTION_HILOGE("strtoull failed for thread id");
        return 0;
    }
    return static_cast<uint64_t>(tid);
}

void SelectionUtil::SetThreadName(const std::string &name)
{
    if (name.empty()) {
        return;
    }
    if (name.length() > static_cast<size_t>(MAX_THREAD_NAME_LEN)) {
        SELECTION_HILOGW("name truncated to: %{public}s",
            name.substr(0, MAX_THREAD_NAME_LEN).c_str());
    }
    std::string truncatedName = name.substr(0, MAX_THREAD_NAME_LEN);
    prctl(PR_SET_NAME, truncatedName.c_str());
}

bool SelectionUtil::ValidateUserId(int32_t userId)
{
    return userId >= 0;
}

bool SelectionUtil::ValidateNonNegativeInt32(int32_t value)
{
    return value >= 0;
}

std::optional<std::tuple<std::string, std::string>> SelectionUtil::ParseAppInfo(const std::string &appInfo)
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
