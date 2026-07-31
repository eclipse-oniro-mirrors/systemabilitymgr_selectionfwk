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

#include "selection_format_converter.h"
#include "selection_string_converter.h"
#include <algorithm>
#include <sstream>
#include <fstream>
#include <regex>

namespace OHOS {
namespace SelectionFwk {
namespace {
// 魔鬼数字具名常量
constexpr uint8_t ASCII_CONTROL_THRESHOLD = 32;   // ASCII 控制字符阈值（小于该值为控制字符）
constexpr int32_t BASE64_DECODE_TABLE_SIZE = 128;  // Base64 解码表大小
constexpr uint32_t BASE64_GROUP_SIZE = 4;           // Base64 每组 4 字符
constexpr uint32_t BASE64_TRIPLE_SIZE = 3;           // Base64 每组对应 3 字节
constexpr uint32_t BASE64_SHIFT_24 = 16;             // 24 位打包位移：最高字节
constexpr uint32_t BASE64_SHIFT_18 = 18;
constexpr uint32_t BASE64_SHIFT_16 = 16;
constexpr uint32_t BASE64_SHIFT_12 = 12;
constexpr uint32_t BASE64_SHIFT_8 = 8;
constexpr uint32_t BASE64_SHIFT_6 = 6;
constexpr uint32_t BASE64_MASK_6BIT = 0x3F;          // Base64 6 位掩码
constexpr uint32_t BASE64_MASK_8BIT = 0xFF;          // 8 位字节掩码
// 二进制与十六进制相关常量
constexpr int BITS_PER_BYTE = 8;                      // 每字节比特数
constexpr int HIGH_BIT_INDEX = 7;                      // 最高位索引（从 7 递减到 0）
constexpr size_t HEX_DIGITS_PER_BYTE = 2;             // 每字节对应的十六进制字符数
constexpr int HEX_RADIX = 16;                          // 十六进制基数
// 格式检测置信度
constexpr double CONFIDENCE_HIGH = 0.9;
constexpr double CONFIDENCE_MEDIUM = 0.85;
constexpr double CONFIDENCE_LOW = 0.7;

// 处理 JSON 美化扫描中的字符串/转义字符，命中（已消费该字符）返回 true
bool HandleScanCharInString(std::string& result, char c, bool& inString, bool& escapeNext)
{
    if (escapeNext) {
        result += c;
        escapeNext = false;
        return true;
    }
    if (c == '\\' && inString) {
        result += c;
        escapeNext = true;
        return true;
    }
    if (c == '"') {
        inString = !inString;
        result += c;
        return true;
    }
    if (inString) {
        result += c;
        return true;
    }
    return false;
}

// 处理 JSON 美化中的结构字符（{}[] , :）
void AppendStructChar(std::string& result, char c, uint32_t indent, int32_t& level)
{
    switch (c) {
        case '{':
        case '[':
            result += c;
            result += '\n';
            result += std::string(static_cast<size_t>(level + 1) * indent, ' ');
            level++;
            break;
        case '}':
        case ']':
            result += '\n';
            level--;
            result += std::string(static_cast<size_t>(level) * indent, ' ');
            result += c;
            break;
        case ',':
            result += c;
            result += '\n';
            result += std::string(static_cast<size_t>(level) * indent, ' ');
            break;
        case ':':
            result += c;
            result += ' ';
            break;
        default:
            if (!std::isspace(static_cast<unsigned char>(c))) {
                result += c;
            }
            break;
    }
}

// 处理 XML 压缩中的注释/CDATA/标签间空白跳过逻辑，命中返回 true 并推进 i
bool HandleXmlMinifyChar(std::string& result, const std::string& xml, size_t& i,
                         bool& inComment, bool& inCDATA)
{
    constexpr size_t COMMENT_START_LEN = 4;  // "<!--"
    constexpr size_t COMMENT_END_LEN = 3;    // "-->"
    constexpr size_t CDATA_START_LEN = 9;    // "<![CDATA["
    constexpr size_t CDATA_END_LEN = 3;      // "]]>"

    if (i + COMMENT_START_LEN - 1 < xml.length() &&
        xml.substr(i, COMMENT_START_LEN) == "<!--") {
        inComment = true;
        result += "<!--";
        i += COMMENT_START_LEN - 1;
        return true;
    }
    if (inComment && i + COMMENT_END_LEN - 1 < xml.length() &&
        xml.substr(i, COMMENT_END_LEN) == "-->") {
        inComment = false;
        result += "-->";
        i += COMMENT_END_LEN - 1;
        return true;
    }
    if (i + CDATA_START_LEN - 1 < xml.length() &&
        xml.substr(i, CDATA_START_LEN) == "<![CDATA[") {
        inCDATA = true;
        result += "<![CDATA[";
        i += CDATA_START_LEN - 1;
        return true;
    }
    if (inCDATA && i + CDATA_END_LEN - 1 < xml.length() &&
        xml.substr(i, CDATA_END_LEN) == "]]>") {
        inCDATA = false;
        result += "]]>";
        i += CDATA_END_LEN - 1;
        return true;
    }
    if (inComment || inCDATA) {
        return false;
    }
    if (std::isspace(static_cast<unsigned char>(xml[i])) &&
        i > 0 && xml[i - 1] == '>' && i + 1 < xml.length() && xml[i + 1] == '<') {
        return true;
    }
    return false;
}

// 按需对 CSV 字段两端裁剪空白
std::string TrimCsvField(const std::string& field, bool trimFields)
{
    if (!trimFields) {
        return field;
    }
    auto trimResult = SelectionStringConverter::GetInstance().Trim(field, TrimType::BOTH);
    return trimResult.success ? trimResult.result : field;
}

// 对单个 CSV 字段完成反转义 + 裁剪
std::string FinalizeCsvField(const std::string& field, bool trimFields)
{
    std::string unescaped = SelectionStringConverter::UnescapeCSV(field);
    return TrimCsvField(unescaped, trimFields);
}

// CSV 解析中遇到换行时收尾当前行
void FinishCsvRow(std::vector<std::vector<std::string>>& result,
                  std::vector<std::string>& currentRow,
                  std::string& currentField, bool trimFields)
{
    if (!currentField.empty() || !currentRow.empty()) {
        currentRow.push_back(FinalizeCsvField(currentField, trimFields));
    }
    if (!currentRow.empty()) {
        result.push_back(currentRow);
        currentRow.clear();
    }
    currentField.clear();
}

// 将最多 3 字节编码为 Base64 字符并追加；count 为实际字节数（1-3）
void EncodeBase64Triple(std::string& result, const uint8_t* triple,
                        size_t count, const char* encodeTable)
{
    uint32_t value = (static_cast<uint32_t>(triple[0]) << BASE64_SHIFT_24);
    if (count >= BASE64_TRIPLE_SIZE - 1) {
        value |= (static_cast<uint32_t>(triple[1]) << BASE64_SHIFT_8);
    }
    if (count >= BASE64_TRIPLE_SIZE) {
        value |= static_cast<uint32_t>(triple[2]);
    }
    result += encodeTable[(value >> BASE64_SHIFT_18) & BASE64_MASK_6BIT];
    result += encodeTable[(value >> BASE64_SHIFT_12) & BASE64_MASK_6BIT];
    if (count >= BASE64_TRIPLE_SIZE - 1) {
        result += encodeTable[(value >> BASE64_SHIFT_6) & BASE64_MASK_6BIT];
    } else {
        result += '=';
    }
    if (count >= BASE64_TRIPLE_SIZE) {
        result += encodeTable[value & BASE64_MASK_6BIT];
    } else {
        result += '=';
    }
}

// 解析 4 个 Base64 字符并追加解码字节（'=' 为填充）
void DecodeBase64Quartet(std::vector<uint8_t>& result, const std::string& base64,
                         size_t pos, const int* decodeTable)
{
    uint32_t value = 0;
    for (uint32_t j = 0; j < BASE64_GROUP_SIZE; ++j) {
        if (base64[pos + j] == '=') {
            continue;
        }
        unsigned char c = static_cast<unsigned char>(base64[pos + j]);
        if (c < BASE64_DECODE_TABLE_SIZE) {
            int decoded = decodeTable[c];
            if (decoded >= 0) {
                value = (value << BASE64_SHIFT_6) | static_cast<uint32_t>(decoded);
            }
        }
    }
    result.push_back((value >> BASE64_SHIFT_16) & BASE64_MASK_8BIT);
    if (base64[pos + 2] != '=') {
        result.push_back((value >> BASE64_SHIFT_8) & BASE64_MASK_8BIT);
    }
    if (base64[pos + 3] != '=') {
        result.push_back(value & BASE64_MASK_8BIT);
    }
}

// 按目标格式派发 JSON 源转换，避免 Convert 中多层 switch 嵌套
FormatConversionResult ConvertJsonTo(DataFormat toFormat, const std::string& input)
{
    switch (toFormat) {
        case DataFormat::XML:
            return SelectionFormatConverter::JsonToXml(input);
        case DataFormat::CSV:
            return SelectionFormatConverter::JsonToCsv(input);
        default:
            return FormatConversionResult::Failure("Unsupported conversion");
    }
}

// 按目标格式派发 XML 源转换
FormatConversionResult ConvertXmlTo(DataFormat toFormat, const std::string& input)
{
    switch (toFormat) {
        case DataFormat::JSON:
            return SelectionFormatConverter::XmlToJson(input);
        case DataFormat::CSV:
            return SelectionFormatConverter::XmlToCsv(input);
        default:
            return FormatConversionResult::Failure("Unsupported conversion");
    }
}

// 按目标格式派发 CSV 源转换
FormatConversionResult ConvertCsvTo(DataFormat toFormat, const std::string& input)
{
    switch (toFormat) {
        case DataFormat::JSON:
            return SelectionFormatConverter::CsvToJson(input);
        case DataFormat::XML:
            return SelectionFormatConverter::CsvToXml(input);
        default:
            return FormatConversionResult::Failure("Unsupported conversion");
    }
}
} // namespace

// FormatConversionResult implementation
FormatConversionResult FormatConversionResult::Success(const std::string& value, uint32_t bytes)
{
    FormatConversionResult result;
    result.success = true;
    result.result = value;
    result.bytesProcessed = bytes;
    return result;
}

FormatConversionResult FormatConversionResult::Failure(const std::string& error)
{
    FormatConversionResult result;
    result.success = false;
    result.errorMessage = error;
    return result;
}

// SelectionFormatConverter implementation
SelectionFormatConverter& SelectionFormatConverter::GetInstance()
{
    static SelectionFormatConverter instance;
    return instance;
}

FormatConversionResult SelectionFormatConverter::Convert(const std::string& input,
                                                        DataFormat fromFormat,
                                                        DataFormat toFormat,
                                                        const std::unordered_map<std::string, std::string>& options)
{
    (void)options;
    if (fromFormat == toFormat) {
        return FormatConversionResult::Success(input);
    }

    switch (fromFormat) {
        case DataFormat::JSON:
            return ConvertJsonTo(toFormat, input);
        case DataFormat::XML:
            return ConvertXmlTo(toFormat, input);
        case DataFormat::CSV:
            return ConvertCsvTo(toFormat, input);
        default:
            return FormatConversionResult::Failure("Unsupported source format");
    }
}

bool SelectionFormatConverter::SupportsFormat(DataFormat format) const
{
    return format == DataFormat::JSON || format == DataFormat::XML || format == DataFormat::CSV;
}

std::vector<DataFormat> SelectionFormatConverter::GetSupportedFormats() const
{
    return {DataFormat::JSON, DataFormat::XML, DataFormat::CSV};
}

FormatConversionResult SelectionFormatConverter::JsonToXml(const std::string& json)
{
    // Simplified JSON to XML conversion
    std::string xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    xml += "<root>\n";

    // Basic JSON parsing and XML generation
    // In production, use proper JSON/XML libraries

    xml += "</root>";
    return FormatConversionResult::Success(xml);
}

FormatConversionResult SelectionFormatConverter::XmlToJson(const std::string& xml)
{
    // Simplified XML to JSON conversion
    std::string json = "{\n";

    // Basic XML parsing and JSON generation
    // In production, use proper JSON/XML libraries

    json += "\n}";
    return FormatConversionResult::Success(json);
}

FormatConversionResult SelectionFormatConverter::JsonToCsv(const std::string& json)
{
    // Simplified JSON to CSV conversion
    std::string csv = "";

    // Basic JSON parsing and CSV generation
    // In production, use proper JSON/CSV libraries

    return FormatConversionResult::Success(csv);
}

FormatConversionResult SelectionFormatConverter::CsvToJson(const std::string& csv)
{
    // Simplified CSV to JSON conversion
    std::string json = "[\n";

    // Basic CSV parsing and JSON generation
    // In production, use proper JSON/CSV libraries

    json += "\n]";
    return FormatConversionResult::Success(json);
}

FormatConversionResult SelectionFormatConverter::XmlToCsv(const std::string& xml)
{
    // Simplified XML to CSV conversion
    std::string csv = "";

    // Basic XML parsing and CSV generation
    // In production, use proper XML/CSV libraries

    return FormatConversionResult::Success(csv);
}

FormatConversionResult SelectionFormatConverter::CsvToXml(const std::string& csv)
{
    // Simplified CSV to XML conversion
    std::string xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    xml += "<rows>\n";

    // Basic CSV parsing and XML generation
    // In production, use proper CSV/XML libraries

    xml += "</rows>";
    return FormatConversionResult::Success(xml);
}

// JsonUtil implementation
std::string JsonUtil::PrettyPrint(const std::string& json, uint32_t indent)
{
    std::string result;
    int32_t level = 0;
    bool inString = false;
    bool escapeNext = false;

    for (size_t i = 0; i < json.length(); ++i) {
        char c = json[i];
        if (HandleScanCharInString(result, c, inString, escapeNext)) {
            continue;
        }
        AppendStructChar(result, c, indent, level);
    }

    return result;
}

std::string JsonUtil::Minify(const std::string& json)
{
    std::string result;
    bool inString = false;
    bool escapeNext = false;

    for (size_t i = 0; i < json.length(); ++i) {
        char c = json[i];
        if (HandleScanCharInString(result, c, inString, escapeNext)) {
            continue;
        }
        if (!std::isspace(static_cast<unsigned char>(c))) {
            result += c;
        }
    }

    return result;
}

bool JsonUtil::IsValid(const std::string& json)
{
    // Basic JSON validation
    int32_t braceCount = 0;
    int32_t bracketCount = 0;
    bool inString = false;
    bool escapeNext = false;

    for (char c : json) {
        if (escapeNext) {
            escapeNext = false;
            continue;
        }

        if (c == '\\' && inString) {
            escapeNext = true;
            continue;
        }

        if (c == '"') {
            inString = !inString;
            continue;
        }

        if (inString) {
            continue;
        }

        if (c == '{') {
            braceCount++;
        } else if (c == '}') {
            braceCount--;
        } else if (c == '[') {
            bracketCount++;
        } else if (c == ']') {
            bracketCount--;
        }
    }

    return braceCount == 0 && bracketCount == 0 && !inString;
}

std::string JsonUtil::GetValue(const std::string& json, const std::string& path)
{
    // Simplified path-based value extraction
    // In production, use a proper JSON library

    return "";
}

std::string JsonUtil::SetValue(const std::string& json, const std::string& path, const std::string& value)
{
    // Simplified path-based value setting
    // In production, use a proper JSON library

    return json;
}

std::string JsonUtil::Escape(const std::string& str)
{
    return SelectionStringConverter::EscapeJSON(str);
}

std::string JsonUtil::Unescape(const std::string& str)
{
    return SelectionStringConverter::UnescapeJSON(str);
}

std::vector<std::string> JsonUtil::GetKeys(const std::string& json)
{
    std::vector<std::string> keys;

    // Simplified key extraction
    // In production, use a proper JSON library

    return keys;
}

bool JsonUtil::HasKey(const std::string& json, const std::string& key)
{
    // Simplified key check
    // In production, use a proper JSON library

    return json.find("\"" + key + "\"") != std::string::npos;
}

// XmlUtil implementation
std::string XmlUtil::PrettyPrint(const std::string& xml, uint32_t indent)
{
    std::string result;
    int32_t level = 0;

    std::string indentStr(indent, ' ');
    std::regex tagRegex("<([^>]+)>");
    std::string::const_iterator searchStart = xml.cbegin();
    std::smatch match;

    while (std::regex_search(searchStart, xml.cend(), match, tagRegex)) {
        std::string tag = match[1];
        bool isClosing = tag[0] == '/';
        bool isSelfClosing = tag[tag.length() - 1] == '/';

        if (!isClosing && !isSelfClosing) {
            if (!result.empty() && result.back() != '\n') {
                result += '\n';
            }
            result += std::string(level * indent, ' ');
        }

        result += match[0];

        if (!isClosing && !isSelfClosing && tag.find('?') == std::string::npos) {
            level++;
        } else if (isClosing) {
            level--;
        }

        searchStart = match.suffix().first;
    }

    return result;
}

std::string XmlUtil::Minify(const std::string& xml)
{
    std::string result;
    bool inComment = false;
    bool inCDATA = false;

    for (size_t i = 0; i < xml.length(); ++i) {
        if (HandleXmlMinifyChar(result, xml, i, inComment, inCDATA)) {
            continue;
        }
        result += xml[i];
    }

    return result;
}

bool XmlUtil::IsValid(const std::string& xml)
{
    // Basic XML validation
    if (xml.empty()) {
        return false;
    }

    // Check for proper tag structure
    int32_t tagCount = 0;
    bool inTag = false;
    bool inString = false;

    for (char c : xml) {
        if (c == '"' && !inTag) {
            inString = !inString;
        }

        if (inString) {
            continue;
        }

        if (c == '<') {
            inTag = true;
            tagCount++;
        } else if (c == '>') {
            inTag = false;
            tagCount--;
        }
    }

    return tagCount == 0 && !inTag;
}

std::string XmlUtil::GetValue(const std::string& xml, const std::string& xpath)
{
    // Simplified XPath-based value extraction
    // In production, use a proper XML library

    return "";
}

std::string XmlUtil::SetValue(const std::string& xml, const std::string& xpath, const std::string& value)
{
    // Simplified XPath-based value setting
    // In production, use a proper XML library

    return xml;
}

std::string XmlUtil::Escape(const std::string& str)
{
    return SelectionStringConverter::EscapeXML(str);
}

std::string XmlUtil::Unescape(const std::string& str)
{
    return SelectionStringConverter::UnescapeXML(str);
}

std::vector<std::string> XmlUtil::GetElementsByTagName(const std::string& xml, const std::string& tagName)
{
    std::vector<std::string> elements;

    std::string openTag = "<" + tagName;
    std::string closeTag = "</" + tagName + ">";

    size_t pos = 0;
    while ((pos = xml.find(openTag, pos)) != std::string::npos) {
        size_t endPos = xml.find(closeTag, pos);
        if (endPos != std::string::npos) {
            // Extract the element content
            size_t contentStart = pos + openTag.length();
            while (contentStart < xml.length() && xml[contentStart] != '>') {
                contentStart++;
            }
            contentStart++; // Skip '>'

            std::string element = xml.substr(contentStart, endPos - contentStart);
            elements.push_back(element);
        }
        pos = endPos + closeTag.length();
    }

    return elements;
}

std::vector<std::string> XmlUtil::GetAttributes(const std::string& xml, const std::string& tagName)
{
    std::vector<std::string> attributes;

    std::string openTag = "<" + tagName;

    size_t pos = xml.find(openTag);
    if (pos != std::string::npos) {
        size_t tagEnd = xml.find('>', pos);
        if (tagEnd != std::string::npos) {
            std::string tagContent = xml.substr(pos, tagEnd - pos);

            // Extract attributes
            std::regex attrRegex("(\\w+)\\s*=\\s*\"([^\"]*)\"");
            std::string::const_iterator searchStart = tagContent.cbegin();
            std::smatch match;

            while (std::regex_search(searchStart, tagContent.cend(), match, attrRegex)) {
                attributes.push_back(match[1]);
                searchStart = match.suffix().first;
            }
        }
    }

    return attributes;
}

// CsvUtil implementation
std::string CsvUtil::PrettyPrint(const std::string& csv, const CsvOptions& options)
{
    // Basic CSV pretty printing
    (void)options;
    return csv;
}

std::vector<std::vector<std::string>> CsvUtil::Parse(const std::string& csv, const CsvOptions& options)
{
    std::vector<std::vector<std::string>> result;
    std::vector<std::string> currentRow;
    std::string currentField;
    bool inQuotes = false;

    for (size_t i = 0; i < csv.length(); ++i) {
        char c = csv[i];

        if (c == options.quoteChar) {
            inQuotes = !inQuotes;
        } else if (c == options.delimiter && !inQuotes) {
            currentRow.push_back(FinalizeCsvField(currentField, options.trimFields));
            currentField.clear();
        } else if (c == '\n' && !inQuotes) {
            FinishCsvRow(result, currentRow, currentField, options.trimFields);
        } else {
            currentField += c;
        }
    }

    // 处理末尾字段/行
    FinishCsvRow(result, currentRow, currentField, options.trimFields);

    return result;
}

std::string CsvUtil::Format(const std::vector<std::vector<std::string>>& data, const CsvOptions& options)
{
    std::ostringstream oss;

    for (size_t row = 0; row < data.size(); ++row) {
        for (size_t col = 0; col < data[row].size(); ++col) {
            if (col > 0) {
                oss << options.delimiter;
            }

            std::string field = data[row][col];
            oss << SelectionStringConverter::EscapeCSV(field);
        }

        if (row < data.size() - 1) {
            oss << '\n';
        }
    }

    return oss.str();
}

std::string CsvUtil::GetField(const std::string& csv, uint32_t row, uint32_t col,
                              const CsvOptions& options)
{
    auto parsedData = Parse(csv, options);

    if (row < parsedData.size() && col < parsedData[row].size()) {
        return parsedData[row][col];
    }

    return "";
}

std::string CsvUtil::SetField(const std::string& csv, uint32_t row, uint32_t col,
                              const std::string& value, const CsvOptions& options)
{
    auto parsedData = Parse(csv, options);

    // Ensure the data has enough rows and columns
    while (parsedData.size() <= row) {
        parsedData.push_back(std::vector<std::string>());
    }

    while (parsedData[row].size() <= col) {
        parsedData[row].push_back("");
    }

    parsedData[row][col] = value;

    return Format(parsedData, options);
}

std::vector<std::string> CsvUtil::GetHeaders(const std::string& csv, const CsvOptions& options)
{
    auto parsedData = Parse(csv, options);

    if (options.hasHeader && !parsedData.empty()) {
        return parsedData[0];
    }

    return std::vector<std::string>();
}

// FormatValidator implementation
FormatValidator::ValidationResult FormatValidator::ValidateJson(const std::string& json)
{
    ValidationResult result;

    if (!JsonUtil::IsValid(json)) {
        result.AddError("Invalid JSON format");
    }

    return result;
}

FormatValidator::ValidationResult FormatValidator::ValidateXml(const std::string& xml)
{
    ValidationResult result;

    if (!XmlUtil::IsValid(xml)) {
        result.AddError("Invalid XML format");
    }

    return result;
}

FormatValidator::ValidationResult FormatValidator::ValidateCsv(const std::string& csv)
{
    ValidationResult result;

    // Basic CSV validation
    // In production, implement more thorough validation

    return result;
}

FormatValidator::ValidationResult FormatValidator::ValidateYaml(const std::string& yaml)
{
    ValidationResult result;

    // Basic YAML validation
    // In production, use a proper YAML library

    return result;
}

FormatValidator::ValidationResult FormatValidator::ValidateHex(const std::string& hex)
{
    ValidationResult result;

    for (char c : hex) {
        if (!std::isxdigit(static_cast<unsigned char>(c))) {
            result.AddError("Invalid hex character");
            break;
        }
    }

    return result;
}

FormatValidator::ValidationResult FormatValidator::ValidateBase64(const std::string& base64)
{
    ValidationResult result;

    for (char c : base64) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '+' && c != '/' && c != '=') {
            result.AddError("Invalid base64 character");
            break;
        }
    }

