#include "SecurityStatus.h"

#ifdef Q_OS_MACOS
#include <Security/Security.h>
#endif

bool SecurityStatus::isReady(QString *error) {
#ifdef Q_OS_MACOS
    CFErrorRef taskError = nullptr;
    SecTaskRef task = SecTaskCreateFromSelf(kCFAllocatorDefault);
    if (!task) {
        if (error) *error = "The current process security task is unavailable";
        return false;
    }

    const auto hasEntitlement = [&](CFStringRef name) {
        CFTypeRef value = SecTaskCopyValueForEntitlement(task, name, &taskError);
        const bool enabled = value != nullptr && CFGetTypeID(value) == CFBooleanGetTypeID() &&
                             CFBooleanGetValue(static_cast<CFBooleanRef>(value));
        if (value) CFRelease(value);
        return enabled;
    };

    const bool sandboxed = hasEntitlement(CFSTR("com.apple.security.app-sandbox"));
    const bool fileAccess = hasEntitlement(CFSTR("com.apple.security.files.user-selected.read-write"));
    if (taskError) CFRelease(taskError);
    CFRelease(task);

    if (!sandboxed || !fileAccess) {
        if (error) *error = "Required App Sandbox entitlements are not active";
        return false;
    }
    return true;
#else
    if (error) *error = "Runtime App Sandbox verification is only supported on macOS";
    return false;
#endif
}
