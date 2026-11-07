#pragma once

#include <QObject>

class QTextEdit;
class QThread;
class TranslationWorker;

class TranslationController final : public QObject {
    Q_OBJECT

public:
    explicit TranslationController(QObject *parent = nullptr);
    ~TranslationController() override;
    void translate(const QString &source, const QString &sourceLanguage,
                   const QString &targetLanguage, int maxEndOfTurns, bool includeExplanation);
    void cancel();
    void openTextFile(const QString &path, QTextEdit *destination);
    bool saveTextFile(const QString &path, const QString &text);

signals:
    void partialTranslation(const QString &text);
    void explanationReady(const QString &text);
    void statusChanged(const QString &status);

private:
    QThread *thread_;
    TranslationWorker *worker_;
};
