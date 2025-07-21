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

#include "selection_js_utils.h"

namespace OHOS {
namespace SelectionFwk {
constexpr int32_t STR_MAX_LENGTH = 4096;
constexpr size_t STR_TAIL_LENGTH = 1;
constexpr size_t ARGC_MAX = 6;

const std::map<int32_t, int32_t> JsUtils::ERROR_CODE_MAP = {
    { ErrorCode::ERROR_PARAMETER_CHECK_FAILED, EXCEPTION_PARAMCHECK },
    { ErrorCode::ERROR_SELECTION_SERVICE, EXCEPTION_SELECTION_SERVICE },
    { ErrorCode::ERROR_PANEL_DESTROYED, EXCEPTION_PANEL_DESTROYED },
    { ErrorCode::ERROR_INVALID_OPERATION, EXCEPTION_INVALID_OPERATION }
};

const std::map<int32_t, std::string> JsUtils::ERROR_CODE_CONVERT_MESSAGE_MAP = {
    { EXCEPTION_PARAMCHECK, "The parameters check fails." },
    { EXCEPTION_SELECTION_SERVICE, "Selection service exception." },
    { EXCEPTION_PANEL_DESTROYED, "This selection window has been destroyed." },
    { EXCEPTION_INVALID_OPERATION, "Invalid operation. The selection app is not valid." }
};

const std::map<int32_t, std::string> JsUtils::PARAMETER_TYPE = {
    { TYPE_UNDEFINED, "napi_undefine." },
    { TYPE_NULL, "napi_null." },
    { TYPE_BOOLEAN, "napi_boolean." },
    { TYPE_NUMBER, "napi_number." },
    { TYPE_STRING, "napi_string." },
    { TYPE_SYMBOL, "napi_symbol." },
    { TYPE_OBJECT, "napi_object." },
    { TYPE_FUNCTION, "napi_function." },
    { TYPE_EXTERNAL, "napi_external." },
    { TYPE_BIGINT, "napi_bigint." },
    { TYPE_ARRAY_BUFFER, "ArrayBuffer." },
    { TYPE_ARRAY, "napi_array." },
};

void JsUtils::ThrowException(napi_env env, int32_t err, const std::string &msg, TypeCode type)
{
    std::string errMsg = ToMessage(err);
    napi_value error;
    napi_value code;
    napi_value message;
    if (type == TypeCode::TYPE_NONE) {
        errMsg = errMsg + " " + msg;
        SELECTION_HILOGE("THROW_ERROR message: %{public}s!", errMsg.c_str());
    } else {
        auto iter = PARAMETER_TYPE.find(type);
        if (iter != PARAMETER_TYPE.end()) {
            errMsg = errMsg + "The type of " + msg + " must be " + iter->second;
            SELECTION_HILOGE("THROW_ERROR message: %{public}s!", errMsg.c_str());
        }
    }
    NAPI_CALL_RETURN_VOID(env, napi_create_string_utf8(env, errMsg.c_str(), NAPI_AUTO_LENGTH, &message));
    NAPI_CALL_RETURN_VOID(env, napi_create_error(env, nullptr, message, &error));
    NAPI_CALL_RETURN_VOID(env, napi_create_int32(env, err, &code));
    NAPI_CALL_RETURN_VOID(env, napi_set_named_property(env, error, "code", code));
    NAPI_CALL_RETURN_VOID(env, napi_throw(env, error));
}

napi_value JsUtils::ToError(napi_env env, int32_t code, const std::string &msg)
{
    SELECTION_HILOGD("ToError start");
    napi_value errorObj;
    NAPI_CALL(env, napi_create_object(env, &errorObj));
    napi_value errorCode = nullptr;
    NAPI_CALL(env, napi_create_int32(env, Convert(code), &errorCode));
    napi_value errorMessage = nullptr;
    std::string errMsg = ToMessage(Convert(code)) + " " + msg;
    NAPI_CALL(env, napi_create_string_utf8(env, errMsg.c_str(), NAPI_AUTO_LENGTH, &errorMessage));
    NAPI_CALL(env, napi_set_named_property(env, errorObj, "code", errorCode));
    NAPI_CALL(env, napi_set_named_property(env, errorObj, "message", errorMessage));
    SELECTION_HILOGD("ToError end");
    return errorObj;
}

int32_t JsUtils::Convert(int32_t code)
{
    SELECTION_HILOGD("Convert start.");
    auto iter = ERROR_CODE_MAP.find(code);
    if (iter != ERROR_CODE_MAP.end()) {
        SELECTION_HILOGD("ErrorCode: %{public}d", iter->second);
        return iter->second;
    }
    SELECTION_HILOGD("Convert end.");
    return ERROR_CODE_QUERY_FAILED;
}

const std::string JsUtils::ToMessage(int32_t code)
{
    SELECTION_HILOGD("ToMessage start");
    auto iter = ERROR_CODE_CONVERT_MESSAGE_MAP.find(code);
    if (iter != ERROR_CODE_CONVERT_MESSAGE_MAP.end()) {
        SELECTION_HILOGD("ErrorMessage: %{public}s", (iter->second).c_str());
        return iter->second;
    }
    return "error is out of definition.";
}

bool JsUtils::Equals(napi_env env, napi_value value, napi_ref copy, std::thread::id threadId)
{
    if (copy == nullptr) {
        return value == nullptr;
    }

    if (threadId != std::this_thread::get_id()) {
        SELECTION_HILOGD("napi_value can not be compared");
        return false;
    }

    napi_value copyValue = nullptr;
    napi_get_reference_value(env, copy, &copyValue);

    bool isEquals = false;
    napi_strict_equals(env, value, copyValue, &isEquals);
    SELECTION_HILOGD("value compare result: %{public}d", isEquals);
    return isEquals;
}

void *JsUtils::GetNativeSelf(napi_env env, napi_callback_info info)
{
    size_t argc = ARGC_MAX;
    void *native = nullptr;
    napi_value self = nullptr;
    napi_value argv[ARGC_MAX] = { nullptr };
    napi_status status = napi_invalid_arg;
    napi_get_cb_info(env, info, &argc, argv, &self, nullptr);
    CHECK_RETURN((self != nullptr && argc <= ARGC_MAX), "napi_get_cb_info failed!", nullptr);

    status = napi_unwrap(env, self, &native);
    CHECK_RETURN((status == napi_ok && native != nullptr), "napi_unwrap failed!", nullptr);
    return native;
}

napi_status JsUtils::GetValue(napi_env env, napi_value in, int32_t &out)
{
    SELECTION_HILOGD("napi_value -> int32_t ");
    napi_valuetype type = napi_undefined;
    napi_status status = napi_typeof(env, in, &type);
    CHECK_RETURN((status == napi_ok) && (type == napi_number), "invalid type", napi_generic_failure);
    return napi_get_value_int32(env, in, &out);
}

/* napi_value <-> uint32_t */
napi_status JsUtils::GetValue(napi_env env, napi_value in, uint32_t &out)
{
    napi_valuetype type = napi_undefined;
    napi_status status = napi_typeof(env, in, &type);
    CHECK_RETURN((status == napi_ok) && (type == napi_number), "invalid type", napi_generic_failure);
    return napi_get_value_uint32(env, in, &out);
}

napi_status JsUtils::GetValue(napi_env env, napi_value in, bool &out)
{
    napi_valuetype type = napi_undefined;
    napi_status status = napi_typeof(env, in, &type);
    CHECK_RETURN((status == napi_ok) && (type == napi_boolean), "invalid type", napi_generic_failure);
    return napi_get_value_bool(env, in, &out);
}

napi_status JsUtils::GetValue(napi_env env, napi_value in, double &out)
{
    napi_valuetype type = napi_undefined;
    napi_status status = napi_typeof(env, in, &type);
    CHECK_RETURN((status == napi_ok) && (type == napi_number), "invalid double type", napi_generic_failure);
    return napi_get_value_double(env, in, &out);
}

/* napi_value <-> std::string */
napi_status JsUtils::GetValue(napi_env env, napi_value in, std::string &out)
{
    SELECTION_HILOGD("JsUtils get string value in.");
    napi_valuetype type = napi_undefined;
    napi_status status = napi_typeof(env, in, &type);
    CHECK_RETURN((status == napi_ok) && (type == napi_string), "invalid type", napi_generic_failure);

    size_t maxLen = STR_MAX_LENGTH;
    status = napi_get_value_string_utf8(env, in, NULL, 0, &maxLen);
    if (maxLen <= 0) {
        return status;
    }
    SELECTION_HILOGD("napi_value -> std::string get length %{public}zu", maxLen);
    char *buf = new (std::nothrow) char[maxLen + STR_TAIL_LENGTH];
    if (buf != nullptr) {
        size_t len = 0;
        status = napi_get_value_string_utf8(env, in, buf, maxLen + STR_TAIL_LENGTH, &len);
        if (status == napi_ok) {
            buf[len] = 0;
            out = std::string(buf);
        }
        delete[] buf;
    } else {
        status = napi_generic_failure;
    }
    return status;
}

/* napi_value <-> std::unordered_map<string, string> */

napi_status JsUtils::GetValue(napi_env env, napi_value in, const std::string &type, napi_value &out)
{
    napi_valuetype valueType = napi_undefined;
    napi_status status = napi_typeof(env, in, &valueType);
    if ((status == napi_ok) && (valueType == napi_object)) {
        status = napi_get_named_property(env, in, type.c_str(), &out);
        return status;
    }
    return napi_generic_failure;
}

napi_status JsUtils::GetValue(napi_env env, const std::string &in, napi_value &out)
{
    return napi_create_string_utf8(env, in.c_str(), in.size(), &out);
}

napi_value JsUtils::GetValue(napi_env env, const std::vector<uint8_t> &in)
{
    void *data = nullptr;
    napi_value arrayBuffer = nullptr;
    size_t length = in.size();
    NAPI_CALL(env, napi_create_arraybuffer(env, length, &data, &arrayBuffer));
    // 0 means the size of data.
    CHECK_RETURN(length != 0, "Data size is 0.", arrayBuffer);
    if (memcpy_s(data, length, reinterpret_cast<const void *>(in.data()), length) != 0) {
        return nullptr;
    }
    return arrayBuffer;
}

napi_status JsUtils::GetValue(napi_env env, napi_value in, std::vector<uint8_t> &out)
{
    size_t length = 0;
    void *data = nullptr;
    auto status = napi_get_arraybuffer_info(env, in, &data, &length);
    if (status != napi_ok) {
        SELECTION_HILOGE("Get ArrayBuffer info failed!");
        return status;
    }
    if (data == nullptr && length == 0) {
        SELECTION_HILOGE("Empty ArrayBuffer.");
        out.clear();
        return napi_ok;
    }
    if (data == nullptr) {
        SELECTION_HILOGE("ArrayBuffer data is nullptr!");
        return napi_generic_failure;
    }
    SELECTION_HILOGD("ArrayBuffer data size: %{public}zu.", length);
    out.assign(reinterpret_cast<const uint8_t *>(data), reinterpret_cast<const uint8_t *>(data) + length);
    return napi_ok;
}

} // namespace SelectionFwk
} // namespace OHOS