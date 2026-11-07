#include "FileAccess.h"

#include <QFile>

bool FileAccess::readTextFile(const QString &path, QString *text, QString *error) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error) *error = "Selected text file cannot be opened";
        return false;
    }
    if (text) *text = QString::fromUtf8(file.readAll());
    return true;
}

bool FileAccess::writeTextFile(const QString &path, const QString &text, QString *error) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)
        || file.write(text.toUtf8()) != text.toUtf8().size()) {
        if (error) *error = "Translation cannot be saved";
        return false;
    }
    return true;
}
