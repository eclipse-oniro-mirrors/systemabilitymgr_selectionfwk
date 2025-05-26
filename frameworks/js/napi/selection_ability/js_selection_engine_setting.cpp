#include "js_selection_engine_setting.h"

#include "event_checker.h"
#include "iservice_registry.h"
#include "js_utils.h"
#include "callback_handler.h"
#include "napi/native_node_api.h"
#include "selection_listener_impl.h"
#include "selection_log.h"
#include "system_ability_definition.h"
#include "util.h"

using namespace OHOS;
using namespace OHOS::SelectionFwk;

constexpr size_t ARGC_ONE = 1;
constexpr size_t ARGC_TWO = 2;

const std::string JsSelectionEngineSetting::KDS_CLASS_NAME = "SelectionAbility";
thread_local napi_ref JsSelectionEngineSetting::KDSRef_ = nullptr;
std::mutex JsSelectionEngineSetting::selectionMutex_;
std::shared_ptr<JsSelectionEngineSetting> JsSelectionEngineSetting::selectionDelegate_{ nullptr };
std::mutex JsSelectionEngineSetting::eventHandlerMutex_;
std::shared_ptr<AppExecFwk::EventHandler> JsSelectionEngineSetting::handler_{ nullptr };


napi_value JsSelectionEngineSetting::GetSelectionAbility(napi_env env, napi_callback_info info)
{
    SELECTION_HILOGI("SelectionEngineSetting---GetSelectionAbility");

    return GetSEInstance(env, info);
}

napi_value JsSelectionEngineSetting::Subscribe(napi_env env, napi_callback_info info)
{
    size_t argc = ARGC_TWO;
    napi_value argv[ARGC_TWO] = { nullptr };
    napi_value thisVar = nullptr;
    void *data = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, &data));
    std::string type;
    // 2 means least param num.
    if (argc < 2 || !JsUtil::GetValue(env, argv[0], type) ||
        !EventChecker::IsValidEventType(EventSubscribeModule::SELECTION_METHOD_ABILITY, type) ||
        JsUtil::GetType(env, argv[1]) != napi_function) {
        SELECTION_HILOGE("subscribe failed, type: %{public}s!", type.c_str());
        return nullptr;
    }
    SELECTION_HILOGE("subscribe type: %{public}s.", type.c_str());
    auto engine = reinterpret_cast<JsSelectionEngineSetting *>(JsUtils::GetNativeSelf(env, info));
    if (engine == nullptr) {
        return nullptr;
    }
    std::shared_ptr<JSCallbackObject> callback =
        std::make_shared<JSCallbackObject>(env, argv[1], std::this_thread::get_id(),
            AppExecFwk::EventHandler::Current());
    engine->RegisterListener(argv[ARGC_ONE], type, callback);

    napi_value result = nullptr;
    napi_get_null(env, &result);
    return result;
}


napi_value JsSelectionEngineSetting::UnSubscribe(napi_env env, napi_callback_info info)
{
    size_t argc = ARGC_TWO;
    napi_value argv[ARGC_TWO] = { nullptr };
    napi_value thisVar = nullptr;
    void *data = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, &data));
    std::string type;
    // 1 means least param num.
    if (argc < 1 || !JsUtil::GetValue(env, argv[0], type) ||
        !EventChecker::IsValidEventType(EventSubscribeModule::SELECTION_METHOD_ABILITY, type)) {
        SELECTION_HILOGE("unsubscribe failed, type: %{public}s!", type.c_str());
        return nullptr;
    }

    // if the second param is not napi_function/napi_null/napi_undefined, return
    auto paramType = JsUtil::GetType(env, argv[1]);
    if (paramType != napi_function && paramType != napi_null && paramType != napi_undefined) {
        return nullptr;
    }
    // if the second param is napi_function, delete it, else delete all
    argv[1] = paramType == napi_function ? argv[1] : nullptr;

    SELECTION_HILOGD("unsubscribe type: %{public}s.", type.c_str());
    auto delegate = reinterpret_cast<JsSelectionEngineSetting *>(JsUtils::GetNativeSelf(env, info));
    if (delegate == nullptr) {
        return nullptr;
    }
    delegate->UnRegisterListener(argv[ARGC_ONE], type);
    napi_value result = nullptr;
    napi_get_null(env, &result);
    return result;
}

napi_value JsSelectionEngineSetting::CreatePanel(napi_env env, napi_callback_info info)
{
    SELECTION_HILOGI("SelectionEngineSetting---CreatePanel");

    return nullptr;
}

