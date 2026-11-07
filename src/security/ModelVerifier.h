#pragma once

#include <QString>
#include <QStringList>

class ModelVerifier final {
public:
    bool verify(const QString &path, const QStringList &approvedSha256,
                QString *actualSha256, QString *error) const;
    bool verifyManifest(const QString &path, const QString &manifestPath,
                        QString *actualSha256, QString *error) const;
};
