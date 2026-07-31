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

#include "selection_type_converter.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <limits>

namespace OHOS {
namespace SelectionFwk {
namespace {
// 进制相关具名常量
constexpr size_t PREFIX_LEN_TWO = 2;   // "0x"/"0b" 等两位前缀长度
constexpr int RADIX_HEX = 16;
constexpr int RADIX_OCTAL = 8;
constexpr int RADIX_BINARY = 2;
constexpr int RADIX_DECIMAL = 10;
constexpr double LOSSY_THRESHOLD_BASE = 10.0; // 损失阈值基数
// 类型检测置信度
constexpr double TYPE_CONFIDENCE_BOOL = 0.9;
constexpr double TYPE_CONFIDENCE_INT = 0.95;
constexpr double TYPE_CONFIDENCE_DOUBLE = 0.85;
constexpr double TYPE_CONFIDENCE_STRING = 0.7;

// 尝试按进制前缀（0x/0/0b）解析为整数，命中返回 true
bool ConvertByBasePrefix(const std::string& trimmed, const NumericConversionOptions& options,
                         int64_t& result)
{
    if (options.allowHexPrefix && trimmed.length() > PREFIX_LEN_TWO &&
        trimmed[0] == '0' && (trimmed[1] == 'x' || trimmed[1] == 'X')) {
        result = std::stoll(trimmed.substr(PREFIX_LEN_TWO), nullptr, RADIX_HEX);
        return true;
    }
    if (options.allowOctalPrefix && trimmed.length() > 1 && trimmed[0] == '0') {
        result = std::stoll(trimmed, nullptr, RADIX_OCTAL);
        return true;
    }
    if (options.allowBinaryPrefix && trimmed.length() > PREFIX_LEN_TWO &&
        trimmed[0] == '0' && (trimmed[1] == 'b' || trimmed[1] == 'B')) {
        result = std::stoll(trimmed.substr(PREFIX_LEN_TWO), nullptr, RADIX_BINARY);
        return true;
    }
    return false;
}

// 规整字符串以适配十进制浮点解析（替换小数/千分位分隔符）
std::string NormalizeForDouble(const std::string& trimmed, const NumericConversionOptions& options)
{
    std::string processStr = trimmed;
    // 按需替换小数分隔符
    if (options.decimalSeparator != '.') {
        std::replace(processStr.begin(), processStr.end(), options.decimalSeparator, '.');
    }
    // 移除千分位分隔符
    if (options.thousandsSeparator != '\0') {
        processStr.erase(std::remove(processStr.begin(), processStr.end(),
                                     options.thousandsSeparator), processStr.end());
    }
    return processStr;
}

// 尝试将字符串值转换为 NUMERIC，失败返回 nullopt
bool TryConvertVariantToInt(const std::string& str, int64_t& out)
{
    auto result = SelectionTypeConverter::GetInstance().ConvertToInt(str);
    if (!result.success) {
        return false;
    }
    out = result.GetInt();
    return true;
}

// 尝试将变体值转换为 STRING
bool TryConvertVariantToString(const VariantValue::ValueType& value,
                               std::string& out)
{
    auto& converter = SelectionTypeConverter::GetInstance();
    if (std::holds_alternative<int64_t>(value)) {
        auto result = converter.ConvertToString(std::get<int64_t>(value));
        out = result.success ? result.GetString() : "";
        return result.success;
    }
    if (std::holds_alternative<bool>(value)) {
        auto result = converter.ConvertToString(std::get<bool>(value));
        out = result.success ? result.GetString() : "";
        return result.success;
    }
    return false;
}
} // namespace

// TypeConversionResult implementation
template<typename T>
TypeConversionResult TypeConversionResult::Success(T&& val, TypeCategory type)
{
    TypeConversionResult result;
    result.success = true;
    result.value = std::forward<T>(val);
    result.targetType = type;
    return result;
}

TypeConversionResult TypeConversionResult::Failure(const std::string& error)
{
    TypeConversionResult result;
    result.success = false;
    result.errorMessage = error;
    return result;
}

int64_t TypeConversionResult::GetInt() const
{
    if (std::holds_alternative<int64_t>(value)) {
        return std::get<int64_t>(value);
    }
    return 0;
}

double TypeConversionResult::GetDouble() const
{
    if (std::holds_alternative<double>(value)) {
        return std::get<double>(value);
    }
    return 0.0;
}

std::string TypeConversionResult::GetString() const
{
    if (std::holds_alternative<std::string>(value)) {
        return std::get<std::string>(value);
    }
    return "";
}

bool TypeConversionResult::GetBool() const
{
    if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value);
    }
    return false;
}

