#include "TranslationWorker.h"

#include "engine/LlamaEngine.h"
#include "security/ModelVerifier.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QVector>

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

bool parseGlossary(const QString &yaml, QString *glossary, QString *error) {
    QString source;
    QStringList terms;
    const auto lines = yaml.split('\n');
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

bool loadGlossary(const QString &path, QString *glossary, QString *error) {
    glossary->clear();
    if (path.isEmpty()) return true;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        *error = "Glossary cannot be opened: " + path;
        return false;
    }
    return parseGlossary(QString::fromUtf8(file.readAll()), glossary, error);
}

QVector<QString> splitTranslationInput(const QString &source) {
    // Keep room for the chat template, glossary, and generated translation.
    constexpr int maxChunkCharacters = 1200;
    QVector<QString> chunks;
    int start = 0;

    while (start < source.size()) {
        const int remaining = source.size() - start;
        if (remaining <= maxChunkCharacters) {
            chunks.append(source.mid(start));
            break;
        }

        const int limit = start + maxChunkCharacters;
        int cut = source.lastIndexOf(QStringLiteral("\n\n"), limit - 1);
        if (cut >= start + maxChunkCharacters / 2) {
            cut += 2;
        } else {
            cut = -1;
            for (int i = limit - 1; i > start + maxChunkCharacters / 2; --i) {
                const QChar character = source.at(i);
                const bool sentenceEnd = (character == '.' || character == '!' ||
                                          character == '?' || character == ':' ||
                                          character == ';');
                if (sentenceEnd && i + 1 < source.size() &&
                    source.at(i + 1).isSpace()) {
                    cut = i + 1;
                    break;
                }
            }
            if (cut < 0) {
                cut = source.lastIndexOf(QChar(' '), limit - 1);
            }
            if (cut <= start) {
                cut = limit;
            }
        }

        chunks.append(source.mid(start, cut - start));
        start = cut;
    }

    return chunks;
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

void TranslationWorker::setGlossaryYaml(const QString &yaml) {
    QString parsed;
    QString error;
    if (!parseGlossary(yaml, &parsed, &error)) {
        emit statusChanged("Error: " + error);
        return;
    }
    glossaryYaml_ = yaml;
    glossary_ = parsed;
    emit statusChanged("Glossary updated");
}

void TranslationWorker::translate(const QString &source, const QString &sourceLanguage,
                                 const QString &targetLanguage, int maxEndOfTurns,
                                 bool includeExplanation) {
    cancelled_ = false;
    if (!modelLoaded_) {
        emit statusChanged("Error: model is not ready");
        return;
    }
    QString error;
    const auto chunks = splitTranslationInput(source);
    QString completeTranslation;
    QString completeExplanation;
    if (!engine_->beginTranslation(&error)) {
        emit statusChanged("Error: " + error);
        return;
    }

    for (qsizetype chunkIndex = 0; chunkIndex < chunks.size() && !cancelled_; ++chunkIndex) {
        emit statusChanged(QString("Translating part %1/%2")
                               .arg(chunkIndex + 1)
                               .arg(chunks.size()));
        QString chunkOutput;
        const auto completed = engine_->translate(
            chunks.at(chunkIndex), sourceLanguage, targetLanguage, glossary_, maxEndOfTurns,
            includeExplanation,
            [this, &chunkOutput, &completeTranslation, includeExplanation](const QString &partial) {
                chunkOutput = partial;
                if (!cancelled_ && !includeExplanation) {
                    emit partialTranslation(completeTranslation + chunkOutput);
                }
            },
            &error);
        if (!completed) {
            engine_->endTranslation();
            if (cancelled_) {
                emit statusChanged("Cancelled");
            } else {
                emit statusChanged("Error: " + error);
            }
            return;
        }

        if (includeExplanation) {
            const auto marker = chunkOutput.indexOf("EXPLANATION:");
            if (marker >= 0) {
                const auto translation = chunkOutput.left(marker).trimmed();
                const auto explanation = chunkOutput.mid(marker + 13).trimmed();
                completeTranslation += translation;
                completeExplanation += explanation;
            } else {
                completeTranslation += chunkOutput;
                completeExplanation += "The model did not provide an explanation.";
            }
        } else {
            completeTranslation += chunkOutput;
        }

        if (chunkIndex + 1 < chunks.size()) {
            completeTranslation += '\n';
            if (includeExplanation) completeExplanation += "\n --- \n";
        }
    }

    if (cancelled_) {
        engine_->endTranslation();
        emit statusChanged("Cancelled");
        return;
    }
    engine_->endTranslation();
    emit partialTranslation(completeTranslation);
    if (includeExplanation) {
        emit explanationReady(completeExplanation);
    }
    emit statusChanged("Completed");
}

void TranslationWorker::cancel() {
    requestCancel();
}

void TranslationWorker::requestCancel() {
    cancelled_ = true;
    if (engine_) engine_->cancel();
}