    return result;
}

// FormatDetector implementation
FormatDetector::DetectionResult FormatDetector::DetectFormat(const std::string& data)
{
    DetectionResult result;

    // Basic format detection
    if (data.empty()) {
        result.detectedFormat = DataFormat::STRING; // Default
        return result;
    }

    // Check for JSON
    if ((data[0] == '{' || data[0] == '[') && JsonUtil::IsValid(data)) {
        result.detectedFormat = DataFormat::JSON;
        result.confidence = CONFIDENCE_HIGH;
        result.formatProbabilities[DataFormat::JSON] = CONFIDENCE_HIGH;
    }

    // Check for XML
    if (data.find("<?xml") == 0 || data.find("<root") == 0) {
        if (XmlUtil::IsValid(data)) {
            result.detectedFormat = DataFormat::XML;
            result.confidence = CONFIDENCE_MEDIUM;
            result.formatProbabilities[DataFormat::XML] = CONFIDENCE_MEDIUM;
        }
    }

    // Check for CSV
    if (data.find(',') != std::string::npos && data.find('\n') != std::string::npos) {
        result.detectedFormat = DataFormat::CSV;
        result.confidence = CONFIDENCE_LOW;
        result.formatProbabilities[DataFormat::CSV] = CONFIDENCE_LOW;
    }

    return result;
}

