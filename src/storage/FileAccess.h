#pragma once

#include <QString>

class QTextEdit;

class FileAccess final {
public:
    static bool readTextFile(const QString &path, QString *text, QString *error);
    static bool writeTextFile(const QString &path, const QString &text, QString *error);
};
