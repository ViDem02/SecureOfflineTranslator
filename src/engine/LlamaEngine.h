#pragma once

#include <atomic>

#include "InferenceEngine.h"

struct llama_context;
struct llama_model;

class LlamaEngine final : public InferenceEngine {
public:
    ~LlamaEngine() override;
    bool loadModel(const QString &path, QString *error) override;
    bool translate(const QString &source, const QString &sourceLanguage,
                   const QString &targetLanguage, const QString &glossary,
                   const PartialResult &onPartial,
                   QString *error) override;
    void cancel() override;

private:
    llama_model *model_ = nullptr;
    llama_context *context_ = nullptr;
    std::atomic_bool cancelled_ = false;
};
