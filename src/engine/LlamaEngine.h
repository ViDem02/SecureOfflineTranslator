#pragma once

#include <atomic>

#include "InferenceEngine.h"

struct llama_context;
struct llama_model;

class LlamaEngine final : public InferenceEngine {
public:
    ~LlamaEngine() override;
    bool loadModel(const QString &path, QString *error) override;
    bool beginTranslation(QString *error) override;
    void endTranslation() override;
    bool translate(const QString &source, const QString &sourceLanguage,
                   const QString &targetLanguage, const QString &glossary,
                   int maxEndOfTurns,
                   bool includeExplanation,
                   const PartialResult &onPartial,
                   QString *error) override;
    void cancel() override;

private:
    llama_model *model_ = nullptr;
    llama_context *context_ = nullptr;
    std::atomic_bool cancelled_ = false;
    int contextPosition_ = 0;
    bool translationActive_ = false;
};