sptr<ISelectionService> JsSelectionEngineSetting::GetSelectionSystemAbility()
{
    auto systemAbilityManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        SELECTION_HILOGE("system ability manager is nullptr!");
        return nullptr;
    }

    sptr<IRemoteObject> systemAbility = nullptr;
    systemAbility = systemAbilityManager->GetSystemAbility(SELECTION_FWK_SA_ID);
    if (systemAbility == nullptr) {
        SELECTION_HILOGE("get system ability is nullptr!");
        return nullptr;
    }

    abilityManager_ = iface_cast<ISelectionService>(systemAbility);
    return abilityManager_;
}

void JsSelectionEngineSetting::RegisterListener(napi_value callback, std::string type,
    std::shared_ptr<JSCallbackObject> callbackObj)
{
    SELECTION_HILOGD("RegisterListener %{public}s", type.c_str());
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (jsCbMap_.empty() || jsCbMap_.find(type) == jsCbMap_.end()) {
        SELECTION_HILOGD("methodName %{public}s is not registered!", type.c_str());
    }
    auto callbacks = jsCbMap_[type];
    bool ret = std::any_of(callbacks.begin(), callbacks.end(), [&callback](std::shared_ptr<JSCallbackObject> cb) {
        return JsUtils::Equals(cb->env_, callback, cb->callback_, cb->threadId_);
    });
    if (ret) {
        SELECTION_HILOGD("JsSelectionEngineSetting callback already registered!");
        return;
    }

    SELECTION_HILOGI("add %{public}s callbackObj into jsCbMap_.", type.c_str());
    jsCbMap_[type].push_back(std::move(callbackObj));

    auto proxy = GetSelectionSystemAbility();
    if (proxy == nullptr) {
        SELECTION_HILOGE("selection system ability is nullptr!");
        return;
    }
    auto selectionInterface = GetJsSelectionEngineSetting();
    listenerStub_ = new (std::nothrow) SelectionListenerImpl(selectionInterface);
    SELECTION_HILOGI("Begin to call SA RegisterListener");
    proxy->RegisterListener(listenerStub_->AsObject());
}

void JsSelectionEngineSetting::UnRegisterListener(napi_value callback, std::string type)
{
    SELECTION_HILOGI("unregister listener: %{public}s.", type.c_str());
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (jsCbMap_.empty() || jsCbMap_.find(type) == jsCbMap_.end()) {
        SELECTION_HILOGE("methodName %{public}s is not unregistered!", type.c_str());
        return;
    }

    if (callback == nullptr) {
        jsCbMap_.erase(type);
        SELECTION_HILOGE("callback is nullptr!");
        return;
    }

    for (auto item = jsCbMap_[type].begin(); item != jsCbMap_[type].end(); item++) {
        if ((callback != nullptr) &&
            (JsUtils::Equals((*item)->env_, callback, (*item)->callback_, (*item)->threadId_))) {
            jsCbMap_[type].erase(item);
            break;
        }
    }
    if (jsCbMap_[type].empty()) {
        jsCbMap_.erase(type);
    }

    auto proxy = GetSelectionSystemAbility();
    if (proxy == nullptr || listenerStub_ == nullptr) {
        SELECTION_HILOGE("selection system ability or listenerStub_ is nullptr!");
        return;
    }
    proxy->UnregisterListener(listenerStub_->AsObject());
    listenerStub_ = nullptr;
}

napi_value JsSelectionEngineSetting::GetSEInstance(napi_env env, napi_callback_info info)
{
    napi_value instance = nullptr;
    napi_value cons = nullptr;
    if (napi_get_reference_value(env, KDSRef_, &cons) != napi_ok) {
        SELECTION_HILOGI("failed to get reference value.");
        return nullptr;
    }
    if (napi_new_instance(env, cons, 0, nullptr, &instance) != napi_ok) {
        SELECTION_HILOGI("failed to new instance.");
        return nullptr;
    }
    return instance;
}

std::shared_ptr<JsSelectionEngineSetting> JsSelectionEngineSetting::GetJsSelectionEngineSetting()
{
    if (selectionDelegate_ == nullptr) {
        std::lock_guard<std::mutex> lock(selectionMutex_);
        if (selectionDelegate_ == nullptr) {
            auto delegate = std::make_shared<JsSelectionEngineSetting>();
            if (delegate == nullptr) {
                SELECTION_HILOGE("keyboard delegate is nullptr!");
                return nullptr;
            }
            selectionDelegate_ = delegate;
        }
    }
    {
        std::lock_guard<std::mutex> lock(eventHandlerMutex_);
        handler_ = AppExecFwk::EventHandler::Current();
    }
    return selectionDelegate_;
}

