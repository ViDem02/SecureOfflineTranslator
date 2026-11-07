#pragma once

#include <functional>

#include <QString>

class InferenceEngine {
public:
    using PartialResult = std::function<void(const QString &)>;

    virtual ~InferenceEngine() = default;
    virtual bool loadModel(const QString &path, QString *error) = 0;
    virtual bool translate(const QString &source, const QString &sourceLanguage,
                           const QString &targetLanguage, const QString &glossary,
                           const PartialResult &onPartial,
                           QString *error) = 0;
    virtual void cancel() = 0;
};
