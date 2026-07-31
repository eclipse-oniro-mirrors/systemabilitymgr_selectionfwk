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

#include "selection_string_converter.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <regex>
#include <random>

namespace OHOS {
namespace SelectionFwk {
namespace {
// 魔鬼数字具名常量
constexpr uint8_t ASCII_MAX = 127;              // ASCII 字符上限
constexpr uint8_t ASCII_CONTROL_MAX = 32;       // ASCII 控制字符阈值（小于该值为控制字符）
constexpr uint32_t DJB2_HASH_OFFSET = 5381;     // DJB2 哈希初值
constexpr uint64_t FNV_OFFSET_BASIS_64 = 14695981039346656037ULL; // FNV-1a 64 位偏移初值
constexpr uint64_t FNV_PRIME_64 = 1099511628211ULL;               // FNV-1a 64 位素数
constexpr uint32_t CRC32_INIT_MASK = 0xFFFFFFFF;   // CRC32 初始/最终异或掩码
constexpr uint32_t CRC32_POLYNOMIAL = 0xEDB88320;   // CRC32 多项式
constexpr uint32_t CRC32_BITS = 8;                  // CRC32 每字节处理的比特数
// UTF-8 编码相关掩码与标记
constexpr uint8_t UTF8_ONE_BYTE_MAX = 0x80;        // 单字节序列上界
constexpr uint8_t UTF8_TWO_BYTE_MASK = 0xE0;
constexpr uint8_t UTF8_TWO_BYTE_TAG = 0xC0;
constexpr uint8_t UTF8_THREE_BYTE_MASK = 0xF0;
constexpr uint8_t UTF8_THREE_BYTE_TAG = 0xE0;
constexpr uint8_t UTF8_FOUR_BYTE_MASK = 0xF8;
constexpr uint8_t UTF8_FOUR_BYTE_TAG = 0xF0;
constexpr uint8_t UTF8_CONTINUATION_MASK = 0xC0;
constexpr uint8_t UTF8_CONTINUATION_TAG = 0x80;
// Latin-1 重音字符范围
constexpr uint8_t LATIN1_ACCENT_START = 0xC0;
constexpr uint8_t LATIN1_ACCENT_END = 0xFF;
// 数值/时间格式化
constexpr uint64_t BYTES_PER_KB = 1024;
constexpr int32_t BYTES_UNIT_MAX_INDEX = 4;
constexpr int64_t MILLIS_PER_SECOND = 1000;
constexpr int64_t SECONDS_PER_MINUTE = 60;
constexpr int64_t MINUTES_PER_HOUR = 60;
constexpr int64_t HOURS_PER_DAY = 24;
constexpr int32_t TIMESTAMP_BUFFER_SIZE = 256;
constexpr double BYTE_ENTROPY_DIVISOR = 8.0;  // 每字节最大信息熵（bit）
constexpr double MIN_COMPRESSION_RATIO = 0.1;
constexpr double MAX_COMPRESSION_RATIO = 1.0;
// UUID v4 相关
constexpr int UUID_HEX_RADIX_MAX = 16;  // 十六进制随机数上界（0-15）
constexpr int UUID_GROUP_FIRST = 8;
constexpr int UUID_GROUP_SECOND = 4;
constexpr int UUID_GROUP_THIRD = 3;
constexpr int UUID_GROUP_FIFTH = 12;
// 十六进制输出宽度
constexpr int UNICODE_ESCAPE_HEX_WIDTH = 4;   // \uXXXX 转义序列的十六进制位数
constexpr int HASH32_HEX_WIDTH = 8;            // 32 位哈希的十六进制位数
constexpr int HASH64_HEX_WIDTH = 16;           // 64 位哈希的十六进制位数

// 处理单个 JSON 转义字符输出，命中转义分支返回 true
bool AppendEscapedJsonChar(std::ostringstream& oss, char c)
{
    switch (c) {
        case '"':
            oss << "\\\"";
            return true;
        case '\\':
            oss << "\\\\";
            return true;
        case '/':
            oss << "\\/";
            return true;
        case '\b':
            oss << "\\b";
            return true;
        case '\f':
            oss << "\\f";
            return true;
        case '\n':
            oss << "\\n";
            return true;
        case '\r':
            oss << "\\r";
            return true;
        case '\t':
            oss << "\\t";
            return true;
        default:
            return false;
    }
}

// 处理单个 JSON 反转义字符输出，advance 为额外推进的字符数
bool AppendUnescapedJsonChar(std::string& result, char c, size_t& advance)
{
    advance = 0;
    switch (c) {
        case '"':
            result += '"';
            return true;
        case '\\':
            result += '\\';
            return true;
        case '/':
            result += '/';
            return true;
        case 'b':
            result += '\b';
            return true;
        case 'f':
            result += '\f';
            return true;
        case 'n':
            result += '\n';
            return true;
        case 'r':
            result += '\r';
            return true;
        case 't':
            result += '\t';
            return true;
        default:
            return false;
    }
}

// 替换字符串中指定 XML 实体为单字符
void ReplaceEntity(std::string& str, const std::string& entity, const std::string& replacement)
{
    constexpr size_t ENTITY_ADVANCE = 1; // 替换后从首字符继续扫描，避免重叠
    size_t pos = 0;
    while ((pos = str.find(entity, pos)) != std::string::npos) {
        str.replace(pos, entity.length(), replacement);
        pos += ENTITY_ADVANCE;
    }
}

// 返回单个 Latin-1 字符去重音后的 ASCII 字符（无映射返回 0）
char DiacriticToAscii(unsigned char uc)
{
    switch (uc) {
        case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC4: case 0xC5:
            return 'A';
        case 0xE0: case 0xE1: case 0xE2: case 0xE3: case 0xE4: case 0xE5:
            return 'a';
        case 0xC8: case 0xC9: case 0xCA: case 0xCB:
            return 'E';
        case 0xE8: case 0xE9: case 0xEA: case 0xEB:
            return 'e';
        default:
            return 0;
    }
}

// 计算给定 UTF-8 起始字节对应的字节长度（1-4），非法返回 1
uint8_t GetUTF8ByteLen(unsigned char c)
{
    if (c < UTF8_ONE_BYTE_MAX) {
        return 1;
    }
    if ((c & UTF8_TWO_BYTE_MASK) == UTF8_TWO_BYTE_TAG) {
        return 2;
    }
    if ((c & UTF8_THREE_BYTE_MASK) == UTF8_THREE_BYTE_TAG) {
        return 3;
    }
    if ((c & UTF8_FOUR_BYTE_MASK) == UTF8_FOUR_BYTE_TAG) {
        return 4;
    }
    return 1;
}
} // namespace

// StringConversionResult implementation
StringConversionResult StringConversionResult::Success(const std::string& value, uint32_t bytes, uint32_t chars)
{
    StringConversionResult result;
    result.success = true;
    result.result = value;
    result.bytesProcessed = bytes;
    result.charactersProcessed = chars;
    return result;
}

StringConversionResult StringConversionResult::Failure(const std::string& error)
{
    StringConversionResult result;
    result.success = false;
    result.errorMessage = error;
    return result;
}

// SelectionStringConverter implementation
SelectionStringConverter& SelectionStringConverter::GetInstance()
{
    static SelectionStringConverter instance;
    return instance;
}

StringConversionResult SelectionStringConverter::ConvertEncoding(const std::string& input,
                                                                StringEncoding fromEncoding,
                                                                StringEncoding toEncoding)
{
    // Simplified encoding conversion
    // In production, you'd use a proper encoding library like ICU

    if (fromEncoding == toEncoding) {
        return StringConversionResult::Success(input, input.length(), input.length());
    }

    std::string result = input;
    uint32_t bytesProcessed = input.length();
    uint32_t charsProcessed = input.length();

    // Basic UTF-8 validation
    if (fromEncoding == StringEncoding::UTF8 && !IsValidUTF8(input)) {
        return StringConversionResult::Failure("Invalid UTF-8 input");
    }

    // Conversion logic would go here
    return StringConversionResult::Success(result, bytesProcessed, charsProcessed);
}

StringConversionResult SelectionStringConverter::ConvertCase(const std::string& input,
                                                            CaseConversionType caseType)
{
    switch (caseType) {
        case CaseConversionType::TO_UPPER:
            return StringConversionResult::Success(ToUpperCase(input));
        case CaseConversionType::TO_LOWER:
            return StringConversionResult::Success(ToLowerCase(input));
        case CaseConversionType::TO_TITLE:
            return StringConversionResult::Success(ToTitleCase(input));
        case CaseConversionType::TO_CAMEL:
            return StringConversionResult::Success(ToCamelCase(input));
        case CaseConversionType::TO_PASCAL:
            return StringConversionResult::Success(ToPascalCase(input));
        case CaseConversionType::TO_SNAKE:
            return StringConversionResult::Success(ToSnakeCase(input));
        case CaseConversionType::TO_KEBAB:
            return StringConversionResult::Success(ToKebabCase(input));
        default:
            return StringConversionResult::Failure("Unknown case conversion type");
    }
}

StringConversionResult SelectionStringConverter::Trim(const std::string& input, TrimType trimType)
{
    std::string result = input;

    switch (trimType) {
        case TrimType::LEFT:
            result.erase(result.begin(), std::find_if(result.begin(), result.end(),
                       [](int c) { return !std::isspace(c); }));
            break;
        case TrimType::RIGHT:
            result.erase(std::find_if(result.rbegin(), result.rend(),
                       [](int c) { return !std::isspace(c); }).base(), result.end());
            break;
        case TrimType::BOTH:
            result.erase(result.begin(), std::find_if(result.begin(), result.end(),
                       [](int c) { return !std::isspace(c); }));
            result.erase(std::find_if(result.rbegin(), result.rend(),
                       [](int c) { return !std::isspace(c); }).base(), result.end());
            break;
        default:
            break;
    }

    return StringConversionResult::Success(result);
}

StringConversionResult SelectionStringConverter::NormalizeWhitespace(const std::string& input)
{
    std::string result;
    bool inWhitespace = false;

    for (char c : input) {
        if (std::isspace(c)) {
            if (!inWhitespace) {
                result += ' ';
                inWhitespace = true;
            }
        } else {
            result += c;
            inWhitespace = false;
        }
    }

    // Trim trailing whitespace
    if (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }

    return StringConversionResult::Success(result);
}

StringConversionResult SelectionStringConverter::RemoveDiacritics(const std::string& input)
{
    std::string result;

    for (char c : input) {
        unsigned char uc = static_cast<unsigned char>(c);
        // 处理常见 Latin-1 重音字符
        if (uc >= LATIN1_ACCENT_START && uc <= LATIN1_ACCENT_END) {
            char ascii = DiacriticToAscii(uc);
            result += (ascii != 0) ? ascii : c;
        } else {
            result += c;
        }
    }

    return StringConversionResult::Success(result);
}

bool SelectionStringConverter::IsValidUTF8(const std::string& str)
{
    size_t i = 0;
    while (i < str.length()) {
        unsigned char c = static_cast<unsigned char>(str[i]);

        if (c < UTF8_ONE_BYTE_MAX) {
            // 单字节字符
            i++;
        } else if ((c & UTF8_TWO_BYTE_MASK) == UTF8_TWO_BYTE_TAG) {
            // 2 字节序列
            if (i + 1 >= str.length() ||
                (static_cast<unsigned char>(str[i + 1]) & UTF8_CONTINUATION_MASK) != UTF8_CONTINUATION_TAG) {
                return false;
            }
            i += 2;
        } else if ((c & UTF8_THREE_BYTE_MASK) == UTF8_THREE_BYTE_TAG) {
            // 3 字节序列
            if (i + 2 >= str.length() ||
                (static_cast<unsigned char>(str[i + 1]) & UTF8_CONTINUATION_MASK) != UTF8_CONTINUATION_TAG ||
                (static_cast<unsigned char>(str[i + 2]) & UTF8_CONTINUATION_MASK) != UTF8_CONTINUATION_TAG) {
                return false;
            }
            i += 3;
        } else if ((c & UTF8_FOUR_BYTE_MASK) == UTF8_FOUR_BYTE_TAG) {
            // 4 字节序列
            if (i + 3 >= str.length() ||
                (static_cast<unsigned char>(str[i + 1]) & UTF8_CONTINUATION_MASK) != UTF8_CONTINUATION_TAG ||
                (static_cast<unsigned char>(str[i + 2]) & UTF8_CONTINUATION_MASK) != UTF8_CONTINUATION_TAG ||
                (static_cast<unsigned char>(str[i + 3]) & UTF8_CONTINUATION_MASK) != UTF8_CONTINUATION_TAG) {
                return false;
            }
            i += 4;
        } else {
            return false;
        }
    }
    return true;
}

uint32_t SelectionStringConverter::GetUTF8Length(const std::string& str)
{
    uint32_t length = 0;
    size_t i = 0;

    while (i < str.length()) {
        unsigned char c = static_cast<unsigned char>(str[i]);
        i += GetUTF8ByteLen(c);
        length++;
    }

    return length;
}

std::vector<std::string> SelectionStringConverter::SplitUTF8(const std::string& str)
{
    std::vector<std::string> result;
    size_t i = 0;

    while (i < str.length()) {
        unsigned char c = static_cast<unsigned char>(str[i]);
        size_t charLen = GetUTF8ByteLen(c);

        if (i + charLen <= str.length()) {
            result.push_back(str.substr(i, charLen));
        }

        i += charLen;
    }

    return result;
}

std::string SelectionStringConverter::EscapeJSON(const std::string& str)
{
    std::ostringstream oss;
    for (char c : str) {
        if (AppendEscapedJsonChar(oss, c)) {
            continue;
        }
        if (static_cast<unsigned char>(c) < ASCII_CONTROL_MAX) {
            // 控制字符以 \uXXXX 形式输出
            oss << "\\u" << std::hex << std::setw(UNICODE_ESCAPE_HEX_WIDTH) << std::setfill('0')
                << static_cast<int>(c);
        } else {
            oss << c;
        }
    }
    return oss.str();
}

std::string SelectionStringConverter::UnescapeJSON(const std::string& str)
{
    std::string result;
    bool escape = false;

    for (size_t i = 0; i < str.length(); ++i) {
        if (!escape) {
            if (str[i] == '\\') {
                escape = true;
            } else {
                result += str[i];
            }
            continue;
        }
        // 处理转义字符
        escape = false;
        size_t advance = 0;
        if (AppendUnescapedJsonChar(result, str[i], advance)) {
            i += advance;
            continue;
        }
        if (str[i] == 'u') {
            // Unicode 转义，简化处理：跳过后续 4 个十六进制字符
            constexpr size_t UNICODE_ESCAPE_TAIL = 4;
            if (i + UNICODE_ESCAPE_TAIL < str.length()) {
                i += UNICODE_ESCAPE_TAIL;
            }
            continue;
        }
        result += str[i];
    }

    return result;
}

std::string SelectionStringConverter::EscapeXML(const std::string& str)
{
    std::ostringstream oss;
    for (char c : str) {
        switch (c) {
            case '<':
                oss << "&lt;";
                break;
            case '>':
                oss << "&gt;";
                break;
            case '&':
                oss << "&amp;";
                break;
            case '\'':
                oss << "&apos;";
                break;
            case '"':
                oss << "&quot;";
                break;
            default:
                oss << c;
                break;
        }
    }
    return oss.str();
}

std::string SelectionStringConverter::UnescapeXML(const std::string& str)
{
    std::string result = str;
    // 替换 XML 预定义实体
    ReplaceEntity(result, "&lt;", "<");
    ReplaceEntity(result, "&gt;", ">");
    ReplaceEntity(result, "&amp;", "&");
    ReplaceEntity(result, "&apos;", "'");
    ReplaceEntity(result, "&quot;", "\"");
    return result;
}

std::string SelectionStringConverter::EscapeCSV(const std::string& str)
{
    if (str.find(',') != std::string::npos || str.find('"') != std::string::npos ||
        str.find('\n') != std::string::npos) {
        std::string escaped = "\"";
        for (char c : str) {
            if (c == '"') {
                escaped += "\"\"";
            } else {
                escaped += c;
            }
        }
        escaped += "\"";
        return escaped;
    }
    return str;
}

std::string SelectionStringConverter::UnescapeCSV(const std::string& str)
{
    if (str.length() >= 2 && str.front() == '"' && str.back() == '"') {
        std::string result = str.substr(1, str.length() - 2);
        std::string unescaped;

        for (size_t i = 0; i < result.length(); ++i) {
            if (result[i] == '"' && i + 1 < result.length() && result[i + 1] == '"') {
                unescaped += '"';
                i++; // Skip next quote
            } else {
                unescaped += result[i];
            }
        }

        return unescaped;
    }
    return str;
}

std::string SelectionStringConverter::ToUpperCase(const std::string& str)
{
    std::string result;
    for (char c : str) {
        result += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return result;
}

std::string SelectionStringConverter::ToLowerCase(const std::string& str)
{
    std::string result;
    for (char c : str) {
        result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
}

std::string SelectionStringConverter::ToTitleCase(const std::string& str)
{
    std::string result;
    bool capitalizeNext = true;

    for (char c : str) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            result += c;
            capitalizeNext = true;
        } else if (capitalizeNext) {
            result += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            capitalizeNext = false;
        } else {
            result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }

    return result;
}

std::string SelectionStringConverter::ToCamelCase(const std::string& str)
{
    std::string result;
    bool capitalizeNext = false;

    for (char c : str) {
        if (c == '_' || c == '-' || c == ' ') {
            capitalizeNext = true;
        } else if (capitalizeNext) {
            result += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            capitalizeNext = false;
        } else {
            result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }

    return result;
}

std::string SelectionStringConverter::ToPascalCase(const std::string& str)
{
    std::string result;
    bool capitalizeNext = true;

    for (char c : str) {
        if (c == '_' || c == '-' || c == ' ') {
            capitalizeNext = true;
        } else if (capitalizeNext) {
            result += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            capitalizeNext = false;
        } else {
            result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }

    return result;
}

std::string SelectionStringConverter::ToSnakeCase(const std::string& str)
{
    std::string result;

    for (char c : str) {
        if (std::isupper(static_cast<unsigned char>(c))) {
            if (!result.empty() && result.back() != '_') {
                result += '_';
            }
            result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        } else if (c == '-' || c == ' ') {
            result += '_';
        } else {
            result += c;
        }
    }

    return result;
}

std::string SelectionStringConverter::ToKebabCase(const std::string& str)
{
    std::string result;

    for (char c : str) {
        if (std::isupper(static_cast<unsigned char>(c))) {
            if (!result.empty() && result.back() != '-') {
                result += '-';
            }
            result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        } else if (c == '_' || c == ' ') {
            result += '-';
        } else {
            result += c;
        }
    }

    return result;
}

// StringValidator implementation
StringValidator::ValidationResult StringValidator::ValidateUTF8(const std::string& str)
{
    ValidationResult result;

    if (!SelectionStringConverter::IsValidUTF8(str)) {
        result.AddError("Invalid UTF-8 encoding");
    }

    return result;
}

StringValidator::ValidationResult StringValidator::ValidateASCII(const std::string& str)
{
    ValidationResult result;

    for (char c : str) {
        if (static_cast<unsigned char>(c) > ASCII_MAX) {
            result.AddError("Non-ASCII character found");
            break;
        }
    }

    return result;
}

StringValidator::ValidationResult StringValidator::ValidateLength(const std::string& str,
                                                                  uint32_t minLength,
                                                                  uint32_t maxLength)
{
    ValidationResult result;

    uint32_t length = str.length();
    if (length < minLength) {
        result.AddError("String length is less than minimum required");
    }
    if (length > maxLength) {
        result.AddError("String length exceeds maximum allowed");
    }

    return result;
}

StringValidator::ValidationResult StringValidator::ValidatePattern(const std::string& str,
                                                                   const std::string& pattern)
{
    ValidationResult result;

    try {
        std::regex regex(pattern);
        if (!std::regex_match(str, regex)) {
            result.AddError("String does not match required pattern");
        }
    } catch (const std::regex_error& e) {
        result.AddError(std::string("Invalid pattern: ") + e.what());
    }

    return result;
}

StringValidator::ValidationResult StringValidator::ValidateNoControlChars(const std::string& str)
{
    ValidationResult result;

    for (char c : str) {
        if (iscntrl(static_cast<unsigned char>(c)) && c != '\t' && c != '\n' && c != '\r') {
            result.AddWarning("String contains control characters");
            break;
        }
    }

    return result;
}

StringValidator::ValidationResult StringValidator::ValidatePrintable(const std::string& str)
{
    ValidationResult result;

    for (char c : str) {
        if (!isprint(static_cast<unsigned char>(c)) && c != '\t' && c != '\n' && c != '\r') {
            result.AddError("String contains non-printable characters");
            break;
        }
    }

    return result;
}

// StringTransformPipeline implementation
StringTransformPipeline::StringTransformPipeline()
{
}

StringTransformPipeline& StringTransformPipeline::AddTransform(TransformFunc transform)
{
    transforms_.push_back(transform);
    return *this;
}

StringTransformPipeline& StringTransformPipeline::AddCaseConversion(CaseConversionType caseType)
{
    transforms_.push_back([caseType](const std::string& str) {
        auto& converter = SelectionStringConverter::GetInstance();
        auto result = converter.ConvertCase(str, caseType);
        return result.success ? result.result : str;
    });
    return *this;
}

StringTransformPipeline& StringTransformPipeline::AddTrim(TrimType trimType)
{
    transforms_.push_back([trimType](const std::string& str) {
        auto& converter = SelectionStringConverter::GetInstance();
        auto result = converter.Trim(str, trimType);
        return result.success ? result.result : str;
    });
    return *this;
}

StringTransformPipeline& StringTransformPipeline::AddNormalization()
{
    transforms_.push_back([](const std::string& str) {
        auto& converter = SelectionStringConverter::GetInstance();
        auto result = converter.NormalizeWhitespace(str);
        return result.success ? result.result : str;
    });
    return *this;
}

StringTransformPipeline& StringTransformPipeline::AddDiacriticRemoval()
{
    transforms_.push_back([](const std::string& str) {
        auto& converter = SelectionStringConverter::GetInstance();
        auto result = converter.RemoveDiacritics(str);
        return result.success ? result.result : str;
    });
    return *this;
}

StringConversionResult StringTransformPipeline::Execute(const std::string& input) const
{
    std::string current = input;

    for (const auto& transform : transforms_) {
        current = transform(current);
    }

    return StringConversionResult::Success(current);
}

void StringTransformPipeline::Clear()
{
    transforms_.clear();
}

// StringDiffUtil implementation
uint32_t StringDiffUtil::ComputeEditDistance(const std::string& str1, const std::string& str2)
{
    const size_t m = str1.length();
    const size_t n = str2.length();

    std::vector<std::vector<uint32_t>> dp(m + 1, std::vector<uint32_t>(n + 1, 0));

    for (size_t i = 0; i <= m; ++i) {
        dp[i][0] = i;
    }
    for (size_t j = 0; j <= n; ++j) {
        dp[0][j] = j;
    }

    for (size_t i = 1; i <= m; ++i) {
        for (size_t j = 1; j <= n; ++j) {
            if (str1[i - 1] == str2[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1];
            } else {
                dp[i][j] = 1 + std::min({dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]});
            }
        }
    }

    return dp[m][n];
}

double StringDiffUtil::ComputeSimilarity(const std::string& str1, const std::string& str2)
{
    uint32_t distance = ComputeEditDistance(str1, str2);
    uint32_t maxLen = std::max(str1.length(), str2.length());

    if (maxLen == 0) {
        return 1.0;
    }

    return 1.0 - (static_cast<double>(distance) / static_cast<double>(maxLen));
}

StringDiffUtil::DiffResult StringDiffUtil::ComputeDiff(const std::string& str1, const std::string& str2)
{
    DiffResult result;
    result.editDistance = ComputeEditDistance(str1, str2);
    result.similarityRatio = ComputeSimilarity(str1, str2);

    // Detailed diff calculation would go here
    // This is a simplified version

    return result;
}

std::string StringDiffUtil::CreatePatch(const std::string& original, const std::string& modified)
{
    // Simplified patch creation
    std::ostringstream oss;
    oss << "--- original\n";
    oss << "+++ modified\n";
    oss << "@@ 1," << original.length() << " +1," << modified.length() << " @@\n";

    // Diff algorithm would go here

    return oss.str();
}

std::string StringDiffUtil::ApplyPatch(const std::string& original, const std::string& patch)
{
    // Simplified patch application
    // In production, you'd use a proper patch library

    return original;
}

// StringHashUtil implementation
uint32_t StringHashUtil::ComputeHash(const std::string& str)
{
    uint32_t hash = DJB2_HASH_OFFSET;

    for (char c : str) {
        // DJB2: hash * 33 + c
        hash = ((hash << 5) + hash) + static_cast<unsigned char>(c);
    }

    return hash;
}

uint64_t StringHashUtil::ComputeHash64(const std::string& str)
{
    uint64_t hash = FNV_OFFSET_BASIS_64;

    for (char c : str) {
        hash ^= static_cast<unsigned char>(c);
        hash *= FNV_PRIME_64;
    }

    return hash;
}

std::string StringHashUtil::ComputeMD5(const std::string& str)
{
    // In production, use a proper crypto library
    std::ostringstream oss;
    uint32_t hash = ComputeHash(str);
    oss << std::hex << std::setw(HASH32_HEX_WIDTH) << std::setfill('0') << hash;
    return oss.str();
}

std::string StringHashUtil::ComputeSHA256(const std::string& str)
{
    // In production, use a proper crypto library
    std::ostringstream oss;
    uint64_t hash = ComputeHash64(str);
    oss << std::hex << std::setw(HASH64_HEX_WIDTH) << std::setfill('0') << hash;
    return oss.str();
}

std::string StringHashUtil::ComputeCRC32(const std::string& str)
{
    // 简化版 CRC32
    uint32_t crc = CRC32_INIT_MASK;

    for (char c : str) {
        crc ^= static_cast<unsigned char>(c);
        for (uint32_t j = 0; j < CRC32_BITS; ++j) {
            crc = (crc >> 1) ^ ((crc & 1) ? CRC32_POLYNOMIAL : 0);
        }
    }

    crc ^= CRC32_INIT_MASK;
    return std::to_string(crc);
}

std::string StringHashUtil::GenerateUUID()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    // UUID v4 每位为 0-15 的十六进制字符
    static std::uniform_int_distribution<> dis(0, UUID_HEX_RADIX_MAX - 1);
    // UUID v4 变体字段首位取值 8-11（1000-1011），用 8 + (0-3) 生成
    constexpr int UUID_VARIANT_BASE = 8;
    constexpr int UUID_VARIANT_MASK = 3;

    std::ostringstream oss;
    oss << std::hex;

    for (int i = 0; i < UUID_GROUP_FIRST; ++i) {
        oss << dis(gen);
    }
    oss << "-";
    for (int i = 0; i < UUID_GROUP_SECOND; ++i) {
        oss << dis(gen);
    }
    oss << "-4"; // UUID v4 版本位
    for (int i = 0; i < UUID_GROUP_THIRD; ++i) {
        oss << dis(gen);
    }
    oss << "-";
    oss << (UUID_VARIANT_BASE + (dis(gen) & UUID_VARIANT_MASK)); // 变体位
    for (int i = 0; i < UUID_GROUP_THIRD; ++i) {
        oss << dis(gen);
    }
    oss << "-";
    for (int i = 0; i < UUID_GROUP_FIFTH; ++i) {
        oss << dis(gen);
    }

    return oss.str();
}

std::string StringHashUtil::GenerateUUIDFromString(const std::string& str)
{
    // 由哈希派生 UUID 时各字段在十六进制串中的起止位置
    constexpr size_t UUID_HEX_TOTAL_LEN = 16;
    constexpr size_t POS_FIRST_START = 0;
    constexpr size_t POS_SECOND_START = 8;
    constexpr size_t LEN_SECOND = 4;
    constexpr size_t POS_THIRD_START = 12;
    constexpr size_t LEN_THIRD = 3;
    constexpr size_t POS_FOURTH_START = 15;
    constexpr size_t LEN_FOURTH = 3;
    constexpr size_t POS_FIFTH_START = 16;
    constexpr size_t LEN_FIFTH = 12;

    uint64_t hash = ComputeHash64(str);

    std::ostringstream oss;
    oss << std::hex << std::setw(UUID_HEX_TOTAL_LEN) << std::setfill('0') << hash;
    std::string hex = oss.str();

    // 按 UUID 格式拼接（版本位固定 4，变体位固定 a）
    return hex.substr(POS_FIRST_START, UUID_GROUP_FIRST) + "-" +
           hex.substr(POS_SECOND_START, LEN_SECOND) + "-4" +
           hex.substr(POS_THIRD_START, LEN_THIRD) + "-a" +
           hex.substr(POS_FOURTH_START, LEN_FOURTH) + "-" +
           hex.substr(POS_FIFTH_START, LEN_FIFTH);
}

// StringCompressionUtil implementation
std::vector<uint8_t> StringCompressionUtil::CompressGZIP(const std::string& str)
{
    // In production, use a proper compression library like zlib
    std::vector<uint8_t> result(str.begin(), str.end());
    return result;
}

std::string StringCompressionUtil::DecompressGZIP(const std::vector<uint8_t>& data)
{
    // In production, use a proper compression library like zlib
    return std::string(data.begin(), data.end());
}

std::vector<uint8_t> StringCompressionUtil::CompressZLIB(const std::string& str)
{
    // In production, use a proper compression library like zlib
    std::vector<uint8_t> result(str.begin(), str.end());
    return result;
}

std::string StringCompressionUtil::DecompressZLIB(const std::vector<uint8_t>& data)
{
    // In production, use a proper compression library like zlib
    return std::string(data.begin(), data.end());
}

std::vector<uint8_t> StringCompressionUtil::CompressLZ4(const std::string& str)
{
    // In production, use a proper compression library like lz4
    std::vector<uint8_t> result(str.begin(), str.end());
    return result;
}

std::string StringCompressionUtil::DecompressLZ4(const std::vector<uint8_t>& data)
{
    // In production, use a proper compression library like lz4
    return std::string(data.begin(), data.end());
}

double StringCompressionUtil::EstimateCompressionRatio(const std::string& str)
{
    // Simple heuristic based on character repetition
    std::unordered_map<char, uint32_t> charCounts;

    for (char c : str) {
        charCounts[c]++;
    }

    if (charCounts.empty()) {
        return 1.0;
    }

    // 基于信息熵估算压缩率
    double entropy = 0.0;
    double len = static_cast<double>(str.length());

    for (const auto& pair : charCounts) {
        double prob = static_cast<double>(pair.second) / len;
        entropy -= prob * std::log2(prob);
    }

    // 估算压缩比 = 熵 / 每字节最大熵，限制在 [0.1, 1.0]
    return std::max(MIN_COMPRESSION_RATIO,
                    std::min(MAX_COMPRESSION_RATIO, entropy / BYTE_ENTROPY_DIVISOR));
}

// StringFormatter implementation
std::string StringFormatter::FormatBytes(uint64_t bytes)
{
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double value = static_cast<double>(bytes);

    while (value >= static_cast<double>(BYTES_PER_KB) && unitIndex < BYTES_UNIT_MAX_INDEX) {
        value /= static_cast<double>(BYTES_PER_KB);
        unitIndex++;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << value << " " << units[unitIndex];
    return oss.str();
}

std::string StringFormatter::FormatNumber(int64_t number, const std::string& locale)
{
    std::ostringstream oss;

    if (locale == "en_US") {
        oss << std::put_money(number);
    } else {
        oss << number;
    }

    return oss.str();
}

std::string StringFormatter::FormatDuration(int64_t milliseconds)
{
    int64_t seconds = milliseconds / MILLIS_PER_SECOND;
    int64_t minutes = seconds / SECONDS_PER_MINUTE;
    int64_t hours = minutes / MINUTES_PER_HOUR;
    int64_t days = hours / HOURS_PER_DAY;

    milliseconds %= MILLIS_PER_SECOND;
    seconds %= SECONDS_PER_MINUTE;
    minutes %= MINUTES_PER_HOUR;
    hours %= HOURS_PER_DAY;

    std::ostringstream oss;

    if (days > 0) {
        oss << days << "d ";
    }
    if (hours > 0 || days > 0) {
        oss << hours << "h ";
    }
    if (minutes > 0 || hours > 0 || days > 0) {
        oss << minutes << "m ";
    }
    oss << seconds << "s";

    return oss.str();
}

std::string StringFormatter::FormatTimestamp(int64_t timestamp, const std::string& format)
{
    // 简化时间戳格式化
    std::time_t time = timestamp / MILLIS_PER_SECOND;
    std::tm* tm = std::gmtime(&time);
    if (tm == nullptr) {
        return std::to_string(timestamp);
    }

    char buffer[TIMESTAMP_BUFFER_SIZE] = {0};
    std::strftime(buffer, sizeof(buffer), format.c_str(), tm);

    return std::string(buffer);
}

std::string StringFormatter::FormatPercentage(double value, uint32_t precision)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value << "%";
    return oss.str();
}

std::string StringFormatter::Truncate(const std::string& str, uint32_t maxLength, const std::string& ellipsis)
{
    if (str.length() <= maxLength) {
        return str;
    }

    return str.substr(0, maxLength - ellipsis.length()) + ellipsis;
}

std::string StringFormatter::PadLeft(const std::string& str, uint32_t width, char padChar)
{
    if (str.length() >= width) {
        return str;
    }

    return std::string(width - str.length(), padChar) + str;
}

std::string StringFormatter::PadRight(const std::string& str, uint32_t width, char padChar)
{
    if (str.length() >= width) {
        return str;
    }

    return str + std::string(width - str.length(), padChar);
}

std::string StringFormatter::Center(const std::string& str, uint32_t width, char padChar)
{
    if (str.length() >= width) {
        return str;
    }

    uint32_t totalPad = width - str.length();
    uint32_t leftPad = totalPad / 2;
    uint32_t rightPad = totalPad - leftPad;

    return std::string(leftPad, padChar) + str + std::string(rightPad, padChar);
}

} // namespace SelectionFwk
} // namespace OHOS
