#include "security/ModelVerifier.h"

#include <QCryptographicHash>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class ModelVerifierTests final : public QObject {
    Q_OBJECT

private slots:
    void acceptsApprovedHash();
    void rejectsTamperedFile();
    void rejectsManifestFilenameMismatch();
};

void ModelVerifierTests::acceptsApprovedHash() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath("model.gguf");
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    const QByteArray content("approved model bytes");
    QCOMPARE(file.write(content), content.size());
    file.close();

    const auto hash = QCryptographicHash::hash(content, QCryptographicHash::Sha256).toHex();
    QString actual;
    QString error;
    QVERIFY(ModelVerifier().verify(path, {QString::fromLatin1(hash)}, &actual, &error));
    QCOMPARE(actual, QString::fromLatin1(hash));
    QVERIFY(error.isEmpty());
}

void ModelVerifierTests::rejectsTamperedFile() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath("model.gguf");
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QVERIFY(file.write("tampered model bytes") > 0);
    file.close();

    QString actual;
    QString error;
    QVERIFY(!ModelVerifier().verify(path, {"00"}, &actual, &error));
    QVERIFY(error.contains("not approved"));
    QVERIFY(!actual.isEmpty());
}

void ModelVerifierTests::rejectsManifestFilenameMismatch() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto modelPath = directory.filePath("unexpected.gguf");
    QFile model(modelPath);
    QVERIFY(model.open(QIODevice::WriteOnly));
    QVERIFY(model.write("model") > 0);
    model.close();

    const auto manifestPath = directory.filePath("approved-models.json");
    QFile manifest(manifestPath);
    QVERIFY(manifest.open(QIODevice::WriteOnly));
    QVERIFY(manifest.write(R"({"models":[{"file":"expected.gguf","sha256":"00"}]})") > 0);
    manifest.close();

    QString actual;
    QString error;
    QVERIFY(!ModelVerifier().verifyManifest(modelPath, manifestPath, &actual, &error));
    QVERIFY(error.contains("not present"));
}

QTEST_MAIN(ModelVerifierTests)
#include "ModelVerifierTests.moc"
