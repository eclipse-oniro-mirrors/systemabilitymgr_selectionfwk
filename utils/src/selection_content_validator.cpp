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

// 选区内容校验器：负责对用户选中的文本做合规性检查与净化。
// 约束来源：selection content capped at 6,000 bytes（见 AGENTS.md）。
// 本文件当前为预留实现，未接入 service/plugins 的 BUILD.gn，待统一替换散落的硬编码校验后启用。

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include "selection_errors.h"
#include "selection_log.h"

namespace OHOS {
namespace SelectionFwk {

namespace {
// 选区内容字节上限，与 AGENTS.md 约束一致
constexpr uint32_t CONTENT_MAX_BYTES = 6000;

// UTF-8 续接字节取值范围
constexpr uint8_t UTF8_CONTINUATION_MIN = 0x80;
constexpr uint8_t UTF8_CONTINUATION_MAX = 0xBF;

// UTF-8 首字节掩码与取值
constexpr uint8_t UTF8_LEAD_MASK_TWO = 0xE0;
constexpr uint8_t UTF8_LEAD_VAL_TWO = 0xC0;
constexpr uint8_t UTF8_LEAD_MASK_THREE = 0xF0;
constexpr uint8_t UTF8_LEAD_VAL_THREE = 0xE0;
constexpr uint8_t UTF8_LEAD_MASK_FOUR = 0xF8;
constexpr uint8_t UTF8_LEAD_VAL_FOUR = 0xF0;

// UTF-8 解码时各首字节的有效载荷掩码
constexpr uint8_t UTF8_TWO_BYTE_PAYLOAD_MASK = 0x1F;
constexpr uint8_t UTF8_THREE_BYTE_PAYLOAD_MASK = 0x0F;
constexpr uint8_t UTF8_FOUR_BYTE_PAYLOAD_MASK = 0x07;
constexpr uint8_t UTF8_CONTINUATION_PAYLOAD_MASK = 0x3F;

// 每个续接字节贡献的有效位数，位移量按其倍数推导（6 / 12 / 18）
constexpr uint8_t UTF8_CONTINUATION_BITS = 6;
// 组合位移量：三字节首字节 / 四字节第二续接用 DOUBLE，四字节首字节用 TRIPLE
constexpr uint8_t UTF8_CONT_SHIFT_DOUBLE = UTF8_CONTINUATION_BITS * 2;
constexpr uint8_t UTF8_CONT_SHIFT_TRIPLE = UTF8_CONTINUATION_BITS * 3;

// UTF-8 序列长度（字节数）
constexpr uint8_t UTF8_SEQ_LEN_INVALID = 0;
constexpr uint8_t UTF8_SEQ_LEN_ONE = 1;
constexpr uint8_t UTF8_SEQ_LEN_TWO = 2;
constexpr uint8_t UTF8_SEQ_LEN_THREE = 3;
constexpr uint8_t UTF8_SEQ_LEN_FOUR = 4;

// UTF-8 续接字节在序列内的偏移（首字节偏移为 0）
constexpr uint8_t UTF8_CONT_OFFSET_FIRST = 1;
constexpr uint8_t UTF8_CONT_OFFSET_SECOND = 2;
constexpr uint8_t UTF8_CONT_OFFSET_THIRD = 3;

// 合法的 UTF-8 首字节上下界（排除 overlong 序列）
constexpr uint8_t UTF8_TWO_BYTE_MIN = 0xC2;
constexpr uint8_t UTF8_TWO_BYTE_MAX = 0xDF;
constexpr uint8_t UTF8_THREE_BYTE_MIN = 0xE0;
constexpr uint8_t UTF8_THREE_BYTE_MAX = 0xEF;
constexpr uint8_t UTF8_FOUR_BYTE_MIN = 0xF0;
constexpr uint8_t UTF8_FOUR_BYTE_MAX = 0xF4;

// 允许保留的控制字符：水平制表、换行、回车
constexpr uint8_t ALLOWED_HT = 0x09;
constexpr uint8_t ALLOWED_LF = 0x0A;
constexpr uint8_t ALLOWED_CR = 0x0D;
constexpr uint8_t DEL_BYTE = 0x7F;

// ASCII 控制字符区上界（< 此值为控制字符）与空格码点
constexpr uint8_t ASCII_CONTROL_BOUND = 0x20;
constexpr uint8_t ASCII_SPACE = 0x20;

// UTF-8 BOM
constexpr uint8_t BOM_BYTE0 = 0xEF;
constexpr uint8_t BOM_BYTE1 = 0xBB;
constexpr uint8_t BOM_BYTE2 = 0xBF;
constexpr uint8_t BOM_LEN = 3;
constexpr size_t BOM_BYTE_INDEX_0 = 0;
constexpr size_t BOM_BYTE_INDEX_1 = 1;
constexpr size_t BOM_BYTE_INDEX_2 = 2;

// FNV-1a 64bit 参数
constexpr uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
constexpr uint64_t FNV_PRIME = 1099511628211ULL;

// ASCII 标点四段区间
constexpr uint32_t ASCII_PUNCT_RANGE1_MIN = 0x21;
constexpr uint32_t ASCII_PUNCT_RANGE1_MAX = 0x2F;
constexpr uint32_t ASCII_PUNCT_RANGE2_MIN = 0x3A;
constexpr uint32_t ASCII_PUNCT_RANGE2_MAX = 0x40;
constexpr uint32_t ASCII_PUNCT_RANGE3_MIN = 0x5B;
constexpr uint32_t ASCII_PUNCT_RANGE3_MAX = 0x60;
constexpr uint32_t ASCII_PUNCT_RANGE4_MIN = 0x7B;
constexpr uint32_t ASCII_PUNCT_RANGE4_MAX = 0x7E;

// ASCII 字母 / 数字码点区间
constexpr uint32_t ASCII_UPPER_A = 0x41;
constexpr uint32_t ASCII_UPPER_Z = 0x5A;
constexpr uint32_t ASCII_LOWER_A = 0x61;
constexpr uint32_t ASCII_LOWER_Z = 0x7A;
constexpr uint32_t ASCII_DIGIT_ZERO = 0x30;
constexpr uint32_t ASCII_DIGIT_NINE = 0x39;

// Unicode 码点区间
constexpr uint32_t CP_HAN_MIN = 0x4E00;
constexpr uint32_t CP_HAN_MAX = 0x9FFF;
constexpr uint32_t CP_HAN_EXT_A_MIN = 0x3400;
constexpr uint32_t CP_HAN_EXT_A_MAX = 0x4DBF;
constexpr uint32_t CP_HAN_COMPAT_MIN = 0xF900;
constexpr uint32_t CP_HAN_COMPAT_MAX = 0xFAFF;
constexpr uint32_t CP_ZWSP = 0x200B;      // 零宽空格
constexpr uint32_t CP_ZWNJ = 0x200C;      // 零宽非连接符
constexpr uint32_t CP_ZWJ = 0x200D;       // 零宽连接符
constexpr uint32_t CP_LRM = 0x200E;       // 从左至右标记
constexpr uint32_t CP_RLM = 0x200F;       // 从右至左标记
constexpr uint32_t CP_WJ = 0x2060;        // 文字连接符
constexpr uint32_t CP_ZWNBSP = 0xFEFF;    // 零宽不换行空格（亦作 BOM）
constexpr uint32_t CP_INVALID = 0;        // 解码失败时返回的无效码点

// 配置项禁用哨兵（0 表示不限制 / 不启用）
constexpr uint32_t CONFIG_DISABLED = 0;

// 连续重复字符计数器初值
constexpr uint32_t REPEAT_COUNT_RESET = 0;
constexpr uint32_t REPEAT_COUNT_INITIAL = 1;
} // namespace

// 选区内容校验结果，便于调用方定位失败原因
struct SelectionContentValidationResult {
    SelectionServiceError code = INVALID_DATA;
    uint32_t byteLen = 0;
    uint32_t charLen = 0;
    bool truncated = false;
};

// 字符类别统计，用于选区内容画像与 DFX 上报
struct CharCategoryStat {
    uint32_t letters = 0;       // ASCII 字母
    uint32_t digits = 0;       // ASCII 数字
    uint32_t han = 0;          // 汉字（含扩展 A / 兼容区）
    uint32_t punctuation = 0;  // ASCII 标点
    uint32_t whitespace = 0;    // 空白字符
    uint32_t control = 0;      // 控制字符（已剔除 \t \n \r）
    uint32_t zeroWidth = 0;    // 零宽字符
    uint32_t other = 0;        // 其余可见字符
    uint32_t total = 0;        // 合计字符数
};

// 校验器配置，供带配置的入口使用
struct ValidatorConfig {
    uint32_t maxBytes = CONTENT_MAX_BYTES;
    uint32_t maxChars = CONFIG_DISABLED;             // CONFIG_DISABLED 表示不限制字符数
    bool stripBom = true;              // 净化时移除 BOM
    bool dropZeroWidth = true;         // 净化时移除零宽字符
    bool dropControlChars = true;      // 净化时移除控制字符
    bool normalizeWhitespace = false;  // 合并连续空白为单个空格
    uint32_t maxRepeat = CONFIG_DISABLED;            // CONFIG_DISABLED 表示不做连续重复压缩
};

// 单字符净化决策，由 DecideSanitizeAction 返回
enum class SanitizeAction {
    KEEP,
    DROP,
    REPLACE_SPACE,
    REPLACE_NEWLINE,
};

class SelectionContentValidator {
public:
    static SelectionContentValidator& GetInstance();