// SelectionTypeConverter implementation
SelectionTypeConverter& SelectionTypeConverter::GetInstance()
{
    static SelectionTypeConverter instance;
    return instance;
}

TypeConversionResult SelectionTypeConverter::ConvertToInt(const std::string& value,
                                                        const NumericConversionOptions& options)
{
    std::string trimmed = TrimWhitespace(value);

    if (trimmed.empty()) {
        return TypeConversionResult::Failure("Cannot convert empty string to int");
    }

    try {
        int64_t result = 0;

        if (ConvertByBasePrefix(trimmed, options, result)) {
            return TypeConversionResult::Success(result, TypeCategory::NUMERIC);
        }

        // 十进制解析
        size_t pos = 0;
        result = std::stoll(trimmed, &pos, RADIX_DECIMAL);

        // 存在小数部分时按需舍入
        if (options.roundResult && pos < trimmed.length()) {
            double doubleValue = std::stod(trimmed);
            result = static_cast<int64_t>(std::round(doubleValue));
        }

        return TypeConversionResult::Success(result, TypeCategory::NUMERIC);
    } catch (const std::exception& e) {
        return TypeConversionResult::Failure(std::string("Failed to convert to int: ") + e.what());
    }
}

TypeConversionResult SelectionTypeConverter::ConvertToDouble(const std::string& value,
                                                             const NumericConversionOptions& options)
{
    std::string trimmed = TrimWhitespace(value);

    if (trimmed.empty()) {
        return TypeConversionResult::Failure("Cannot convert empty string to double");
    }

    try {
        std::string processStr = NormalizeForDouble(trimmed, options);
        double result = std::stod(processStr);
        return TypeConversionResult::Success(result, TypeCategory::NUMERIC);
    } catch (const std::exception& e) {
        return TypeConversionResult::Failure(std::string("Failed to convert to double: ") + e.what());
    }
}

TypeConversionResult SelectionTypeConverter::ConvertToBool(const std::string& value)
{
    std::string trimmed = TrimWhitespace(value);
    std::string lowerCase = trimmed;
    std::transform(lowerCase.begin(), lowerCase.end(), lowerCase.begin(),
                  [](unsigned char c) { return std::tolower(c); });

    if (lowerCase == "true" || lowerCase == "yes" || lowerCase == "1" ||
        lowerCase == "on" || lowerCase == "enabled") {
        return TypeConversionResult::Success(true, TypeCategory::BOOLEAN);
    } else if (lowerCase == "false" || lowerCase == "no" || lowerCase == "0" ||
               lowerCase == "off" || lowerCase == "disabled") {
        return TypeConversionResult::Success(false, TypeCategory::BOOLEAN);
    }

    return TypeConversionResult::Failure("Cannot convert to boolean");
}

TypeConversionResult SelectionTypeConverter::ConvertToString(int64_t value)
{
    return TypeConversionResult::Success(std::to_string(value), TypeCategory::STRING);
}

TypeConversionResult SelectionTypeConverter::ConvertToString(double value,
                                                             const NumericConversionOptions& options)
{
    std::ostringstream oss;

    if (options.precision > 0) {
        oss << std::fixed << std::setprecision(options.precision) << value;
    } else {
        oss << value;
    }

    return TypeConversionResult::Success(oss.str(), TypeCategory::STRING);
}

TypeConversionResult SelectionTypeConverter::ConvertToString(bool value)
{
    return TypeConversionResult::Success(value ? "true" : "false", TypeCategory::BOOLEAN);
}

TypeConversionResult SelectionTypeConverter::ConvertToHex(int64_t value, bool uppercase)
{
    std::ostringstream oss;
    oss << std::hex << (uppercase ? std::uppercase : std::nouppercase) << value;
    return TypeConversionResult::Success("0x" + oss.str(), TypeCategory::STRING);
}

