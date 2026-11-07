#pragma once

#include <QMainWindow>

class QComboBox;
class QProgressBar;
class QPushButton;
class QTextEdit;
class QLabel;
class QToolButton;
class TranslationController;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void translate();
    void cancel();
    void openTextFile();
    void saveTranslation();
    void copyTranslation();
    void clearText();
    void swapLanguages();
    void updateSourceCount();
    void updateStatus(const QString &status);

    QTextEdit *sourceText_;
    QTextEdit *translatedText_;
    QComboBox *sourceLanguage_;
    QComboBox *targetLanguage_;
    QProgressBar *progress_;
    QLabel *status_;
    QPushButton *translateButton_;
    QPushButton *cancelButton_;
    QPushButton *copyButton_;
    QPushButton *saveButton_;
    QLabel *sourceCount_;
    QLabel *translationCount_;
    QLabel *modelStatus_;
    QLabel *securityStatus_;
    TranslationController *controller_;
};
