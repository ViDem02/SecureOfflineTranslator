#include "TranslationWorker.h"

#include "engine/LlamaEngine.h"
#include "security/ModelVerifier.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>

namespace {
QString unquote(QString value) {
    value = value.trimmed();
    if (value.size() >= 2 &&
        ((value.startsWith('"') && value.endsWith('"')) ||
         (value.startsWith('\'') && value.endsWith('\'')))) {
        return value.mid(1, value.size() - 2);
    }
    return value;
}

bool loadGlossary(const QString &path, QString *glossary, QString *error) {
    glossary->clear();
    if (path.isEmpty()) return true;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        *error = "Glossary cannot be opened: " + path;
        return false;
    }

    QString source;
    QStringList terms;
    const auto lines = QString::fromUtf8(file.readAll()).split('\n');
    for (const auto &line : lines) {
        const auto trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith('#') || trimmed == "terms:") continue;
        const auto separator = trimmed.indexOf(':');
        if (separator <= 0) {
            *error = "Glossary has invalid YAML";
            return false;
        }
        const auto key = trimmed.left(separator).remove('-').trimmed();
        const auto value = unquote(trimmed.mid(separator + 1));
        if (key == "source") {
            source = value;
        } else if (key == "target" && !source.isEmpty()) {
            terms.append(source + " -> " + value);
            source.clear();
        } else if (key != "source" && key != "target") {
            *error = "Glossary supports only source and target fields";
            return false;
        }
    }
    if (!source.isEmpty()) {
        *error = "Glossary entry is missing target";
        return false;
    }
    *glossary = terms.join('\n');
    return true;
}
}

TranslationWorker::TranslationWorker() : engine_(new LlamaEngine()) {}

TranslationWorker::~TranslationWorker() {
    delete engine_;
}

void TranslationWorker::initializeModel() {
    if (modelLoaded_) return;

    const auto modelPath = QDir(QCoreApplication::applicationDirPath())
                               .filePath("../Resources/" SECURE_TRANSLATOR_BUNDLED_MODEL_FILE);
    const auto manifestPath = QStringLiteral(":/models/approved-models.json");
    emit statusChanged("Verifying model");

    ModelVerifier verifier;
    QString actualHash;
    QString error;
    if (!verifier.verifyManifest(modelPath, manifestPath, &actualHash, &error)) {
        emit statusChanged("Error: " + error);
        return;
    }

    emit statusChanged("Loading model");
    if (!engine_->loadModel(modelPath, &error)) {
        emit statusChanged("Error: " + error);
        return;
    }

    const auto glossaryFile = QStringLiteral(SECURE_TRANSLATOR_BUNDLED_GLOSSARY_FILE);
    const auto glossaryPath = glossaryFile.isEmpty()
        ? QString()
        : QDir(QCoreApplication::applicationDirPath()).filePath("../Resources/" + glossaryFile);
    if (!loadGlossary(glossaryPath, &glossary_, &error)) {
        emit statusChanged("Error: " + error);
        return;
    }

    modelLoaded_ = true;
    emit statusChanged("Model ready");
}

void TranslationWorker::translate(const QString &source, const QString &sourceLanguage,
                                 const QString &targetLanguage) {
    cancelled_ = false;
    if (!modelLoaded_) {
        emit statusChanged("Error: model is not ready");
        return;
    }
    QString error;
    emit statusChanged("Translating");
    const auto completed = engine_->translate(
        source, sourceLanguage, targetLanguage, glossary_,
        [this](const QString &partial) {
            if (!cancelled_) emit partialTranslation(partial);
        },
        &error);
    if (completed) {
        emit statusChanged("Completed");
    } else if (cancelled_) {
        emit statusChanged("Cancelled");
    } else {
        emit statusChanged("Error: " + error);
    }
}

void TranslationWorker::cancel() {
    requestCancel();
}

void TranslationWorker::requestCancel() {
    cancelled_ = true;
    if (engine_) engine_->cancel();
}