TypeConversionResult SelectionTypeConverter::ConvertFromHex(const std::string& hex)
{
    std::string processStr = hex;

    // 去除可能的 0x 前缀
    if (processStr.length() > PREFIX_LEN_TWO && processStr[0] == '0' &&
        (processStr[1] == 'x' || processStr[1] == 'X')) {
        processStr = processStr.substr(PREFIX_LEN_TWO);
    }

    try {
        int64_t result = std::stoll(processStr, nullptr, RADIX_HEX);
        return TypeConversionResult::Success(result, TypeCategory::NUMERIC);
    } catch (const std::exception& e) {
        return TypeConversionResult::Failure(std::string("Failed to convert from hex: ") + e.what());
    }
}

TypeConversionResult SelectionTypeConverter::ConvertToOctal(int64_t value)
{
    std::ostringstream oss;
    oss << std::oct << value;
    return TypeConversionResult::Success("0" + oss.str(), TypeCategory::STRING);
}

TypeConversionResult SelectionTypeConverter::ConvertFromOctal(const std::string& octal)
{
    std::string processStr = octal;

    // 去除可能的前导 0
    if (!processStr.empty() && processStr[0] == '0') {
        processStr = processStr.substr(1);
    }

    try {
        int64_t result = std::stoll(processStr, nullptr, RADIX_OCTAL);
        return TypeConversionResult::Success(result, TypeCategory::NUMERIC);
    } catch (const std::exception& e) {
        return TypeConversionResult::Failure(std::string("Failed to convert from octal: ") + e.what());
    }
}

TypeConversionResult SelectionTypeConverter::ConvertToBinary(int64_t value)
{
    if (value == 0) {
        return TypeConversionResult::Success("0b0", TypeCategory::STRING);
    }

    std::string result;
    bool negative = value < 0;
    uint64_t absValue = negative ? -value : value;

    while (absValue > 0) {
        result = (absValue % RADIX_BINARY == 0 ? "0" : "1") + result;
        absValue /= RADIX_BINARY;
    }

    return TypeConversionResult::Success("0b" + (negative ? "-" : "") + result, TypeCategory::STRING);
}

TypeConversionResult SelectionTypeConverter::ConvertFromBinary(const std::string& binary)
{
    std::string processStr = binary;

    // 去除可能的 0b 前缀
    if (processStr.length() > PREFIX_LEN_TWO && processStr[0] == '0' &&
        (processStr[1] == 'b' || processStr[1] == 'B')) {
        processStr = processStr.substr(PREFIX_LEN_TWO);
    }

    try {
        int64_t result = std::stoll(processStr, nullptr, RADIX_BINARY);
        return TypeConversionResult::Success(result, TypeCategory::NUMERIC);
    } catch (const std::exception& e) {
        return TypeConversionResult::Failure(std::string("Failed to convert from binary: ") + e.what());
    }
}

TypeConversionResult SelectionTypeConverter::ConvertToBase64(const std::vector<uint8_t>& data)
{
    // Base64 encoding implementation would go here
    // For now, return a placeholder
    return TypeConversionResult::Success("", TypeCategory::STRING);
}

TypeConversionResult SelectionTypeConverter::ConvertFromBase64(const std::string& base64)
{
    // Base64 decoding implementation would go here
    // For now, return a placeholder
    std::vector<uint8_t> data;
    return TypeConversionResult::Success(0, TypeCategory::NUMERIC);
}

bool SelectionTypeConverter::IsNumeric(const std::string& str)
{
    if (str.empty()) {
        return false;
    }

    size_t start = 0;
    if (str[0] == '-' || str[0] == '+') {
        if (str.length() == 1) {
            return false;
        }
        start = 1;
    }

    bool hasDecimal = false;
    for (size_t i = start; i < str.length(); ++i) {
        if (str[i] == '.') {
            if (hasDecimal) {
                return false;
            }
            hasDecimal = true;
        } else if (!std::isdigit(static_cast<unsigned char>(str[i]))) {
            return false;
        }
    }

    return true;
}

