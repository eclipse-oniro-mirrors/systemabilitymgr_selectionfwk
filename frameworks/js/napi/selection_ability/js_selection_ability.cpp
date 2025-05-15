#include "js_selection_ability.h"

#include "napi/native_node_api.h"
#include "selection_log.h"

using namespace OHOS::SelectionFwk;

napi_value JsSelectionAbility::GetSelectionAbility(napi_env env, napi_callback_info info)
{
    SELECTION_HILOGI("GetSelectionAbility");
    return nullptr;
}

napi_value JsSelectionAbility::Init(napi_env env, napi_value exports)
{
    SELECTION_HILOGI("napi init");
    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("getSelectionAbility", GetSelectionAbility),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(properties) / sizeof(properties[0]), properties));
    return exports;
}