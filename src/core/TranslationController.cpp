#include "TranslationController.h"

#include "TranslationWorker.h"

#include "storage/FileAccess.h"

#include <QMetaObject>
#include <QThread>
#include <QTextEdit>

TranslationController::TranslationController(QObject *parent)
    : QObject(parent), thread_(new QThread(this)), worker_(new TranslationWorker()) {
    worker_->moveToThread(thread_);
    connect(worker_, &TranslationWorker::partialTranslation,
            this, &TranslationController::partialTranslation);
    connect(worker_, &TranslationWorker::explanationReady,
            this, &TranslationController::explanationReady);
    connect(worker_, &TranslationWorker::statusChanged,
            this, &TranslationController::statusChanged);
    connect(thread_, &QThread::finished, worker_, &QObject::deleteLater);
    thread_->start();
    QMetaObject::invokeMethod(worker_, "initializeModel", Qt::QueuedConnection);
}

TranslationController::~TranslationController() {
    thread_->quit();
    thread_->wait();
}

void TranslationController::translate(const QString &source, const QString &sourceLanguage,
                                      const QString &targetLanguage, int maxEndOfTurns,
                                      bool includeExplanation) {
    QMetaObject::invokeMethod(worker_, "translate", Qt::QueuedConnection,
                              Q_ARG(QString, source),
                              Q_ARG(QString, sourceLanguage),
                              Q_ARG(QString, targetLanguage),
                              Q_ARG(int, maxEndOfTurns),
                              Q_ARG(bool, includeExplanation));
}

void TranslationController::cancel() {
    worker_->requestCancel();
}

void TranslationController::openTextFile(const QString &path, QTextEdit *destination) {
    QString text;
    QString error;
    if (FileAccess::readTextFile(path, &text, &error)) {
        destination->setPlainText(text);
    } else {
        emit statusChanged(error);
    }
}

bool TranslationController::saveTextFile(const QString &path, const QString &text) {
    QString error;
    const bool saved = FileAccess::writeTextFile(path, text, &error);
    if (!saved) {
        emit statusChanged(error);
    }
    return saved;
}