bool SelectionTypeConverter::IsBoolean(const std::string& str)
{
    std::string lowerCase = str;
    std::transform(lowerCase.begin(), lowerCase.end(), lowerCase.begin(),
                  [](unsigned char c) { return std::tolower(c); });

    return lowerCase == "true" || lowerCase == "false" ||
           lowerCase == "yes" || lowerCase == "no" ||
           lowerCase == "1" || lowerCase == "0";
}

std::string SelectionTypeConverter::TrimWhitespace(const std::string& str)
{
    size_t start = 0;
    while (start < str.length() && std::isspace(static_cast<unsigned char>(str[start]))) {
        start++;
    }

    size_t end = str.length();
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
        end--;
    }

    return str.substr(start, end - start);
}

// TypeValidator implementation
TypeValidator::ValidationResult TypeValidator::ValidateInt(const std::string& value,
                                                          const NumericConversionOptions& options)
{
    ValidationResult result;

    if (value.empty()) {
        result.AddError("Empty string cannot be converted to int");
        return result;
    }

    auto converter = SelectionTypeConverter::GetInstance();
    auto conversionResult = converter.ConvertToInt(value, options);

    if (!conversionResult.success) {
        result.AddError("Invalid integer format: " + conversionResult.errorMessage);
    }

    return result;
}

TypeValidator::ValidationResult TypeValidator::ValidateDouble(const std::string& value,
                                                            const NumericConversionOptions& options)
{
    ValidationResult result;

    if (value.empty()) {
        result.AddError("Empty string cannot be converted to double");
        return result;
    }

    auto converter = SelectionTypeConverter::GetInstance();
    auto conversionResult = converter.ConvertToDouble(value, options);

    if (!conversionResult.success) {
        result.AddError("Invalid double format: " + conversionResult.errorMessage);
    }

    return result;
}

TypeValidator::ValidationResult TypeValidator::ValidateBool(const std::string& value)
{
    ValidationResult result;

    if (value.empty()) {
        result.AddError("Empty string cannot be converted to bool");
        return result;
    }

    auto converter = SelectionTypeConverter::GetInstance();
    auto conversionResult = converter.ConvertToBool(value);

    if (!conversionResult.success) {
        result.AddError("Invalid boolean format: " + conversionResult.errorMessage);
    }

    return result;
}

TypeValidator::ValidationResult TypeValidator::ValidateType(const std::string& value,
                                                           TypeCategory expectedType)
{
    switch (expectedType) {
        case TypeCategory::NUMERIC:
            return ValidateInt(value);
        case TypeCategory::BOOLEAN:
            return ValidateBool(value);
        default:
            ValidationResult result;
            result.AddWarning("Type validation not implemented for this category");
            return result;
    }
}

bool TypeValidator::CanConvertToInt(const std::string& value)
{
    return ValidateInt(value).isValid;
}

bool TypeValidator::CanConvertToDouble(const std::string& value)
{
    return ValidateDouble(value).isValid;
}

bool TypeValidator::CanConvertToBool(const std::string& value)
{
    return ValidateBool(value).isValid;
}

// AutoTypeDetector implementation
AutoTypeDetector::DetectionResult AutoTypeDetector::DetectType(const std::string& value)
{
    DetectionResult result;

    if (value.empty()) {
        result.detectedType = TypeCategory::STRING;
        return result;
    }

    // Check for boolean
    if (TypeValidator::CanConvertToBool(value)) {
        result.detectedType = TypeCategory::BOOLEAN;
        result.confidence = TYPE_CONFIDENCE_BOOL;
        result.typeProbabilities[TypeCategory::BOOLEAN] = TYPE_CONFIDENCE_BOOL;
        return result;
    }

    // Check for numeric
    if (TypeValidator::CanConvertToInt(value)) {
        result.detectedType = TypeCategory::NUMERIC;
        result.confidence = TYPE_CONFIDENCE_INT;
        result.typeProbabilities[TypeCategory::NUMERIC] = TYPE_CONFIDENCE_INT;
        return result;
    }

    // Check for double
    if (TypeValidator::CanConvertToDouble(value)) {
        result.detectedType = TypeCategory::NUMERIC;
        result.confidence = TYPE_CONFIDENCE_DOUBLE;
        result.typeProbabilities[TypeCategory::NUMERIC] = TYPE_CONFIDENCE_DOUBLE;
        return result;
    }

    // Default to string
    result.detectedType = TypeCategory::STRING;
    result.confidence = TYPE_CONFIDENCE_STRING;
    result.typeProbabilities[TypeCategory::STRING] = TYPE_CONFIDENCE_STRING;

    return result;
}