    // 综合校验入口，返回详细结果
    SelectionContentValidationResult Validate(std::string_view content) const;

    // 按配置综合校验
    SelectionContentValidationResult ValidateWithConfig(std::string_view content,
        const ValidatorConfig& cfg) const;

    // 字节长度是否超出上限
    bool CheckByteLength(std::string_view content) const;

    // 统计 UTF-8 字符数（按 code point 计数，非字节数）
    uint32_t CountUtf8Chars(std::string_view content) const;

    // UTF-8 边界安全截断：不超过 maxBytes 且不切断多字节字符
    std::string TruncateUtf8Safe(std::string_view content, uint32_t maxBytes) const;

    // 过滤控制字符与非法字节，保留可见文本与 \t \n \r
    std::string Sanitize(std::string_view content) const;

    // 按配置净化
    std::string SanitizeWithConfig(std::string_view content, const ValidatorConfig& cfg) const;

    // 是否仅包含空白字符
    bool IsBlankOnly(std::string_view content) const;

    // 是否包含非法 UTF-8 序列
    bool ContainsInvalidUtf8(std::string_view content) const;

    // 是否以 UTF-8 BOM 开头
    bool HasBom(std::string_view content) const;

    // 移除开头的 UTF-8 BOM，无 BOM 时原样返回
    std::string StripBom(std::string_view content) const;

