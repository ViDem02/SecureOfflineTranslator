#include "LlamaEngine.h"

#include <llama.h>

#include <algorithm>

#include <QByteArray>
#include <QVector>

namespace {
bool hasRepeatedSuffix(const QString &text) {
    const int maximum = std::min(240, static_cast<int>(text.size() / 3));
    for (int length = 24; length <= maximum; ++length) {
        const auto suffix = text.right(length);
        if (text.endsWith(suffix + suffix + suffix)) return true;
    }
    return false;
}

QString makePrompt(const llama_model *model, const QString &source,
                   const QString &sourceLanguage, const QString &targetLanguage,
                   const QString &glossary, bool includeExplanation,
                   bool continuation) {
    if (continuation) {
                       return QString("\n<end_of_turn>\n<start_of_turn>user\n"
                                      "Continue translating from %1 to %2. Translate only this next "
                                      "source part. Do not add information, commentary, or another "
                                      "paragraph after the translation. Preserve the established "
                                      "terminology and formatting. Technical data must not be "
                                      "altered: reproduce it verbatim. This includes numbers, "
                                      "decimal separators, units, tolerances, dates, times, "
                                      "identifiers, standards, model names, formulas, and symbols. "
                                      "If you are unsure whether technical data can be translated "
                                      "or reformatted, leave it in English and preserve it "
                                      "verbatim:\n%3\n"
                                      "<end_of_turn>\n<start_of_turn>model\n")
                           .arg(sourceLanguage, targetLanguage, source);
    }
    const auto outputInstruction = includeExplanation
        ? QStringLiteral(" After the translation, add a section exactly named "
                         "EXPLANATION: and briefly explain the main translation choices.")
        : QString();
    const auto glossaryInstruction = glossary.isEmpty()
        ? QString()
        : QString("\n\nUse these glossary terms when applicable:\n%1").arg(glossary);
    const auto *templateName = llama_model_chat_template(model, nullptr);
    if (!templateName) {
        return QString("You are a professional translator. Translate the text below "
                       "from %1 to %2. Do not repeat the source text, explain your work, "
                       "or add commentary. Return only the translation. Preserve numbers, "
                       "units, symbols, identifiers, formatting, and line breaks. Technical "
                       "data must not be altered: reproduce it verbatim. This includes numbers, "
                       "decimal separators, units, tolerances, dates, times, identifiers, "
                       "standards, model names, formulas, and symbols. If you are unsure "
                       "whether technical data can be translated or reformatted, leave it in "
                       "English and preserve it verbatim.%3%4\n\n"
                       "SOURCE TEXT:\n%5\n\nTRANSLATION:")
            .arg(sourceLanguage, targetLanguage, glossaryInstruction, outputInstruction, source);
    }
    const auto systemText =
        QStringLiteral("You are a professional translator. Translate the user's text "
                       "faithfully. Never repeat the source text, explain your work, or add "
                       "commentary. Return only the translation. Preserve numbers, units, "
                       "symbols, identifiers, formatting, and line breaks. Technical data "
                       "must not be altered: reproduce it verbatim. This includes numbers, "
                       "decimal separators, units, tolerances, dates, times, identifiers, "
                       "standards, model names, formulas, and symbols. If you are unsure "
                       "whether technical data can be translated or reformatted, leave it in "
                       "English and preserve it verbatim.")
                       + outputInstruction + glossaryInstruction;
    const QByteArray system = systemText.toUtf8();
    const QByteArray user = QString("Translate the following text from %1 to %2. "
                                    "Return only the %2 translation:\n\n%3")
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
    const auto modelContext = llama_model_n_ctx_train(model_);
    contextParams.n_ctx = std::min<uint32_t>(8192, modelContext > 0 ? modelContext : 8192);
    contextParams.n_batch = contextParams.n_ctx;
    contextParams.n_ubatch = std::min<uint32_t>(512, contextParams.n_ctx);
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

bool LlamaEngine::beginTranslation(QString *error) {
    if (!model_) {
        if (error) *error = "No verified model is loaded";
        return false;
    }
    if (context_) {
        llama_free(context_);
        context_ = nullptr;
    }
    auto params = llama_context_default_params();
    const auto modelContext = llama_model_n_ctx_train(model_);
    params.n_ctx = std::min<uint32_t>(8192, modelContext > 0 ? modelContext : 8192);
    params.n_batch = params.n_ctx;
    params.n_ubatch = std::min<uint32_t>(512, params.n_ctx);
    params.n_threads = 4;
    params.n_threads_batch = 4;
    context_ = llama_init_from_model(model_, params);
    if (!context_) {
        if (error) *error = "The model context could not be created";
        return false;
    }
    contextPosition_ = 0;
    translationActive_ = true;
    return true;
}

void LlamaEngine::endTranslation() {
    if (context_) {
        llama_memory_clear(llama_get_memory(context_), true);
    }
    contextPosition_ = 0;
    translationActive_ = false;
}

bool LlamaEngine::translate(const QString &source, const QString &sourceLanguage,
                            const QString &targetLanguage, const QString &glossary,
                            int maxEndOfTurns,
                            bool includeExplanation,
                            const PartialResult &onPartial,
                            QString *error) {
    if (!model_ || !context_) {
        if (error) *error = "No verified model is loaded";
        return false;
    }
    if (maxEndOfTurns < 1) {
        if (error) *error = "Maximum end_of_turn count must be at least 1";
        return false;
    }
    cancelled_ = false;
    if (!translationActive_ && !beginTranslation(error)) return false;
    const auto prompt = makePrompt(model_, source, sourceLanguage, targetLanguage, glossary,
                                   includeExplanation, contextPosition_ > 0);
    if (prompt.isEmpty()) {
        if (error) *error = "The model chat template could not be applied";
        return false;
    }
    const auto *vocab = llama_model_get_vocab(model_);
    QByteArray promptBytes = prompt.toUtf8();
    QVector<llama_token> tokens(1024);
    auto tokenCount = llama_tokenize(vocab, promptBytes.constData(), promptBytes.size(),
                                     tokens.data(), tokens.size(), contextPosition_ == 0, false);
    if (tokenCount < 0) {
        tokens.resize(-tokenCount);
        tokenCount = llama_tokenize(vocab, promptBytes.constData(), promptBytes.size(),
                                    tokens.data(), tokens.size(), contextPosition_ == 0, false);
    }
    if (tokenCount <= 0) {
        if (error) *error = "The translation prompt could not be tokenized";
        return false;
    }
    tokens.resize(tokenCount);
    const auto contextSize = static_cast<int>(llama_n_ctx(context_));
    if (tokenCount >= contextSize) {
        if (error) {
            *error = QString("Translation part is too large for the model context "
                             "(prompt tokens=%1, context=%2)")
                         .arg(tokenCount).arg(contextSize);
        }
        return false;
    }
    const auto availableTokens = contextSize - contextPosition_ - tokenCount - 1;
    if (availableTokens <= 0) {
        if (error) *error = "Input is too large for the model context";
        return false;
    }
    auto batch = llama_batch_init(tokenCount, 0, 1);
    batch.n_tokens = tokenCount;
    const auto promptPosition = contextPosition_;
    for (int32_t i = 0; i < tokenCount; ++i) {
        batch.token[i] = tokens[i];
        batch.pos[i] = promptPosition + i;
        batch.n_seq_id[i] = 1;
        batch.seq_id[i][0] = 0;
        batch.logits[i] = i == tokenCount - 1;
    }
    const auto promptDecode = llama_decode(context_, batch);
    llama_batch_free(batch);
    if (promptDecode != 0) {
        if (error) {
            *error = QString("The model could not process the prompt (llama_decode=%1, "
                             "tokens=%2, context=%3)")
                         .arg(promptDecode).arg(tokenCount).arg(llama_n_ctx(context_));
        }
        return false;
    }
    auto samplerParams = llama_sampler_chain_default_params();
    auto *sampler = llama_sampler_chain_init(samplerParams);
    if (!sampler) {
        if (error) *error = "The sampler could not be created";
        return false;
    }
    llama_sampler_chain_add(sampler, llama_sampler_init_temp(0.2f));
    llama_sampler_chain_add(sampler, llama_sampler_init_penalties(
        llama_vocab_n_tokens(vocab), 256, 1.15f, 0.0f, 0.0f));
    llama_sampler_chain_add(sampler, llama_sampler_init_greedy());
    auto nextBatch = llama_batch_init(1, 0, 1);
    QByteArray outputBytes;
    int endOfTurnCount = 0;
    const auto maxGeneratedTokens = availableTokens;
    int decodedTokens = 0;
    bool stoppedByEndOfTurn = false;
    bool stoppedByRepetition = false;
    for (int i = 0; i < maxGeneratedTokens && !cancelled_; ++i) {
        const auto token = llama_sampler_sample(sampler, context_, -1);
        if (llama_vocab_is_eog(vocab, token)) {
            ++endOfTurnCount;
            if (endOfTurnCount >= maxEndOfTurns) {
                stoppedByEndOfTurn = true;
                break;
            }
        }
        llama_sampler_accept(sampler, token);
        char piece[256];
        const auto length = llama_token_to_piece(vocab, token, piece, sizeof(piece), 0, false);
        if (length > 0) {
            outputBytes.append(piece, length);
            const auto output = QString::fromUtf8(outputBytes);
            const auto endOfTurn = output.indexOf("<end_of_turn>");
            if (endOfTurn >= 0) {
                outputBytes = output.left(endOfTurn).toUtf8();
                onPartial(QString::fromUtf8(outputBytes));
                ++endOfTurnCount;
                if (endOfTurnCount >= maxEndOfTurns) {
                    stoppedByEndOfTurn = true;
                    break;
                }
            } else {
                const auto startOfTurn = output.indexOf("<start_of_turn>");
                if (startOfTurn >= 0) {
                    outputBytes = output.left(startOfTurn).toUtf8();
                    onPartial(QString::fromUtf8(outputBytes));
                    ++endOfTurnCount;
                    if (endOfTurnCount >= maxEndOfTurns) {
                        stoppedByEndOfTurn = true;
                        break;
                    }
                } else {
                    onPartial(output);
                    if (hasRepeatedSuffix(output)) {
                        stoppedByRepetition = true;
                        break;
                    }
                }
            }
        }
        if (i + 1 >= maxGeneratedTokens) {
            break;
        }
        nextBatch.n_tokens = 1;
        nextBatch.token[0] = token;
        nextBatch.pos[0] = promptPosition + tokenCount + i;
        nextBatch.n_seq_id[0] = 1;
        nextBatch.seq_id[0][0] = 0;
        nextBatch.logits[0] = 1;
        const auto tokenDecode = llama_decode(context_, nextBatch);
        if (tokenDecode != 0) {
            if (error) {
                *error = QString("The model ran out of context or memory "
                                 "(llama_decode=%1, generated=%2, context=%3)")
                             .arg(tokenDecode).arg(i + 1).arg(llama_n_ctx(context_));
            }
            llama_batch_free(nextBatch);
            llama_sampler_free(sampler);
            return false;
        }
        ++decodedTokens;
    }
    llama_batch_free(nextBatch);
    llama_sampler_free(sampler);
    if (cancelled_) {
        if (error) *error = "Translation cancelled";
        return false;
    }
    if (stoppedByRepetition) {
        if (error) {
            *error = "Translation stopped because the model began repeating output";
        }
        return false;
    }
    contextPosition_ = promptPosition + tokenCount + decodedTokens;
    if (!stoppedByEndOfTurn) {
        if (error) {
            *error = QString("Translation output reached the context limit "
                             "(generated=%1, context=%2); the part must be split "
                             "into smaller sections")
                         .arg(maxGeneratedTokens).arg(llama_n_ctx(context_));
        }
        return false;
    }
    return true;
}

void LlamaEngine::cancel() {
    cancelled_ = true;
}
