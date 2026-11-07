#include "MainWindow.h"

#include "core/TranslationController.h"
#include "security/SecurityStatus.h"
#include "system/ResourceMonitor.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QClipboard>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFile>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSizePolicy>
#include <QSpinBox>
#include <QSplitter>
#include <QTextEdit>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

namespace {
QLabel *heading(const QString &text, QWidget *parent) {
    auto *label = new QLabel(text, parent);
    label->setObjectName("panelHeading");
    return label;
}

QFrame *card(QWidget *parent) {
    auto *frame = new QFrame(parent);
    frame->setObjectName("statusCard");
    return frame;
}
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    QString securityError;
    securityReady_ = SecurityStatus::isReady(&securityError);
    setWindowTitle("Secure Offline Translator");
    setMinimumSize(820, 560);
    resize(1100, 720);

    auto *central = new QWidget(this);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(16);

    auto *header = new QHBoxLayout();
    auto *title = new QLabel("Secure Offline Translator", this);
    title->setObjectName("title");
    auto *subtitle = new QLabel("Local technical translation", this);
    subtitle->setObjectName("subtitle");
    auto *titleColumn = new QVBoxLayout();
    titleColumn->setSpacing(2);
    titleColumn->addWidget(title);
    titleColumn->addWidget(subtitle);
    header->addLayout(titleColumn);
    header->addStretch();
    modelStatus_ = new QLabel("Model: bundled", this);
    modelStatus_->setObjectName("statusPill");
    header->addWidget(modelStatus_);
    securityStatus_ = new QLabel(this);
    securityStatus_->setText(securityReady_ ? "Security: sandbox active"
                                            : "Security: unavailable");
    if (!securityReady_) securityStatus_->setToolTip(securityError);
    securityStatus_->setObjectName(securityReady_ ? "statusPill" : "warningPill");
    header->addWidget(securityStatus_);
    resourceStatus_ = new QLabel("Memory: checking", this);
    resourceStatus_->setObjectName("resourcePill");
    header->addWidget(resourceStatus_);
    root->addLayout(header);

