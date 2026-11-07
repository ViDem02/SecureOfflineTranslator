#pragma once

#include <atomic>

#include <QObject>

class TranslationWorker final : public QObject {
    Q_OBJECT

public:
    explicit TranslationWorker();
    ~TranslationWorker() override;

public slots:
    void initializeModel();
    void translate(const QString &source, const QString &sourceLanguage,
                   const QString &targetLanguage, int maxEndOfTurns, bool includeExplanation);
    void cancel();
    void requestCancel();

signals:
    void partialTranslation(const QString &text);
    void explanationReady(const QString &text);
    void statusChanged(const QString &status);

private:
    std::atomic_bool cancelled_ = false;
    class LlamaEngine *engine_;
    bool modelLoaded_ = false;
    QString glossary_;
};