FormatDetector::DetectionResult FormatDetector::DetectFormat(const std::vector<uint8_t>& data)
{
    std::string str(data.begin(), data.end());
    return DetectFormat(str);
}

bool FormatDetector::IsBinary(const std::string& data)
{
    for (char c : data) {
        if (static_cast<unsigned char>(c) < ASCII_CONTROL_THRESHOLD && c != '\t' &&
            c != '\n' && c != '\r') {
            return true;
        }
    }
    return false;
}

bool FormatDetector::IsBinary(const std::vector<uint8_t>& data)
{
    for (uint8_t b : data) {
        if (b < ASCII_CONTROL_THRESHOLD && b != '\t' && b != '\n' && b != '\r') {
            return true;
        }
    }
    return false;
}

// BinaryFormatUtil implementation
std::vector<uint8_t> BinaryFormatUtil::StringToBytes(const std::string& str, StringEncoding encoding)
{
    std::vector<uint8_t> result;

    switch (encoding) {
        case StringEncoding::UTF8:
            result.assign(str.begin(), str.end());
            break;
        default:
            result.assign(str.begin(), str.end());
            break;
    }

    return result;
}

std::string BinaryFormatUtil::BytesToString(const std::vector<uint8_t>& bytes, StringEncoding encoding)
{
    switch (encoding) {
        case StringEncoding::UTF8:
            return std::string(bytes.begin(), bytes.end());
        default:
            return std::string(bytes.begin(), bytes.end());
    }
}