    auto *toolbar = addToolBar("File");
    toolbar->setMovable(false);
    auto *openAction = toolbar->addAction("Open .txt");
    auto *saveAction = toolbar->addAction("Save translation");
    toolbar->addSeparator();
    auto *clearAction = toolbar->addAction("Clear");
    toolbar->addSeparator();
    auto *swapAction = toolbar->addAction("Swap languages");
    toolbar->addSeparator();
    auto *importGlossaryAction = toolbar->addAction("Import glossary");
    auto *exportGlossaryAction = toolbar->addAction("Export glossary");
    auto *editGlossaryAction = toolbar->addAction("Edit glossary");
    openAction->setShortcut(QKeySequence::Open);
    saveAction->setShortcut(QKeySequence::Save);
    clearAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
    swapAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    connect(openAction, &QAction::triggered, this, &MainWindow::openTextFile);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveTranslation);
    connect(clearAction, &QAction::triggered, this, &MainWindow::clearText);
    connect(swapAction, &QAction::triggered, this, &MainWindow::swapLanguages);
    connect(importGlossaryAction, &QAction::triggered, this, &MainWindow::importGlossary);
    connect(exportGlossaryAction, &QAction::triggered, this, &MainWindow::exportGlossary);
    connect(editGlossaryAction, &QAction::triggered, this, &MainWindow::editGlossary);

    auto *languageRow = new QHBoxLayout();
    languageRow->addWidget(new QLabel("Translate", this));
    sourceLanguage_ = new QComboBox(this);
    targetLanguage_ = new QComboBox(this);
    sourceLanguage_->addItems({"Italian", "English", "French"});
    targetLanguage_->addItems({"English", "Italian", "French"});
    targetLanguage_->setCurrentText("English");
    maxEndOfTurns_ = new QSpinBox(this);
    maxEndOfTurns_->setRange(1, 8);
    maxEndOfTurns_->setValue(1);
    maxEndOfTurns_->setToolTip("Maximum number of end_of_turn markers before stopping");
    includeExplanation_ = new QCheckBox("Explain", this);
    includeExplanation_->setChecked(true);
    includeExplanation_->setToolTip("Ask Gemma to explain the translation");
    languageRow->addWidget(sourceLanguage_);
    languageRow->addWidget(new QLabel("to", this));
    languageRow->addWidget(targetLanguage_);
    languageRow->addWidget(new QLabel("max end_of_turn", this));
    languageRow->addWidget(maxEndOfTurns_);
    languageRow->addWidget(includeExplanation_);
    languageRow->addStretch();
    root->addLayout(languageRow);

    auto *panels = new QSplitter(Qt::Horizontal, this);
    panels->setChildrenCollapsible(false);
    auto *sourcePanel = new QWidget(panels);
    auto *sourceLayout = new QVBoxLayout(sourcePanel);
    sourceLayout->setContentsMargins(0, 0, 8, 0);
    auto *sourceHeader = new QHBoxLayout();
    sourceHeader->addWidget(heading("Original", sourcePanel));
    sourceCount_ = new QLabel("0 characters", sourcePanel);
    sourceCount_->setObjectName("counter");
    sourceHeader->addStretch();
    sourceHeader->addWidget(sourceCount_);
    sourceLayout->addLayout(sourceHeader);
    sourceText_ = new QTextEdit(sourcePanel);
    sourceText_->setAcceptRichText(false);
    sourceText_->setPlaceholderText("Paste or type the text to translate…");
    sourceLayout->addWidget(sourceText_);

    auto *translationPanel = new QWidget(panels);
    auto *translationLayout = new QVBoxLayout(translationPanel);
    translationLayout->setContentsMargins(8, 0, 0, 0);
    auto *translationHeader = new QHBoxLayout();
    translationHeader->addWidget(heading("Translation", translationPanel));
    translationCount_ = new QLabel("0 characters", translationPanel);
    translationCount_->setObjectName("counter");
    translationHeader->addStretch();
    translationHeader->addWidget(translationCount_);
    translationLayout->addLayout(translationHeader);
    translatedText_ = new QTextEdit(translationPanel);
    translatedText_->setReadOnly(true);
    translatedText_->setPlaceholderText("The local translation will appear here…");
    translationLayout->addWidget(translatedText_);
    translationLayout->addWidget(new QLabel("Explanation", translationPanel));
    explanationText_ = new QTextEdit(translationPanel);
    explanationText_->setReadOnly(true);
    explanationText_->setPlaceholderText("Optional explanation from Gemma…");
    explanationText_->setMaximumHeight(120);
    translationLayout->addWidget(explanationText_);

    panels->addWidget(sourcePanel);
    panels->addWidget(translationPanel);
    panels->setStretchFactor(0, 1);
    panels->setStretchFactor(1, 1);
    root->addWidget(panels, 1);

    auto *bottom = new QHBoxLayout();
    progress_ = new QProgressBar(this);
    progress_->setRange(0, 0);
    progress_->setTextVisible(false);
    progress_->setFixedWidth(180);
    progress_->hide();
    status_ = new QLabel("Ready", this);
    status_->setObjectName("statusText");
    bottom->addWidget(progress_);
    bottom->addWidget(status_, 1);
    copyButton_ = new QPushButton("Copy", this);
    saveButton_ = new QPushButton("Save", this);
    translateButton_ = new QPushButton("Translate", this);
    cancelButton_ = new QPushButton("Cancel", this);
    translateButton_->setObjectName("primaryButton");
    translateButton_->setEnabled(securityReady_);
    cancelButton_->setEnabled(false);
    copyButton_->setEnabled(false);
    saveButton_->setEnabled(false);
    bottom->addWidget(copyButton_);
    bottom->addWidget(saveButton_);
    bottom->addWidget(translateButton_);
    bottom->addWidget(cancelButton_);
    root->addLayout(bottom);
    setCentralWidget(central);

    setStyleSheet(
        "QMainWindow { background: #f5f7fa; }"
        "#title { font-size: 24px; font-weight: 700; color: #172033; }"
        "#subtitle { color: #667085; }"
        "#panelHeading { font-size: 15px; font-weight: 600; color: #344054; }"
        "#counter, #statusText { color: #667085; }"
        "QTextEdit { background: white; border: 1px solid #d0d5dd; border-radius: 8px; padding: 10px; font-size: 15px; }"
        "QTextEdit:focus { border: 2px solid #6172f3; }"
        "QComboBox, QPushButton { padding: 7px 12px; border: 1px solid #d0d5dd; border-radius: 6px; background: white; }"
        "#primaryButton { color: white; background: #4654d9; border: none; font-weight: 600; }"
        "#primaryButton:disabled { background: #98a2b3; }"
        "#statusPill, #warningPill { padding: 6px 10px; border-radius: 12px; background: #ecfdf3; color: #027a48; }"
        "#warningPill { background: #fffaeb; color: #b54708; }"
        "QToolBar { border: none; spacing: 8px; }"
        "QProgressBar { border: none; background: #e4e7ec; border-radius: 4px; }"
        "QProgressBar::chunk { background: #6172f3; border-radius: 4px; }");

    controller_ = securityReady_ ? new TranslationController(this) : nullptr;
    auto *resourceTimer = new QTimer(this);
    resourceTimer->setInterval(2000);
    connect(resourceTimer, &QTimer::timeout, this, &MainWindow::updateResourceStatus);
    resourceTimer->start();
    updateResourceStatus();
    connect(translateButton_, &QPushButton::clicked, this, &MainWindow::translate);
    connect(cancelButton_, &QPushButton::clicked, this, &MainWindow::cancel);
    connect(copyButton_, &QPushButton::clicked, this, &MainWindow::copyTranslation);
    connect(saveButton_, &QPushButton::clicked, this, &MainWindow::saveTranslation);
    connect(sourceText_, &QTextEdit::textChanged, this, &MainWindow::updateSourceCount);
    connect(translatedText_, &QTextEdit::textChanged, this, [this] {
        translationCount_->setText(QString("%1 characters")
            .arg(translatedText_->toPlainText().toUcs4().size()));
        copyButton_->setEnabled(!translatedText_->toPlainText().isEmpty());
        saveButton_->setEnabled(!translatedText_->toPlainText().isEmpty());
    });
    if (controller_) {
        connect(controller_, &TranslationController::statusChanged,
                this, &MainWindow::updateStatus);
        connect(controller_, &TranslationController::partialTranslation,
                translatedText_, &QTextEdit::setPlainText);
        connect(controller_, &TranslationController::explanationReady,
                explanationText_, &QTextEdit::setPlainText);
    } else {
        status_->setText("Translation disabled: sandbox verification failed");
    }

    auto *translateAction = new QAction(this);
    translateAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Return));
    addAction(translateAction);
    connect(translateAction, &QAction::triggered, this, &MainWindow::translate);
}