    // 移除零宽字符（ZWSP/ZWNJ/ZWJ/LRM/RLM/WJ/ZWNBSP）
    std::string DropZeroWidth(std::string_view content) const;

    // 字符类别统计
    CharCategoryStat CountCharCategories(std::string_view content) const;

    // 压缩连续重复字符，每字符至多保留 maxRepeat 个；maxRepeat 为 CONFIG_DISABLED 表示原样返回
    std::string CompressRepeats(std::string_view content, uint32_t maxRepeat) const;

    // 合并连续空白为单个空格，保留 \n \r 换行语义
    std::string NormalizeWhitespace(std::string_view content) const;

    // 将字节偏移对齐到其后最近的字符首字节位置
    size_t AlignToCharBoundary(std::string_view content, size_t byteOffset) const;

    // 字节偏移转字符偏移
    size_t ByteOffsetToCharOffset(std::string_view content, size_t byteOffset) const;

    // 字符偏移转字节偏移
    size_t CharOffsetToByteOffset(std::string_view content, size_t charOffset) const;

    // 提取前 N 个字符（按 code point，非字节）
    std::string ExtractFirstNChars(std::string_view content, uint32_t n) const;

    // 计算内容哈希（FNV-1a 64bit），用于去重/缓存键
    uint64_t ComputeHash(std::string_view content) const;

    // 批量校验
    std::vector<SelectionContentValidationResult> ValidateBatch(
        const std::vector<std::string_view>& contents) const;

private:
    // 由首字节推断该 UTF-8 序列字节数，非法首字节返回 UTF8_SEQ_LEN_INVALID
    uint8_t GetUtf8SequenceLength(uint8_t firstByte) const;

    // 是否 UTF-8 续接字节
    bool IsUtf8ContinuationByte(uint8_t b) const;

    // 是否控制字符（排除 \t \n \r）
    bool IsControlChar(uint8_t b) const;

    // 校验从 pos 起长度为 len 的 UTF-8 序列合法性
    bool ValidateUtf8Sequence(std::string_view content, size_t pos, uint8_t len) const;

    // 解码从 pos 起长度为 len 的 UTF-8 序列为码点；非法返回 CP_INVALID
    uint32_t DecodeCodePoint(std::string_view content, size_t pos, uint8_t len) const;

    // 码点是否零宽字符
    bool IsZeroWidthCodePoint(uint32_t cp) const;

    // 码点是否汉字（含扩展 A / 兼容区）
    bool IsHanCodePoint(uint32_t cp) const;

    // 码点是否 ASCII 标点
    bool IsPunctuationCodePoint(uint32_t cp) const;

    // 码点是否 ASCII 字母
    static bool IsAsciiLetterCodePoint(uint32_t cp);

    // 码点是否 ASCII 数字
    static bool IsDigitCodePoint(uint32_t cp);

