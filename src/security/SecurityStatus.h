#pragma once

#include <QString>

class SecurityStatus final {
public:
    static bool isReady(QString *error);
};