TypeCategory AutoTypeDetector::DetectMostLikelyType(const std::string& value)
{
    return DetectType(value).detectedType;
}

std::vector<TypeCategory> AutoTypeDetector::GetAllPossibleTypes(const std::string& value)
{
    std::vector<TypeCategory> possibleTypes;

    if (TypeValidator::CanConvertToBool(value)) {
        possibleTypes.push_back(TypeCategory::BOOLEAN);
    }

    if (TypeValidator::CanConvertToInt(value) || TypeValidator::CanConvertToDouble(value)) {
        possibleTypes.push_back(TypeCategory::NUMERIC);
    }

    // Always can be string
    possibleTypes.push_back(TypeCategory::STRING);

    return possibleTypes;
}

// TypeCastUtil implementation
std::optional<int32_t> TypeCastUtil::SafeInt32(int64_t value)
{
    if (value >= std::numeric_limits<int32_t>::min() &&
        value <= std::numeric_limits<int32_t>::max()) {
        return static_cast<int32_t>(value);
    }
    return std::nullopt;
}

std::optional<int16_t> TypeCastUtil::SafeInt16(int64_t value)
{
    if (value >= std::numeric_limits<int16_t>::min() &&
        value <= std::numeric_limits<int16_t>::max()) {
        return static_cast<int16_t>(value);
    }
    return std::nullopt;
}

std::optional<int8_t> TypeCastUtil::SafeInt8(int64_t value)
{
    if (value >= std::numeric_limits<int8_t>::min() &&
        value <= std::numeric_limits<int8_t>::max()) {
        return static_cast<int8_t>(value);
    }
    return std::nullopt;
}

std::optional<uint32_t> TypeCastUtil::SafeUInt32(int64_t value)
{
    if (value >= 0 && value <= std::numeric_limits<uint32_t>::max()) {
        return static_cast<uint32_t>(value);
    }
    return std::nullopt;
}

std::optional<uint16_t> TypeCastUtil::SafeUInt16(int64_t value)
{
    if (value >= 0 && value <= std::numeric_limits<uint16_t>::max()) {
        return static_cast<uint16_t>(value);
    }
    return std::nullopt;
}

std::optional<uint8_t> TypeCastUtil::SafeUInt8(int64_t value)
{
    if (value >= 0 && value <= std::numeric_limits<uint8_t>::max()) {
        return static_cast<uint8_t>(value);
    }
    return std::nullopt;
}

std::optional<float> TypeCastUtil::SafeFloat(double value)
{
    if (value >= std::numeric_limits<float>::lowest() &&
        value <= std::numeric_limits<float>::max()) {
        return static_cast<float>(value);
    }
    return std::nullopt;
}

bool TypeCastUtil::IsLossyConversion(double toInt)
{
    return toInt != std::floor(toInt);
}

bool TypeCastUtil::IsLossyConversion(int64_t toDouble)
{
    return static_cast<double>(toDouble) != static_cast<int64_t>(static_cast<double>(toDouble));
}

bool TypeCastUtil::IsLossyConversion(double source, double target, uint32_t precision)
{
    double diff = std::abs(source - target);
    double threshold = std::pow(LOSSY_THRESHOLD_BASE, -static_cast<int>(precision));
    return diff > threshold;
}

bool TypeCastUtil::IsInInt32Range(int64_t value)
{
    return value >= std::numeric_limits<int32_t>::min() &&
           value <= std::numeric_limits<int32_t>::max();
}

bool TypeCastUtil::IsInInt16Range(int64_t value)
{
    return value >= std::numeric_limits<int16_t>::min() &&
           value <= std::numeric_limits<int16_t>::max();
}

bool TypeCastUtil::IsInInt8Range(int64_t value)
{
    return value >= std::numeric_limits<int8_t>::min() &&
           value <= std::numeric_limits<int8_t>::max();
}

bool TypeCastUtil::IsInUInt32Range(int64_t value)
{
    return value >= 0 && value <= std::numeric_limits<uint32_t>::max();
}

