#include "SecurityStatus.h"

bool SecurityStatus::isReady(QString *error) {
    if (error) *error = "Sandbox and signed entitlements must be checked in the packaged app";
    return false;
}