napi_value JsSelectionEngineSetting::JsConstructor(napi_env env, napi_callback_info cbinfo)
{
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, cbinfo, nullptr, nullptr, &thisVar, nullptr));
    auto delegate = GetJsSelectionEngineSetting();
    if (delegate == nullptr) {
        SELECTION_HILOGE("failed to get delegate!");
        napi_value result = nullptr;
        napi_get_null(env, &result);
        return result;
    }
    napi_status status = napi_wrap(
        env, thisVar, delegate.get(), [](napi_env env, void *nativeObject, void *hint) {}, nullptr, nullptr);
    if (status != napi_ok) {
        SELECTION_HILOGE("failed to wrap: %{public}d!", status);
        return nullptr;
    }
    return thisVar;
};

napi_value JsSelectionEngineSetting::Init(napi_env env, napi_value exports)
{
    SELECTION_HILOGI("napi init");
    napi_property_descriptor descriptor[] = {
        DECLARE_NAPI_FUNCTION("getSelectionAbility", GetSelectionAbility),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(descriptor) / sizeof(napi_property_descriptor), descriptor));
    return InitProperty(env, exports);
}

napi_value JsSelectionEngineSetting::InitProperty(napi_env env, napi_value exports)
{
    SELECTION_HILOGI("napi init");
    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("on", Subscribe),
        DECLARE_NAPI_FUNCTION("off", UnSubscribe),
        DECLARE_NAPI_FUNCTION("createPanel", CreatePanel),
    };

    napi_value cons = nullptr;
    NAPI_CALL(env, napi_define_class(env, KDS_CLASS_NAME.c_str(), KDS_CLASS_NAME.size(), JsConstructor, nullptr,
                       sizeof(properties) / sizeof(napi_property_descriptor), properties, &cons));
    NAPI_CALL(env, napi_create_reference(env, cons, 1, &KDSRef_));
    NAPI_CALL(env, napi_set_named_property(env, exports, KDS_CLASS_NAME.c_str(), cons));
    return exports;
}

std::shared_ptr<AppExecFwk::EventHandler> JsSelectionEngineSetting::GetEventHandler()
{
    std::lock_guard<std::mutex> lock(eventHandlerMutex_);
    return handler_;
}

std::shared_ptr<JsSelectionEngineSetting::SelectionEntry> JsSelectionEngineSetting::GetEntry(const std::string &type,
    EntrySetter entrySetter)
{
    SELECTION_HILOGI("start, type: %{public}s", type.c_str());
    std::shared_ptr<SelectionEntry> entry = nullptr;
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        if (jsCbMap_[type].empty()) {
            SELECTION_HILOGI("%{public}s cb-vector is empty.", type.c_str());
            return nullptr;
        }
        entry = std::make_shared<SelectionEntry>(jsCbMap_[type], type);
    }
    if (entrySetter != nullptr) {
        entrySetter(*entry);
    }
    return entry;
}

int32_t JsSelectionEngineSetting::OnSelectionEvent(const std::string &selectionData)
{
    SELECTION_HILOGI("OnSelectionEvent begin");
    std::string type = "selectionEvent";

    if (selectionData.empty()) {
        SELECTION_HILOGE("selectionData is empty");
        return 1;
    }

    auto entry = GetEntry(type, [&selectionData](SelectionEntry &entry) { entry.text = selectionData; });
    if (entry == nullptr) {
        SELECTION_HILOGE("failed to get uv entry!");
        return 1;
    }

    auto eventHandler = GetEventHandler();
    if (eventHandler == nullptr) {
        SELECTION_HILOGE("eventHandler is nullptr!");
        return 1;
    }

    SELECTION_HILOGI("selectionData is [%{public}s]", selectionData.c_str());


    auto task = [entry]() {
        auto getTextChangeProperty = [entry](napi_env env, napi_value *args, uint8_t argc) -> bool {
            if (argc == 0) {
                return false;
            }
            // 0 means the first param of callback.
            napi_create_string_utf8(env, entry->text.c_str(), NAPI_AUTO_LENGTH, &args[0]);
            return true;
        };
        // 1 means callback has one param.
        JsCallbackHandler::Traverse(entry->vecCopy, { 1, getTextChangeProperty });
    };
    eventHandler->PostTask(task, type, 0, AppExecFwk::EventQueue::Priority::VIP);

    SELECTION_HILOGI("OnSelectionEvent end");
    return 0;
}