std::string BinaryFormatUtil::ToHex(const std::vector<uint8_t>& bytes)
{
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');

    for (uint8_t b : bytes) {
        oss << std::setw(HEX_DIGITS_PER_BYTE) << static_cast<int>(b);
    }

    return oss.str();
}

std::vector<uint8_t> BinaryFormatUtil::FromHex(const std::string& hex)
{
    std::vector<uint8_t> result;

    // Process pairs of hex characters; ignore a trailing odd character safely.
    size_t pairCount = hex.length() / HEX_DIGITS_PER_BYTE;
    for (size_t i = 0; i < pairCount; ++i) {
        std::string byteString = hex.substr(i * HEX_DIGITS_PER_BYTE, HEX_DIGITS_PER_BYTE);
        try {
            uint8_t byte = static_cast<uint8_t>(std::stoul(byteString, nullptr, HEX_RADIX));
            result.push_back(byte);
        } catch (const std::exception&) {
            // Skip invalid hex characters to avoid crashing on malformed input.
            continue;
        }
    }

    return result;
}

std::string BinaryFormatUtil::ToBase64(const std::vector<uint8_t>& bytes)
{
    static const char* encodeTable = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;

    for (size_t i = 0; i < bytes.size(); i += BASE64_TRIPLE_SIZE) {
        size_t remaining = bytes.size() - i;
        size_t count = remaining >= BASE64_TRIPLE_SIZE ? BASE64_TRIPLE_SIZE : remaining;
        EncodeBase64Triple(result, &bytes[i], count, encodeTable);
    }

    return result;
}

