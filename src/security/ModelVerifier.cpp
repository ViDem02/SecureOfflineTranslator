#include "ModelVerifier.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

bool ModelVerifier::verify(const QString &path, const QStringList &approvedSha256,
                           QString *actualSha256, QString *error) const {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = "Model file cannot be opened: " + path;
        return false;
    }
    QCryptographicHash hasher(QCryptographicHash::Sha256);
    while (!file.atEnd()) {
        const auto chunk = file.read(1024 * 1024);
        if (chunk.isEmpty() && file.error() != QFile::NoError) {
            if (error) *error = "Model file cannot be read: " + path;
            return false;
        }
        hasher.addData(chunk);
    }
    const auto hash = hasher.result().toHex();
    if (actualSha256) *actualSha256 = QString::fromLatin1(hash);
    const auto actual = QString::fromLatin1(hash);
    if (!approvedSha256.contains(actual, Qt::CaseInsensitive)) {
        if (error) {
            *error = "Model SHA-256 is not approved (actual: " + actual + ")";
        }
        return false;
    }
    return true;
}

bool ModelVerifier::verifyManifest(const QString &path, const QString &manifestPath,
                                   QString *actualSha256, QString *error) const {
    QFile manifest(manifestPath);
    if (!manifest.open(QIODevice::ReadOnly)) {
        if (error) *error = "Approved model manifest cannot be opened: " + manifestPath;
        return false;
    }
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(manifest.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error) *error = "Approved model manifest is invalid";
        return false;
    }
    const auto entries = document.object().value("models").toArray();
    QStringList approved;
    const auto fileName = QFileInfo(path).fileName();
    for (const auto &entry : entries) {
        const auto object = entry.toObject();
        if (object.value("file").toString() == fileName) {
            approved.append(object.value("sha256").toString().toLower());
        }
    }
    if (approved.isEmpty()) {
        if (error) *error = "Model is not present in the approved manifest";
        return false;
    }
    return verify(path, approved, actualSha256, error);
}
