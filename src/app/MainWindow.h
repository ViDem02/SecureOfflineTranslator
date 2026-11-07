#pragma once

#include <QMainWindow>

class QComboBox;
class QCheckBox;
class QProgressBar;
class QSpinBox;
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
    void updateResourceStatus();

    QTextEdit *sourceText_;
    QTextEdit *translatedText_;
    QTextEdit *explanationText_;
    QComboBox *sourceLanguage_;
    QComboBox *targetLanguage_;
    QSpinBox *maxEndOfTurns_;
    QCheckBox *includeExplanation_;
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
    QLabel *resourceStatus_;
    TranslationController *controller_;
    bool securityReady_ = false;
};