    // 按配置对单个码点做净化决策
    SanitizeAction DecideSanitizeAction(uint32_t cp, uint8_t lead, uint8_t len,
        const ValidatorConfig& cfg) const;
};

SelectionContentValidator& SelectionContentValidator::GetInstance()
{
    static SelectionContentValidator instance;
    return instance;
}

uint8_t SelectionContentValidator::GetUtf8SequenceLength(uint8_t firstByte) const
{
    if (firstByte < UTF8_CONTINUATION_MIN) {
        return UTF8_SEQ_LEN_ONE; // ASCII
    }
    if ((firstByte & UTF8_LEAD_MASK_TWO) == UTF8_LEAD_VAL_TWO) {
        return UTF8_SEQ_LEN_TWO;
    }
    if ((firstByte & UTF8_LEAD_MASK_THREE) == UTF8_LEAD_VAL_THREE) {
        return UTF8_SEQ_LEN_THREE;
    }
    if ((firstByte & UTF8_LEAD_MASK_FOUR) == UTF8_LEAD_VAL_FOUR) {
        return UTF8_SEQ_LEN_FOUR;
    }
    return UTF8_SEQ_LEN_INVALID; // 非法首字节
}

bool SelectionContentValidator::IsUtf8ContinuationByte(uint8_t b) const
{
    return b >= UTF8_CONTINUATION_MIN && b <= UTF8_CONTINUATION_MAX;
}

bool SelectionContentValidator::IsControlChar(uint8_t b) const
{
    if (b == ALLOWED_HT || b == ALLOWED_LF || b == ALLOWED_CR) {
        return false;
    }
    return b < ASCII_CONTROL_BOUND || b == DEL_BYTE;
}

bool SelectionContentValidator::ValidateUtf8Sequence(std::string_view content, size_t pos, uint8_t len) const
{
    if (len == UTF8_SEQ_LEN_INVALID || pos + len > content.size()) {
        return false;
    }
    uint8_t lead = static_cast<uint8_t>(content[pos]);
    if (len == UTF8_SEQ_LEN_ONE) {
        return lead < UTF8_CONTINUATION_MIN;
    }
    // 首字节落点合法性
    if (len == UTF8_SEQ_LEN_TWO && (lead < UTF8_TWO_BYTE_MIN || lead > UTF8_TWO_BYTE_MAX)) {
        return false;
    }
    if (len == UTF8_SEQ_LEN_THREE && (lead < UTF8_THREE_BYTE_MIN || lead > UTF8_THREE_BYTE_MAX)) {
        return false;
    }
    if (len == UTF8_SEQ_LEN_FOUR && (lead < UTF8_FOUR_BYTE_MIN || lead > UTF8_FOUR_BYTE_MAX)) {
        return false;
    }
    // 续接字节必须落在 UTF8_CONTINUATION_MIN-UTF8_CONTINUATION_MAX
    for (uint8_t i = UTF8_CONT_OFFSET_FIRST; i < len; ++i) {
        uint8_t b = static_cast<uint8_t>(content[pos + i]);
        if (!IsUtf8ContinuationByte(b)) {
            return false;
        }
    }
    return true;
}

uint32_t SelectionContentValidator::DecodeCodePoint(std::string_view content, size_t pos, uint8_t len) const
{
    if (!ValidateUtf8Sequence(content, pos, len)) {
        return CP_INVALID;
    }
    uint8_t b0 = static_cast<uint8_t>(content[pos]);
    if (len == UTF8_SEQ_LEN_ONE) {
        return b0;
    }
    if (len == UTF8_SEQ_LEN_TWO) {
        return (static_cast<uint32_t>(b0 & UTF8_TWO_BYTE_PAYLOAD_MASK) << UTF8_CONTINUATION_BITS) |
               static_cast<uint32_t>(static_cast<uint8_t>(content[pos + UTF8_CONT_OFFSET_FIRST]) &
                   UTF8_CONTINUATION_PAYLOAD_MASK);
    }
    if (len == UTF8_SEQ_LEN_THREE) {
        return (static_cast<uint32_t>(b0 & UTF8_THREE_BYTE_PAYLOAD_MASK) << UTF8_CONT_SHIFT_DOUBLE) |
               (static_cast<uint32_t>(static_cast<uint8_t>(content[pos + UTF8_CONT_OFFSET_FIRST]) &
                   UTF8_CONTINUATION_PAYLOAD_MASK) << UTF8_CONTINUATION_BITS) |
               static_cast<uint32_t>(static_cast<uint8_t>(content[pos + UTF8_CONT_OFFSET_SECOND]) &
                   UTF8_CONTINUATION_PAYLOAD_MASK);
    }
    // len == UTF8_SEQ_LEN_FOUR
    return (static_cast<uint32_t>(b0 & UTF8_FOUR_BYTE_PAYLOAD_MASK) << UTF8_CONT_SHIFT_TRIPLE) |
           (static_cast<uint32_t>(static_cast<uint8_t>(content[pos + UTF8_CONT_OFFSET_FIRST]) &
               UTF8_CONTINUATION_PAYLOAD_MASK) << UTF8_CONT_SHIFT_DOUBLE) |
           (static_cast<uint32_t>(static_cast<uint8_t>(content[pos + UTF8_CONT_OFFSET_SECOND]) &
               UTF8_CONTINUATION_PAYLOAD_MASK) << UTF8_CONTINUATION_BITS) |
           static_cast<uint32_t>(static_cast<uint8_t>(content[pos + UTF8_CONT_OFFSET_THIRD]) &
               UTF8_CONTINUATION_PAYLOAD_MASK);
}

bool SelectionContentValidator::IsZeroWidthCodePoint(uint32_t cp) const
{
    return cp == CP_ZWSP || cp == CP_ZWNJ || cp == CP_ZWJ || cp == CP_LRM || cp == CP_RLM ||
           cp == CP_WJ || cp == CP_ZWNBSP;
}

bool SelectionContentValidator::IsHanCodePoint(uint32_t cp) const
{
    return (cp >= CP_HAN_MIN && cp <= CP_HAN_MAX) ||
           (cp >= CP_HAN_EXT_A_MIN && cp <= CP_HAN_EXT_A_MAX) ||
           (cp >= CP_HAN_COMPAT_MIN && cp <= CP_HAN_COMPAT_MAX);
}

bool SelectionContentValidator::IsPunctuationCodePoint(uint32_t cp) const
{
    return (cp >= ASCII_PUNCT_RANGE1_MIN && cp <= ASCII_PUNCT_RANGE1_MAX) ||
           (cp >= ASCII_PUNCT_RANGE2_MIN && cp <= ASCII_PUNCT_RANGE2_MAX) ||
           (cp >= ASCII_PUNCT_RANGE3_MIN && cp <= ASCII_PUNCT_RANGE3_MAX) ||
           (cp >= ASCII_PUNCT_RANGE4_MIN && cp <= ASCII_PUNCT_RANGE4_MAX);
}

bool SelectionContentValidator::IsAsciiLetterCodePoint(uint32_t cp)
{
    return (cp >= ASCII_UPPER_A && cp <= ASCII_UPPER_Z) ||
           (cp >= ASCII_LOWER_A && cp <= ASCII_LOWER_Z);
}

bool SelectionContentValidator::IsDigitCodePoint(uint32_t cp)
{
    return cp >= ASCII_DIGIT_ZERO && cp <= ASCII_DIGIT_NINE;
}

bool SelectionContentValidator::CheckByteLength(std::string_view content) const
{
    return content.size() <= CONTENT_MAX_BYTES;
}

uint32_t SelectionContentValidator::CountUtf8Chars(std::string_view content) const
{
    uint32_t count = 0;
    size_t pos = 0;
    while (pos < content.size()) {
        uint8_t lead = static_cast<uint8_t>(content[pos]);
        uint8_t len = GetUtf8SequenceLength(lead);
        if (len == UTF8_SEQ_LEN_INVALID) {
            // 非法字节，按单字节跳过
            ++pos;
            continue;
        }
        if (!ValidateUtf8Sequence(content, pos, len)) {
            ++pos;
            continue;
        }
        ++count;
        pos += len;
    }
    return count;
}

std::string SelectionContentValidator::TruncateUtf8Safe(std::string_view content, uint32_t maxBytes) const
{
    if (maxBytes >= content.size()) {
        return std::string(content);
    }
    size_t end = static_cast<size_t>(maxBytes);
    // 若截断点落在多字节字符中间，回退到该字符首字节之前，避免产生不完整序列
    while (end > 0 && IsUtf8ContinuationByte(static_cast<uint8_t>(content[end]))) {
        --end;
    }
    return std::string(content.data(), end);
}

std::string SelectionContentValidator::Sanitize(std::string_view content) const
{
    std::string out;
    out.reserve(content.size());
    size_t pos = 0;
    while (pos < content.size()) {
        uint8_t lead = static_cast<uint8_t>(content[pos]);
        uint8_t len = GetUtf8SequenceLength(lead);
        if (len == UTF8_SEQ_LEN_INVALID || !ValidateUtf8Sequence(content, pos, len)) {
            // 非法字节直接丢弃
            ++pos;
            continue;
        }
        if (len == UTF8_SEQ_LEN_ONE && IsControlChar(lead)) {
            ++pos;
            continue;
        }
        out.append(content.data() + pos, len);
        pos += len;
    }
    return out;
}

SanitizeAction SelectionContentValidator::DecideSanitizeAction(uint32_t cp, uint8_t lead, uint8_t len,
    const ValidatorConfig& cfg) const
{
    if (cfg.dropZeroWidth && IsZeroWidthCodePoint(cp)) {
        return SanitizeAction::DROP;
    }
    if (cfg.dropControlChars && len == UTF8_SEQ_LEN_ONE && IsControlChar(lead)) {
        return SanitizeAction::DROP;
    }
    if (cfg.normalizeWhitespace && (cp == ASCII_SPACE || cp == ALLOWED_HT)) {
        return SanitizeAction::REPLACE_SPACE;
    }
    if (cfg.normalizeWhitespace && (cp == ALLOWED_LF || cp == ALLOWED_CR)) {
        return SanitizeAction::REPLACE_NEWLINE;
    }
    return SanitizeAction::KEEP;
}

std::string SelectionContentValidator::SanitizeWithConfig(std::string_view content,
    const ValidatorConfig& cfg) const
{
    std::string bomStripped;
    if (cfg.stripBom && HasBom(content)) {
        bomStripped = content.substr(BOM_LEN);
        content = bomStripped;
    }
    std::string out;
    out.reserve(content.size());
    size_t pos = 0;
    bool prevIsWs = false;
    while (pos < content.size()) {
        uint8_t lead = static_cast<uint8_t>(content[pos]);
        uint8_t len = GetUtf8SequenceLength(lead);
        if (len == UTF8_SEQ_LEN_INVALID || !ValidateUtf8Sequence(content, pos, len)) {
            ++pos;
            prevIsWs = false;
            continue;
        }
        uint32_t cp = DecodeCodePoint(content, pos, len);
        switch (DecideSanitizeAction(cp, lead, len, cfg)) {
            case SanitizeAction::DROP:
                break;
            case SanitizeAction::REPLACE_SPACE:
                if (!prevIsWs) {
                    out.push_back(static_cast<char>(ASCII_SPACE));
                    prevIsWs = true;
                }
                break;
            case SanitizeAction::REPLACE_NEWLINE:
                if (!prevIsWs) {
                    out.push_back(static_cast<char>(ALLOWED_LF));
                    prevIsWs = true;
                }
                break;
            default:
                out.append(content.data() + pos, len);
                prevIsWs = false;
                break;
        }
        pos += len;
    }
    if (cfg.maxRepeat > CONFIG_DISABLED) {
        out = CompressRepeats(out, cfg.maxRepeat);
    }
    return out;
}

bool SelectionContentValidator::IsBlankOnly(std::string_view content) const
{
    if (content.empty()) {
        return true;
    }
    size_t pos = 0;
    while (pos < content.size()) {
        uint8_t lead = static_cast<uint8_t>(content[pos]);
        uint8_t len = GetUtf8SequenceLength(lead);
        if (len == UTF8_SEQ_LEN_INVALID || !ValidateUtf8Sequence(content, pos, len)) {
            ++pos;
            continue;
        }
        if (len == UTF8_SEQ_LEN_ONE) {
            if (!std::isspace(static_cast<int>(lead)) && lead != ALLOWED_LF && lead != ALLOWED_CR) {
                return false;
            }
        } else {
            // 多字节字符视为非空白（不做全角空格特殊处理）
            return false;
        }
        pos += len;
    }
    return true;
}

bool SelectionContentValidator::ContainsInvalidUtf8(std::string_view content) const
{
    size_t pos = 0;
    while (pos < content.size()) {
        uint8_t lead = static_cast<uint8_t>(content[pos]);
        uint8_t len = GetUtf8SequenceLength(lead);
        if (len == UTF8_SEQ_LEN_INVALID || !ValidateUtf8Sequence(content, pos, len)) {
            return true;
        }
        pos += len;
    }
    return false;
}

bool SelectionContentValidator::HasBom(std::string_view content) const
{
    return content.size() >= BOM_LEN &&
           static_cast<uint8_t>(content[BOM_BYTE_INDEX_0]) == BOM_BYTE0 &&
           static_cast<uint8_t>(content[BOM_BYTE_INDEX_1]) == BOM_BYTE1 &&
           static_cast<uint8_t>(content[BOM_BYTE_INDEX_2]) == BOM_BYTE2;
}

std::string SelectionContentValidator::StripBom(std::string_view content) const
{
    if (HasBom(content)) {
        return std::string(content.substr(BOM_LEN));
    }
    return std::string(content);
}

std::string SelectionContentValidator::DropZeroWidth(std::string_view content) const
{
    std::string out;
    out.reserve(content.size());
    size_t pos = 0;
    while (pos < content.size()) {
        uint8_t lead = static_cast<uint8_t>(content[pos]);
        uint8_t len = GetUtf8SequenceLength(lead);
        if (len == UTF8_SEQ_LEN_INVALID || !ValidateUtf8Sequence(content, pos, len)) {
            ++pos;
            continue;
        }
        uint32_t cp = DecodeCodePoint(content, pos, len);
        if (IsZeroWidthCodePoint(cp)) {
            pos += len;
            continue;
        }
        out.append(content.data() + pos, len);
        pos += len;
    }
    return out;
}

CharCategoryStat SelectionContentValidator::CountCharCategories(std::string_view content) const
{
    CharCategoryStat stat;
    size_t pos = 0;
    while (pos < content.size()) {
        uint8_t lead = static_cast<uint8_t>(content[pos]);
        uint8_t len = GetUtf8SequenceLength(lead);
        if (len == UTF8_SEQ_LEN_INVALID || !ValidateUtf8Sequence(content, pos, len)) {
            ++pos;
            continue;
        }
        uint32_t cp = DecodeCodePoint(content, pos, len);
        if (IsZeroWidthCodePoint(cp)) {
            ++stat.zeroWidth;
        } else if (len == UTF8_SEQ_LEN_ONE && IsControlChar(lead)) {
            ++stat.control;
        } else if (IsAsciiLetterCodePoint(cp)) {
            ++stat.letters;
        } else if (IsDigitCodePoint(cp)) {
            ++stat.digits;
        } else if (IsHanCodePoint(cp)) {
            ++stat.han;
        } else if (IsPunctuationCodePoint(cp)) {
            ++stat.punctuation;
        } else if (cp == ASCII_SPACE || cp == ALLOWED_HT || cp == ALLOWED_LF || cp == ALLOWED_CR) {
            ++stat.whitespace;
        } else {
            ++stat.other;
        }
        ++stat.total;
        pos += len;
    }
    return stat;
}

std::string SelectionContentValidator::CompressRepeats(std::string_view content, uint32_t maxRepeat) const
{
    if (maxRepeat == CONFIG_DISABLED) {
        return std::string(content);
    }
    std::string out;
    out.reserve(content.size());
    size_t pos = 0;
    uint32_t consecutive = REPEAT_COUNT_RESET;
    uint32_t lastCp = UINT32_MAX;
    uint8_t lastLen = UTF8_SEQ_LEN_INVALID;
    while (pos < content.size()) {
        uint8_t lead = static_cast<uint8_t>(content[pos]);
        uint8_t len = GetUtf8SequenceLength(lead);
        if (len == UTF8_SEQ_LEN_INVALID || !ValidateUtf8Sequence(content, pos, len)) {
            ++pos;
            consecutive = REPEAT_COUNT_RESET;
            lastCp = UINT32_MAX;
            continue;
        }
        uint32_t cp = DecodeCodePoint(content, pos, len);
        if (cp == lastCp && len == lastLen) {
            ++consecutive;
        } else {
            consecutive = REPEAT_COUNT_INITIAL;
            lastCp = cp;
            lastLen = len;
        }
        if (consecutive <= maxRepeat) {
            out.append(content.data() + pos, len);
        }
        pos += len;
    }
    return out;
}

std::string SelectionContentValidator::NormalizeWhitespace(std::string_view content) const
{
    std::string out;
    out.reserve(content.size());
    size_t pos = 0;
    bool prevIsWs = false;
    while (pos < content.size()) {
        uint8_t lead = static_cast<uint8_t>(content[pos]);
        uint8_t len = GetUtf8SequenceLength(lead);
        if (len == UTF8_SEQ_LEN_INVALID || !ValidateUtf8Sequence(content, pos, len)) {
            ++pos;
            prevIsWs = false;
            continue;
        }
        uint32_t cp = DecodeCodePoint(content, pos, len);
        if (cp == ASCII_SPACE || cp == ALLOWED_HT) {
            if (!prevIsWs) {
                out.push_back(static_cast<char>(ASCII_SPACE));
                prevIsWs = true;
            }
        } else if (cp == ALLOWED_LF || cp == ALLOWED_CR) {
            if (!prevIsWs) {
                out.push_back(static_cast<char>(ALLOWED_LF));
                prevIsWs = true;
            }
        } else {
            out.append(content.data() + pos, len);
            prevIsWs = false;
        }
        pos += len;
    }
    return out;
}

size_t SelectionContentValidator::AlignToCharBoundary(std::string_view content, size_t byteOffset) const
{
    if (byteOffset >= content.size()) {
        return content.size();
    }
    // 向后回退直到落在非续接字节上
    while (byteOffset > 0 && IsUtf8ContinuationByte(static_cast<uint8_t>(content[byteOffset]))) {
        --byteOffset;
    }
    return byteOffset;
}

size_t SelectionContentValidator::ByteOffsetToCharOffset(std::string_view content, size_t byteOffset) const
{
    size_t charOffset = 0;
    size_t pos = 0;
    size_t target = std::min(byteOffset, content.size());
    while (pos < target) {
        uint8_t lead = static_cast<uint8_t>(content[pos]);
        uint8_t len = GetUtf8SequenceLength(lead);
        if (len == UTF8_SEQ_LEN_INVALID || !ValidateUtf8Sequence(content, pos, len)) {
            ++pos;
            ++charOffset;
            continue;
        }
        pos += len;
        ++charOffset;
    }
    return charOffset;
}

size_t SelectionContentValidator::CharOffsetToByteOffset(std::string_view content, size_t charOffset) const
{
    size_t pos = 0;
    size_t count = 0;
    while (pos < content.size() && count < charOffset) {
        uint8_t lead = static_cast<uint8_t>(content[pos]);
        uint8_t len = GetUtf8SequenceLength(lead);
        if (len == UTF8_SEQ_LEN_INVALID || !ValidateUtf8Sequence(content, pos, len)) {
            ++pos;
            ++count;
            continue;
        }
        pos += len;
        ++count;
    }
    return pos;
}

std::string SelectionContentValidator::ExtractFirstNChars(std::string_view content, uint32_t n) const
{
    if (n == CONFIG_DISABLED) {
        return std::string();
    }
    size_t pos = 0;
    uint32_t count = 0;
    while (pos < content.size() && count < n) {
        uint8_t lead = static_cast<uint8_t>(content[pos]);
        uint8_t len = GetUtf8SequenceLength(lead);
        if (len == UTF8_SEQ_LEN_INVALID || !ValidateUtf8Sequence(content, pos, len)) {
            ++pos;
            ++count;
            continue;
        }
        pos += len;
        ++count;
    }
    return std::string(content.data(), pos);
}

uint64_t SelectionContentValidator::ComputeHash(std::string_view content) const
{
    uint64_t hash = FNV_OFFSET_BASIS;
    for (size_t i = 0; i < content.size(); ++i) {
        hash ^= static_cast<uint64_t>(static_cast<uint8_t>(content[i]));
        hash *= FNV_PRIME;
    }
    return hash;
}

std::vector<SelectionContentValidationResult> SelectionContentValidator::ValidateBatch(
    const std::vector<std::string_view>& contents) const
{
    std::vector<SelectionContentValidationResult> results;
    results.reserve(contents.size());
    for (const auto& item : contents) {
        results.push_back(Validate(item));
    }
    return results;
}

SelectionContentValidationResult SelectionContentValidator::Validate(std::string_view content) const
{
    SelectionContentValidationResult result;
    result.byteLen = static_cast<uint32_t>(content.size());
    result.charLen = CountUtf8Chars(content);

    if (content.empty() || IsBlankOnly(content)) {
        SELECTION_HILOGW("content is empty or blank-only, byteLen=%{public}u", result.byteLen);
        result.code = CANNOT_GET_CONTENT;
        return result;
    }
    if (ContainsInvalidUtf8(content)) {
        SELECTION_HILOGW("content contains invalid utf8 sequence, byteLen=%{public}u", result.byteLen);
        result.code = INVALID_DATA;
        return result;
    }
    if (!CheckByteLength(content)) {
        SELECTION_HILOGW("content exceeds max bytes, byteLen=%{public}u max=%{public}u",
            result.byteLen, CONTENT_MAX_BYTES);
        result.code = CONTENT_OUT_OF_RANGE;
        result.truncated = true;
        return result;
    }
    result.code = INVALID_DATA; // 默认占位，调用方按需覆盖
    return result;
}

SelectionContentValidationResult SelectionContentValidator::ValidateWithConfig(std::string_view content,
    const ValidatorConfig& cfg) const
{
    SelectionContentValidationResult result;
    result.byteLen = static_cast<uint32_t>(content.size());
    result.charLen = CountUtf8Chars(content);

    if (content.empty() || IsBlankOnly(content)) {
        SELECTION_HILOGW("content is empty or blank-only, byteLen=%{public}u", result.byteLen);
        result.code = CANNOT_GET_CONTENT;
        return result;
    }
    if (ContainsInvalidUtf8(content)) {
        SELECTION_HILOGW("content contains invalid utf8 sequence, byteLen=%{public}u", result.byteLen);
        result.code = INVALID_DATA;
        return result;
    }
    if (cfg.maxBytes != CONFIG_DISABLED && content.size() > cfg.maxBytes) {
        SELECTION_HILOGW("content exceeds configured max bytes, byteLen=%{public}u max=%{public}u",
            result.byteLen, cfg.maxBytes);
        result.code = CONTENT_OUT_OF_RANGE;
        result.truncated = true;
        return result;
    }
    if (cfg.maxChars != CONFIG_DISABLED && result.charLen > cfg.maxChars) {
        SELECTION_HILOGW("content exceeds configured max chars, charLen=%{public}u max=%{public}u",
            result.charLen, cfg.maxChars);
        result.code = CONTENT_OUT_OF_RANGE;
        result.truncated = true;
        return result;
    }
    result.code = INVALID_DATA; // 默认占位，调用方按需覆盖
    return result;
}

} // namespace SelectionFwk
} // namespace OHOS