bool TypeCastUtil::IsInUInt16Range(int64_t value)
{
    return value >= 0 && value <= std::numeric_limits<uint16_t>::max();
}

bool TypeCastUtil::IsInUInt8Range(int64_t value)
{
    return value >= 0 && value <= std::numeric_limits<uint8_t>::max();
}

// TypeInfoUtil implementation
TypeInfoUtil::TypeInfo TypeInfoUtil::GetTypeInfo(TypeCategory type)
{
    TypeInfo info;

    switch (type) {
        case TypeCategory::NUMERIC:
            info.typeName = "numeric";
            info.size = sizeof(int64_t);
            info.isSigned = true;
            info.isPrimitive = true;
            info.description = "Numeric integer value";
            break;

        case TypeCategory::STRING:
            info.typeName = "string";
            info.size = sizeof(std::string);
            info.isSigned = false;
            info.isPrimitive = false;
            info.description = "String text value";
            break;

        case TypeCategory::BOOLEAN:
            info.typeName = "boolean";
            info.size = sizeof(bool);
            info.isSigned = false;
            info.isPrimitive = true;
            info.description = "Boolean true/false value";
            break;

        default:
            info.typeName = "unknown";
            info.size = 0;
            info.isSigned = false;
            info.isPrimitive = false;
            info.description = "Unknown type";
            break;
    }

    return info;
}

std::vector<TypeInfoUtil::TypeInfo> TypeInfoUtil::GetAllTypeInfos()
{
    std::vector<TypeInfo> infos;

    for (uint32_t i = 0; i < static_cast<uint32_t>(TypeCategory::TYPE_CATEGORY_MAX); ++i) {
        infos.push_back(GetTypeInfo(static_cast<TypeCategory>(i)));
    }

    return infos;
}

std::string TypeInfoUtil::GetTypeDescription(TypeCategory type)
{
    return GetTypeInfo(type).description;
}

bool TypeInfoUtil::IsNumericType(TypeCategory type)
{
    return type == TypeCategory::NUMERIC;
}

bool TypeInfoUtil::IsStringType(TypeCategory type)
{
    return type == TypeCategory::STRING;
}

bool TypeInfoUtil::CanConvert(TypeCategory from, TypeCategory to)
{
    // Most conversions are possible
    return true;
}

// VariantValue implementation
VariantValue::VariantValue()
    : value_(std::string(""))
{
}

VariantValue::VariantValue(int64_t value)
    : value_(value)
{
}

VariantValue::VariantValue(double value)
    : value_(value)
{
}

VariantValue::VariantValue(const std::string& value)
    : value_(value)
{
}

VariantValue::VariantValue(bool value)
    : value_(value)
{
}

VariantValue::VariantValue(const std::vector<uint8_t>& value)
    : value_(value)
{
}

TypeCategory VariantValue::GetType() const
{
    if (std::holds_alternative<int64_t>(value_)) {
        return TypeCategory::NUMERIC;
    } else if (std::holds_alternative<double>(value_)) {
        return TypeCategory::NUMERIC;
    } else if (std::holds_alternative<std::string>(value_)) {
        return TypeCategory::STRING;
    } else if (std::holds_alternative<bool>(value_)) {
        return TypeCategory::BOOLEAN;
    } else if (std::holds_alternative<std::vector<uint8_t>>(value_)) {
        return TypeCategory::BINARY;
    }

    return TypeCategory::STRING;
}

VariantValue::ValueType VariantValue::GetValue() const
{
    return value_;
}

int64_t VariantValue::GetInt() const
{
    if (std::holds_alternative<int64_t>(value_)) {
        return std::get<int64_t>(value_);
    }
    return 0;
}

double VariantValue::GetDouble() const
{
    if (std::holds_alternative<double>(value_)) {
        return std::get<double>(value_);
    }
    return 0.0;
}

std::string VariantValue::GetString() const
{
    if (std::holds_alternative<std::string>(value_)) {
        return std::get<std::string>(value_);
    }
    return ToString();
}

bool VariantValue::GetBool() const
{
    if (std::holds_alternative<bool>(value_)) {
        return std::get<bool>(value_);
    }
    return false;
}

