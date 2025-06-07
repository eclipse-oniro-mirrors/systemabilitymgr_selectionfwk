#ifndef PASTEBOARD_DISPOSABLE_OBSERVER_H
#define PASTEBOARD_DISPOSABLE_OBSERVER_H

#include <string>

namespace OHOS {
namespace MiscServices {
class PasteboardDisposableObserver {
public:
    virtual void OnTextReceived(const std::string &text, int32_t errCode) = 0;
};
} // namespace MiscServices
} // namespace OHOS
#endif // PASTEBOARD_DISPOSABLE_OBSERVER_H