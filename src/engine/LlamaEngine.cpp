#include "LlamaEngine.h"

#include <llama.h>

#include <QByteArray>
#include <QVector>

namespace {
QString makePrompt(const llama_model *model, const QString &source,
                   const QString &sourceLanguage, const QString &targetLanguage,
                   const QString &glossary) {
    const auto glossaryInstruction = glossary.isEmpty()
        ? QString()
        : QString("\n\nUse these glossary terms when applicable:\n%1").arg(glossary);
    const auto *templateName = llama_model_chat_template(model, nullptr);
    if (!templateName) {
        return QString("Translate from %1 to %2. Preserve numbers, units, symbols, "
                       "identifiers, and line breaks. Return only the translation.%3\n\n%4")
            .arg(sourceLanguage, targetLanguage, glossaryInstruction, source);
    }
    const auto systemText =
        QStringLiteral("Translate faithfully. Preserve numbers, units, symbols, identifiers, "
                       "formatting, and line breaks. Return only the translation.")
        + glossaryInstruction;
    const QByteArray system = systemText.toUtf8();
    const QByteArray user = QString("Translate from %1 to %2:\n\n%3")
                                .arg(sourceLanguage, targetLanguage, source)
                                .toUtf8();
    llama_chat_message messages[] = {
        {"system", system.constData()},
        {"user", user.constData()},
    };
    const auto size = llama_chat_apply_template(templateName, messages, 2, true, nullptr, 0);
    if (size <= 0) return {};
    QByteArray formatted(size, '\0');
    llama_chat_apply_template(templateName, messages, 2, true, formatted.data(), size);
    return QString::fromUtf8(formatted);
}

}

LlamaEngine::~LlamaEngine() {
    if (context_) llama_free(context_);
    if (model_) llama_model_free(model_);
}

bool LlamaEngine::loadModel(const QString &path, QString *error) {
    if (model_) {
        if (error) *error = "A model is already loaded";
        return false;
    }
    llama_backend_init();
    auto params = llama_model_default_params();
    params.n_gpu_layers = -1;
    model_ = llama_model_load_from_file(path.toUtf8().constData(), params);
    if (!model_) {
        if (error) *error = "The GGUF model could not be loaded";
        return false;
    }
    auto contextParams = llama_context_default_params();
    contextParams.n_ctx = 4096;
    contextParams.n_batch = 512;
    contextParams.n_threads = 4;
    contextParams.n_threads_batch = 4;
    context_ = llama_init_from_model(model_, contextParams);
    if (!context_) {
        llama_model_free(model_);
        model_ = nullptr;
        if (error) *error = "Insufficient memory for the model context";
        return false;
    }
    return true;
}

bool LlamaEngine::translate(const QString &source, const QString &sourceLanguage,
                            const QString &targetLanguage, const QString &glossary,
                            const PartialResult &onPartial,
                            QString *error) {
    if (!model_ || !context_) {
        if (error) *error = "No verified model is loaded";
        return false;
    }
    cancelled_ = false;
    llama_memory_clear(llama_get_memory(context_), true);
    const auto prompt = makePrompt(model_, source, sourceLanguage, targetLanguage, glossary);
    if (prompt.isEmpty()) {
        if (error) *error = "The model chat template could not be applied";
        return false;
    }
    const auto *vocab = llama_model_get_vocab(model_);
    QByteArray promptBytes = prompt.toUtf8();
    QVector<llama_token> tokens(1024);
    auto tokenCount = llama_tokenize(vocab, promptBytes.constData(), promptBytes.size(),
                                     tokens.data(), tokens.size(), true, false);
    if (tokenCount < 0) {
        tokens.resize(-tokenCount);
        tokenCount = llama_tokenize(vocab, promptBytes.constData(), promptBytes.size(),
                                    tokens.data(), tokens.size(), true, false);
    }
    if (tokenCount <= 0) {
        if (error) *error = "The translation prompt could not be tokenized";
        return false;
    }
    tokens.resize(tokenCount);
    auto batch = llama_batch_get_one(tokens.data(), tokenCount);
    if (llama_decode(context_, batch) != 0) {
        if (error) *error = "The model could not process the prompt";
        return false;
    }
    auto samplerParams = llama_sampler_chain_default_params();
    auto *sampler = llama_sampler_chain_init(samplerParams);
    llama_sampler_chain_add(sampler, llama_sampler_init_temp(0.2f));
    llama_sampler_chain_add(sampler, llama_sampler_init_greedy());
    QString output;
    for (int i = 0; i < 512 && !cancelled_; ++i) {
        const auto token = llama_sampler_sample(sampler, context_, -1);
        if (llama_vocab_is_eog(vocab, token) || llama_vocab_is_control(vocab, token)) {
            break;
        }
        llama_sampler_accept(sampler, token);
        char piece[256];
        const auto length = llama_token_to_piece(vocab, token, piece, sizeof(piece), 0, false);
        if (length > 0) {
            const auto chunk = QString::fromUtf8(piece, length);
            output += chunk;
            onPartial(output);
        }
        auto next = llama_batch_get_one(const_cast<llama_token *>(&token), 1);
        if (llama_decode(context_, next) != 0) {
            if (error) *error = "The model ran out of context or memory";
            llama_sampler_free(sampler);
            return false;
        }
    }
    llama_sampler_free(sampler);
    if (cancelled_) {
        if (error) *error = "Translation cancelled";
        return false;
    }
    return true;
}

void LlamaEngine::cancel() {
    cancelled_ = true;
}