MainWindow::~MainWindow() = default;

void MainWindow::translate() {
    if (!securityReady_ || !controller_) {
        status_->setText("Translation disabled: " + QStringLiteral("sandbox verification failed"));
        return;
    }
    if (sourceText_->toPlainText().trimmed().isEmpty()) {
        status_->setText("Enter text before translating");
        return;
    }
    translatedText_->clear();
    progress_->show();
    translateButton_->setEnabled(false);
    cancelButton_->setEnabled(true);
    status_->setText("Preparing translation…");
    controller_->translate(sourceText_->toPlainText(), sourceLanguage_->currentText(),
                           targetLanguage_->currentText(), maxEndOfTurns_->value(),
                           includeExplanation_->isChecked());
}

void MainWindow::cancel() {
    controller_->cancel();
}

void MainWindow::importGlossary() {
    const auto path = QFileDialog::getOpenFileName(this, "Import glossary", {}, "YAML (*.yaml)");
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Import failed", "The glossary could not be opened.");
        return;
    }
    glossaryYaml_ = QString::fromUtf8(file.readAll());
    if (controller_) controller_->setGlossaryYaml(glossaryYaml_);
}

void MainWindow::exportGlossary() {
    if (glossaryYaml_.trimmed().isEmpty()) {
        QMessageBox::information(this, "Export glossary", "No edited glossary is loaded.");
        return;
    }
    const auto path = QFileDialog::getSaveFileName(this, "Export glossary", {}, "YAML (*.yaml)");
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text) ||
        file.write(glossaryYaml_.toUtf8()) < 0) {
        QMessageBox::critical(this, "Export failed", "The glossary could not be saved.");
    }
}

void MainWindow::editGlossary() {
    QDialog dialog(this);
    dialog.setWindowTitle("Edit glossary");
    dialog.resize(600, 400);
    auto *layout = new QVBoxLayout(&dialog);
    auto *editor = new QPlainTextEdit(&dialog);
    editor->setPlainText(glossaryYaml_.isEmpty()
        ? QStringLiteral("terms:\n  - source: \"term\"\n    target: \"terme\"\n")
        : glossaryYaml_);
    layout->addWidget(editor);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() == QDialog::Accepted) {
        glossaryYaml_ = editor->toPlainText();
        if (controller_) controller_->setGlossaryYaml(glossaryYaml_);
    }
}

void MainWindow::openTextFile() {
    const auto path = QFileDialog::getOpenFileName(this, "Open text", {}, "Text (*.txt)");
    if (!path.isEmpty()) controller_->openTextFile(path, sourceText_);
}

void MainWindow::saveTranslation() {
    const auto path = QFileDialog::getSaveFileName(this, "Save translation", {}, "Text (*.txt)");
    if (!path.isEmpty() && !controller_->saveTextFile(path, translatedText_->toPlainText())) {
        QMessageBox::critical(this, "Save failed", "The translation could not be saved.");
    }
}

void MainWindow::copyTranslation() {
    QApplication::clipboard()->setText(translatedText_->toPlainText());
    status_->setText("Translation copied");
}

void MainWindow::clearText() {
    sourceText_->clear();
    translatedText_->clear();
    status_->setText("Ready");
}

void MainWindow::swapLanguages() {
    const auto source = sourceLanguage_->currentText();
    sourceLanguage_->setCurrentText(targetLanguage_->currentText());
    targetLanguage_->setCurrentText(source);
}

void MainWindow::updateSourceCount() {
    sourceCount_->setText(QString("%1 characters")
        .arg(sourceText_->toPlainText().toUcs4().size()));
}

void MainWindow::updateResourceStatus() {
    resourceStatus_->setText(ResourceMonitor::memoryStatus());
}

void MainWindow::updateStatus(const QString &status) {
    status_->setText(status);
    const bool finished = status == "Completed" || status == "Cancelled"
        || status == "Model ready" || status.startsWith("Error");
    if (finished) {
        progress_->hide();
        translateButton_->setEnabled(status == "Completed"
            || status == "Cancelled" || status == "Model ready");
        cancelButton_->setEnabled(false);
    }
    if (status.startsWith("Error")) {
        QMessageBox::warning(this, "Translation unavailable", status);
    }
}