std::vector<uint8_t> BinaryFormatUtil::FromBase64(const std::string& base64)
{
    std::vector<uint8_t> result;
    // Base64 解码值：'+' -> 62，'/' -> 63，其余字母数字按顺序映射
    constexpr int B64_VAL_PLUS = 62;
    constexpr int B64_VAL_SLASH = 63;

    static const int decodeTable[BASE64_DECODE_TABLE_SIZE] = {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, B64_VAL_PLUS, -1, -1, -1, B64_VAL_SLASH,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
        -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
        -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1
    };

    for (size_t i = 0; i + BASE64_GROUP_SIZE - 1 < base64.length(); i += BASE64_GROUP_SIZE) {
        DecodeBase64Quartet(result, base64, i, decodeTable);
    }

    return result;
}

std::string BinaryFormatUtil::ToBinaryString(const std::vector<uint8_t>& bytes)
{
    std::ostringstream oss;

    for (uint8_t b : bytes) {
        for (int i = HIGH_BIT_INDEX; i >= 0; --i) {
            oss << ((b >> i) & 1 ? '1' : '0');
        }
        oss << ' ';
    }

    return oss.str();
}

std::vector<uint8_t> BinaryFormatUtil::FromBinaryString(const std::string& binaryStr)
{
    std::vector<uint8_t> result;
    std::string cleanBinary;

    // 移除空白分隔
    for (char c : binaryStr) {
        if (c == '0' || c == '1') {
            cleanBinary += c;
        }
    }

    // 不足 8 位时高位补零对齐到整字节
    while (cleanBinary.length() % BITS_PER_BYTE != 0) {
        cleanBinary = '0' + cleanBinary;
    }

    for (size_t i = 0; i < cleanBinary.length(); i += BITS_PER_BYTE) {
        uint8_t byte = 0;
        for (uint32_t j = 0; j < BITS_PER_BYTE; ++j) {
            byte = (byte << 1) | (cleanBinary[i + j] == '1' ? 1 : 0);
        }
        result.push_back(byte);
    }

    return result;
}

} // namespace SelectionFwk
} // namespace OHOS