std::vector<uint8_t> VariantValue::GetBinary() const
{
    if (std::holds_alternative<std::vector<uint8_t>>(value_)) {
        return std::get<std::vector<uint8_t>>(value_);
    }
    return std::vector<uint8_t>();
}

void VariantValue::SetInt(int64_t value)
{
    value_ = value;
}

void VariantValue::SetDouble(double value)
{
    value_ = value;
}

void VariantValue::SetString(const std::string& value)
{
    value_ = value;
}

void VariantValue::SetBool(bool value)
{
    value_ = value;
}

void VariantValue::SetBinary(const std::vector<uint8_t>& value)
{
    value_ = value;
}

std::string VariantValue::ToString() const
{
    if (std::holds_alternative<int64_t>(value_)) {
        return std::to_string(std::get<int64_t>(value_));
    } else if (std::holds_alternative<double>(value_)) {
        return std::to_string(std::get<double>(value_));
    } else if (std::holds_alternative<std::string>(value_)) {
        return std::get<std::string>(value_);
    } else if (std::holds_alternative<bool>(value_)) {
        return std::get<bool>(value_) ? "true" : "false";
    } else if (std::holds_alternative<std::vector<uint8_t>>(value_)) {
        const auto& bytes = std::get<std::vector<uint8_t>>(value_);
        return std::string(bytes.begin(), bytes.end());
    }

    return "";
}

bool VariantValue::ConvertTo(TypeCategory targetType)
{
    auto& converter = SelectionTypeConverter::GetInstance();

    switch (targetType) {
        case TypeCategory::NUMERIC: {
            if (!std::holds_alternative<std::string>(value_)) {
                return false;
            }
            int64_t intVal = 0;
            if (!TryConvertVariantToInt(std::get<std::string>(value_), intVal)) {
                return false;
            }
            value_ = intVal;
            return true;
        }
        case TypeCategory::STRING: {
            std::string strVal;
            if (!TryConvertVariantToString(value_, strVal)) {
                return false;
            }
            value_ = strVal;
            return true;
        }
        case TypeCategory::BOOLEAN: {
            if (!std::holds_alternative<std::string>(value_)) {
                return false;
            }
            auto result = converter.ConvertToBool(std::get<std::string>(value_));
            if (!result.success) {
                return false;
            }
            value_ = result.GetBool();
            return true;
        }
        default:
            return false;
    }
}

bool VariantValue::IsNull() const
{
    return std::holds_alternative<std::string>(value_) && std::get<std::string>(value_).empty();
}

bool VariantValue::IsNumeric() const
{
    return std::holds_alternative<int64_t>(value_) || std::holds_alternative<double>(value_);
}

bool VariantValue::IsString() const
{
    return std::holds_alternative<std::string>(value_);
}

bool VariantValue::IsBool() const
{
    return std::holds_alternative<bool>(value_);
}

// TypeConversionPipeline implementation
TypeConversionPipeline::TypeConversionPipeline()
{
}

TypeConversionPipeline& TypeConversionPipeline::AddConversion(ConversionFunc conversion)
{
    conversions_.push_back(conversion);
    return *this;
}

TypeConversionPipeline& TypeConversionPipeline::AddIntConversion(const NumericConversionOptions& options)
{
    conversions_.push_back([options](const std::string& str) {
        auto& converter = SelectionTypeConverter::GetInstance();
        auto result = converter.ConvertToInt(str, options);
        return result.success ? std::to_string(result.GetInt()) : str;
    });
    return *this;
}

TypeConversionPipeline& TypeConversionPipeline::AddDoubleConversion(const NumericConversionOptions& options)
{
    conversions_.push_back([options](const std::string& str) {
        auto& converter = SelectionTypeConverter::GetInstance();
        auto result = converter.ConvertToDouble(str, options);
        return result.success ? std::to_string(result.GetDouble()) : str;
    });
    return *this;
}

TypeConversionPipeline& TypeConversionPipeline::AddBoolConversion()
{
    conversions_.push_back([](const std::string& str) {
        auto& converter = SelectionTypeConverter::GetInstance();
        auto result = converter.ConvertToBool(str);
        return result.success ? (result.GetBool() ? "true" : "false") : str;
    });
    return *this;
}

TypeConversionResult TypeConversionPipeline::Execute(const std::string& input) const
{
    std::string current = input;

    for (const auto& conversion : conversions_) {
        current = conversion(current);
    }

    return TypeConversionResult::Success(current, TypeCategory::STRING);
}

void TypeConversionPipeline::Clear()
{
    conversions_.clear();
}

// BulkTypeConverter implementation
BulkTypeConverter::BulkConversionResult BulkTypeConverter::ConvertAllToInt(
    const std::vector<std::string>& values, const NumericConversionOptions& options)
{
    BulkConversionResult result;

    auto& converter = SelectionTypeConverter::GetInstance();

    for (const auto& value : values) {
        auto conversionResult = converter.ConvertToInt(value, options);

        TypeConversionResult wrapperResult;
        if (conversionResult.success) {
            wrapperResult = TypeConversionResult::Success(conversionResult.GetInt(), TypeCategory::NUMERIC);
            result.successCount++;
        } else {
            wrapperResult = TypeConversionResult::Failure(conversionResult.errorMessage);
            result.failureCount++;
            result.errors.push_back(conversionResult.errorMessage);
        }

        result.results.push_back(wrapperResult);
    }

    return result;
}

BulkTypeConverter::BulkConversionResult BulkTypeConverter::ConvertAllToDouble(
    const std::vector<std::string>& values, const NumericConversionOptions& options)
{
    BulkConversionResult result;

    auto& converter = SelectionTypeConverter::GetInstance();

    for (const auto& value : values) {
        auto conversionResult = converter.ConvertToDouble(value, options);

        TypeConversionResult wrapperResult;
        if (conversionResult.success) {
            wrapperResult = TypeConversionResult::Success(conversionResult.GetDouble(), TypeCategory::NUMERIC);
            result.successCount++;
        } else {
            wrapperResult = TypeConversionResult::Failure(conversionResult.errorMessage);
            result.failureCount++;
            result.errors.push_back(conversionResult.errorMessage);
        }

        result.results.push_back(wrapperResult);
    }

    return result;
}

BulkTypeConverter::BulkConversionResult BulkTypeConverter::ConvertAllToBool(
    const std::vector<std::string>& values)
{
    BulkConversionResult result;

    auto& converter = SelectionTypeConverter::GetInstance();

    for (const auto& value : values) {
        auto conversionResult = converter.ConvertToBool(value);

        TypeConversionResult wrapperResult;
        if (conversionResult.success) {
            wrapperResult = TypeConversionResult::Success(conversionResult.GetBool(), TypeCategory::BOOLEAN);
            result.successCount++;
        } else {
            wrapperResult = TypeConversionResult::Failure(conversionResult.errorMessage);
            result.failureCount++;
            result.errors.push_back(conversionResult.errorMessage);
        }

        result.results.push_back(wrapperResult);
    }

    return result;
}

std::vector<std::string> BulkTypeConverter::ConvertAllToString(const std::vector<int64_t>& values)
{
    std::vector<std::string> result;
    auto& converter = SelectionTypeConverter::GetInstance();

    for (int64_t value : values) {
        auto conversionResult = converter.ConvertToString(value);
        if (conversionResult.success) {
            result.push_back(conversionResult.GetString());
        } else {
            result.push_back("");
        }
    }

    return result;
}

std::vector<std::string> BulkTypeConverter::ConvertAllToString(const std::vector<double>& values,
                                                              const NumericConversionOptions& options)
{
    std::vector<std::string> result;
    auto& converter = SelectionTypeConverter::GetInstance();

    for (double value : values) {
        auto conversionResult = converter.ConvertToString(value, options);
        if (conversionResult.success) {
            result.push_back(conversionResult.GetString());
        } else {
            result.push_back("");
        }
    }

    return result;
}

std::vector<std::string> BulkTypeConverter::ConvertAllToString(const std::vector<bool>& values)
{
    std::vector<std::string> result;
    auto& converter = SelectionTypeConverter::GetInstance();

    for (bool value : values) {
        auto conversionResult = converter.ConvertToString(value);
        if (conversionResult.success) {
            result.push_back(conversionResult.GetString());
        } else {
            result.push_back("");
        }
    }

    return result;
}

} // namespace SelectionFwk
} // namespace OHOS
