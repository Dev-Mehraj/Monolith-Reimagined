#include "AIPanel.h"

#include <QPainter>
#include <QTextDocument>
#include <QSyntaxHighlighter>
#include <QClipboard>
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QScrollBar>
#include <QFrame>
#include <QStackedLayout>

namespace monolith {

// ============================================================================
// ChatMessageBubble Implementation
// ============================================================================

struct ChatMessageBubble::Private {
    QLabel* contentLabel = nullptr;
    QVBoxLayout* layout = nullptr;
    QFrame* borderFrame = nullptr;
};

ChatMessageBubble::ChatMessageBubble(const QString& content, const QString& role,
                                      bool isStreaming, QWidget* parent)
    : QWidget(parent)
    , d(std::make_unique<Private>())
    , m_content(content)
    , m_role(role)
    , m_isStreaming(isStreaming)
    , m_finished(false)
{
    setObjectName("ChatMessageBubble");
    
    d->layout = new QVBoxLayout(this);
    d->layout->setContentsMargins(12, 8, 12, 8);
    d->layout->setSpacing(4);
    
    // Create border frame for visual separation
    d->borderFrame = new QFrame(this);
    d->borderFrame->setFrameShape(QFrame::StyledPanel);
    d->borderFrame->setObjectName(role == "user" ? "UserMessageFrame" : "AIMessageFrame");
    
    d->contentLabel = new QLabel(this);
    d->contentLabel->setWordWrap(true);
    d->contentLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
    d->contentLabel->setOpenExternalLinks(true);
    
    // Set styling based on role
    if (role == "user") {
        d->contentLabel->setStyleSheet(
            "QLabel { "
            "  background-color: #0e639c; "
            "  color: #ffffff; "
            "  padding: 8px 12px; "
            "  border-radius: 8px; "
            "  font-size: 13px; "
            "}");
        d->borderFrame->setStyleSheet(
            "QFrame { "
            "  background-color: #0e639c; "
            "  border-radius: 8px; "
            "}");
    } else {
        d->contentLabel->setStyleSheet(
            "QLabel { "
            "  background-color: #1e1e1e; "
            "  color: #d4d4d4; "
            "  padding: 8px 12px; "
            "  border-radius: 8px; "
            "  font-size: 13px; "
            "}");
        d->borderFrame->setStyleSheet(
            "QFrame { "
            "  background-color: #252526; "
            "  border-radius: 8px; "
            "}");
    }
    
    d->layout->addWidget(d->contentLabel);
    
    appendContent(content);
}

void ChatMessageBubble::appendContent(const QString& content) {
    m_content += content;
    
    // Convert markdown-like syntax to HTML for display
    QString html = m_content.toHtmlEscaped();
    
    // Handle code blocks
    html.replace(QRegularExpression("```(\\w*)\\n([\\s\\S]*?)```"), 
                 R"(<pre style="background:#1e1e1e;padding:8px;border-radius:4px;"><code>\2</code></pre>)");
    
    // Handle inline code
    html.replace(QRegularExpression("`([^`]+)`"), 
                 R"(<code style="background:#3c3c3c;padding:2px 4px;border-radius:3px;">\1</code>)");
    
    // Handle bold
    html.replace(QRegularExpression("\\*\\*([^*]+)\\*\\*"), "<b>\\1</b>");
    
    // Handle italic
    html.replace(QRegularExpression("\\*([^*]+)\\*"), "<i>\\1</i>");
    
    // Handle line breaks
    html.replace("\n", "<br>");
    
    d->contentLabel->setText(html);
}

void ChatMessageBubble::setFinished(bool finished) {
    m_finished = finished;
    if (finished && m_isStreaming) {
        // Add subtle animation end effect
        setStyleSheet("");
    }
}

void ChatMessageBubble::addToolCallIndicator(const ToolCall& toolCall) {
    QLabel* toolLabel = new QLabel(this);
    toolLabel->setObjectName("ToolCallIndicator");
    toolLabel->setText(QString("🔧 Calling tool: %1").arg(toolCall.name));
    toolLabel->setStyleSheet(
        "QLabel { "
        "  background-color: #2d2d30; "
        "  color: #4ec9b0; "
        "  padding: 6px 10px; "
        "  border-radius: 4px; "
        "  font-size: 12px; "
        "}");
    d->layout->addWidget(toolLabel);
}

void ChatMessageBubble::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw rounded rectangle background
    QColor bgColor = m_role == "user" ? QColor("#0e639c") : QColor("#252526");
    painter.setBrush(bgColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 8, 8);
    
    QWidget::paintEvent(event);
}

// ============================================================================
// ToolResultWidget Implementation
// ============================================================================

struct ToolResultWidget::Private {
    QLabel* nameLabel = nullptr;
    QLabel* statusLabel = nullptr;
    QProgressBar* progressBar = nullptr;
    QTextEdit* resultEdit = nullptr;
    QPushButton* retryButton = nullptr;
    QPushButton* cancelButton = nullptr;
    QHBoxLayout* buttonLayout = nullptr;
    bool isLoading = false;
};

ToolResultWidget::ToolResultWidget(const ToolCall& toolCall, QWidget* parent)
    : QWidget(parent)
    , d(std::make_unique<Private>())
{
    setObjectName("ToolResultWidget");
    
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);
    
    // Header with tool name
    auto* headerLayout = new QHBoxLayout();
    d->nameLabel = new QLabel(toolCall.name, this);
    d->nameLabel->setStyleSheet("font-weight: bold; color: #4ec9b0; font-size: 13px;");
    headerLayout->addWidget(d->nameLabel);
    
    d->statusLabel = new QLabel("Executing...", this);
    d->statusLabel->setStyleSheet("color: #dcdcaa; font-size: 12px;");
    headerLayout->addWidget(d->statusLabel);
    headerLayout->addStretch();
    
    layout->addLayout(headerLayout);
    
    // Progress bar
    d->progressBar = new QProgressBar(this);
    d->progressBar->setRange(0, 0); // Indeterminate
    d->progressBar->setFormat("");
    d->progressBar->setFixedHeight(4);
    layout->addWidget(d->progressBar);
    
    // Result area (hidden initially)
    d->resultEdit = new QTextEdit(this);
    d->resultEdit->setReadOnly(true);
    d->resultEdit->setMaximumHeight(150);
    d->resultEdit->setHidden(true);
    d->resultEdit->setStyleSheet(
        "QTextEdit { "
        "  background-color: #1e1e1e; "
        "  color: #d4d4d4; "
        "  border: 1px solid #3c3c3c; "
        "  border-radius: 4px; "
        "  font-family: 'Consolas', 'Monaco', monospace; "
        "  font-size: 12px; "
        "}");
    layout->addWidget(d->resultEdit);
    
    // Buttons
    d->buttonLayout = new QHBoxLayout();
    d->retryButton = new QPushButton("Retry", this);
    d->retryButton->setHidden(true);
    connect(d->retryButton, &QPushButton::clicked, this, &ToolResultWidget::retryClicked);
    d->buttonLayout->addWidget(d->retryButton);
    
    d->cancelButton = new QPushButton("Cancel", this);
    connect(d->cancelButton, &QPushButton::clicked, this, &ToolResultWidget::cancelClicked);
    d->buttonLayout->addWidget(d->cancelButton);
    d->buttonLayout->addStretch();
    
    layout->addLayout(d->buttonLayout);
    
    setStyleSheet(
        "ToolResultWidget { "
        "  background-color: #2d2d30; "
        "  border-radius: 6px; "
        "  margin: 4px 0; "
        "}");
}

void ToolResultWidget::setResult(const ToolResult& result) {
    d->isLoading = false;
    d->progressBar->setHidden(true);
    d->resultEdit->setHidden(false);
    d->cancelButton->setHidden(true);
    
    if (result.isError) {
        d->statusLabel->setText("Failed");
        d->statusLabel->setStyleSheet("color: #f44747; font-size: 12px;");
        d->resultEdit->setStyleSheet(
            "QTextEdit { "
            "  background-color: #3a1d1d; "
            "  color: #f44747; "
            "  border: 1px solid #f44747; "
            "  border-radius: 4px; "
            "}");
        d->retryButton->setHidden(false);
    } else {
        d->statusLabel->setText("Completed");
        d->statusLabel->setStyleSheet("color: #6a9955; font-size: 12px;");
        d->resultEdit->setStyleSheet(
            "QTextEdit { "
            "  background-color: #1e1e1e; "
            "  color: #d4d4d4; "
            "  border: 1px solid #3c3c3c; "
            "  border-radius: 4px; "
            "}");
    }
    
    d->resultEdit->setPlainText(result.content);
}

void ToolResultWidget::setLoading(bool loading) {
    d->isLoading = loading;
    d->progressBar->setHidden(!loading);
    d->statusLabel->setText(loading ? "Executing..." : "");
    d->resultEdit->setHidden(loading);
    d->cancelButton->setHidden(!loading);
    d->retryButton->setHidden(true);
}

// ============================================================================
// CodeBlockWidget Implementation
// ============================================================================

struct CodeBlockWidget::Private {
    QTextEdit* codeEdit = nullptr;
    QLabel* languageLabel = nullptr;
    QPushButton* copyButton = nullptr;
    QPushButton* insertButton = nullptr;
    QHBoxLayout* headerLayout = nullptr;
};

CodeBlockWidget::CodeBlockWidget(const QString& code, const QString& language, QWidget* parent)
    : QWidget(parent)
    , d(std::make_unique<Private>())
    , m_code(code)
    , m_language(language)
{
    setObjectName("CodeBlockWidget");
    
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Header bar
    auto* headerWidget = new QWidget(this);
    d->headerLayout = new QHBoxLayout(headerWidget);
    d->headerLayout->setContentsMargins(8, 4, 8, 4);
    headerWidget->setStyleSheet("background-color: #2d2d30;");
    
    d->languageLabel = new QLabel(language.isEmpty() ? "code" : language, headerWidget);
    d->languageLabel->setStyleSheet("color: #4ec9b0; font-size: 11px; font-weight: bold;");
    d->headerLayout->addWidget(d->languageLabel);
    
    d->headerLayout->addStretch();
    
    d->insertButton = new QPushButton("Insert", headerWidget);
    d->insertButton->setFlat(true);
    d->insertButton->setCursor(Qt::PointingHandCursor);
    d->insertButton->setStyleSheet(
        "QPushButton { "
        "  color: #4ec9b0; "
        "  padding: 2px 8px; "
        "  border-radius: 3px; "
        "} "
        "QPushButton:hover { background-color: #3c3c3c; }");
    connect(d->insertButton, &QPushButton::clicked, this, &CodeBlockWidget::insertIntoEditorClicked);
    d->headerLayout->addWidget(d->insertButton);
    
    d->copyButton = new QPushButton("Copy", headerWidget);
    d->copyButton->setFlat(true);
    d->copyButton->setCursor(Qt::PointingHandCursor);
    d->copyButton->setStyleSheet(
        "QPushButton { "
        "  color: #d4d4d4; "
        "  padding: 2px 8px; "
        "  border-radius: 3px; "
        "} "
        "QPushButton:hover { background-color: #3c3c3c; }");
    connect(d->copyButton, &QPushButton::clicked, [this]() {
        QApplication::clipboard()->setText(m_code);
        d->copyButton->setText("Copied!");
        QTimer::singleShot(1500, [this]() { d->copyButton->setText("Copy"); });
    });
    d->headerLayout->addWidget(d->copyButton);
    
    mainLayout->addWidget(headerWidget);
    
    // Code editor
    d->codeEdit = new QTextEdit(this);
    d->codeEdit->setPlainText(code);
    d->codeEdit->setReadOnly(true);
    d->codeEdit->setFont(QFont("Consolas", 12));
    d->codeEdit->setLineWrapMode(QTextEdit::NoWrap);
    d->codeEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    d->codeEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    d->codeEdit->setStyleSheet(
        "QTextEdit { "
        "  background-color: #1e1e1e; "
        "  color: #d4d4d4; "
        "  border: none; "
        "  padding: 8px; "
        "} "
        "QScrollBar:vertical { "
        "  background: #1e1e1e; "
        "  width: 10px; "
        "} "
        "QScrollBar::handle:vertical { "
        "  background: #424242; "
        "  border-radius: 5px; "
        "  min-height: 20px; "
        "}");
    
    mainLayout->addWidget(d->codeEdit);
    
    setStyleSheet("CodeBlockWidget { border: 1px solid #3c3c3c; border-radius: 6px; }");
}

// ============================================================================
// AIPanel Implementation
// ============================================================================

struct AIPanel::Private {
    LlamaClient* client = nullptr;
    QMenu* settingsMenu = nullptr;
    QAction* clearAction = nullptr;
    QAction* exportAction = nullptr;
    QAction* importAction = nullptr;
    QAction* configAction = nullptr;
};

AIPanel::AIPanel(QWidget* parent)
    : QWidget(parent)
    , d(std::make_unique<Private>())
{
    setObjectName("AIPanel");
    setupUI();
    registerIdeTools();
}

AIPanel::~AIPanel() {
    if (m_ownClient && m_llamaClient) {
        delete m_llamaClient;
    }
}

void AIPanel::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    
    m_stackedWidget = new QStackedWidget(this);
    m_mainLayout->addWidget(m_stackedWidget);
    
    // Main chat widget
    auto* chatWidget = new QWidget(this);
    auto* chatMainLayout = new QVBoxLayout(chatWidget);
    chatMainLayout->setContentsMargins(0, 0, 0, 0);
    chatMainLayout->setSpacing(0);
    
    setupToolBar();
    setupChatArea();
    setupInputArea();
    setupSettingsPanel();
    
    chatMainLayout->addWidget(m_scrollArea);
    chatMainLayout->addLayout(m_inputLayout);
    chatMainLayout->addLayout(m_statusLayout);
    
    m_stackedWidget->addWidget(chatWidget);
    m_stackedWidget->addWidget(m_settingsPanel);
    
    setStyleSheet(
        "AIPanel { "
        "  background-color: #1e1e1e; "
        "} "
        "#ChatMessageBubble { "
        "  margin: 4px 8px; "
        "} "
        "QScrollArea { "
        "  border: none; "
        "  background-color: #1e1e1e; "
        "} "
        "QScrollBar:vertical { "
        "  background: #1e1e1e; "
        "  width: 10px; "
        "} "
        "QScrollBar::handle:vertical { "
        "  background: #424242; "
        "  border-radius: 5px; "
        "  min-height: 20px; "
        "} "
        "QScrollBar::handle:vertical:hover { "
        "  background: #4f4f4f; "
        "}");
}

void AIPanel::setupToolBar() {
    // Tool bar will be added in a real implementation
    // For now, settings are accessed via the settings button
}

void AIPanel::setupChatArea() {
    m_chatContainer = new QWidget(this);
    m_chatLayout = new QVBoxLayout(m_chatContainer);
    m_chatLayout->setContentsMargins(8, 8, 8, 8);
    m_chatLayout->setSpacing(8);
    m_chatLayout->addStretch();
    
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setWidget(m_chatContainer);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void AIPanel::setupInputArea() {
    m_inputLayout = new QHBoxLayout();
    m_inputLayout->setContentsMargins(8, 8, 8, 8);
    m_inputLayout->setSpacing(8);
    
    // Attach button (for files, etc.)
    m_attachButton = new QToolButton(this);
    m_attachButton->setText("📎");
    m_attachButton->setFixedSize(36, 36);
    m_attachButton->setToolTip("Attach file or selection");
    m_attachButton->setStyleSheet(
        "QToolButton { "
        "  background-color: #2d2d30; "
        "  border: 1px solid #3c3c3c; "
        "  border-radius: 6px; "
        "  font-size: 16px; "
        "} "
        "QToolButton:hover { background-color: #3c3c3c; }");
    m_inputLayout->addWidget(m_attachButton);
    
    // Input text edit
    m_inputEdit = new QTextEdit(this);
    m_inputEdit->setPlaceholderText("Ask AI anything... (Shift+Enter for newline)");
    m_inputEdit->setAcceptRichText(false);
    m_inputEdit->setMaximumHeight(120);
    m_inputEdit->setMinimumHeight(40);
    m_inputEdit->setFont(QFont("Segoe UI", 13));
    m_inputEdit->setStyleSheet(
        "QTextEdit { "
        "  background-color: #2d2d30; "
        "  color: #d4d4d4; "
        "  border: 1px solid #3c3c3c; "
        "  border-radius: 6px; "
        "  padding: 8px 12px; "
        "} "
        "QTextEdit:focus { "
        "  border: 1px solid #0e639c; "
        "} "
        "QTextEdit:disabled { "
        "  background-color: #1e1e1e; "
        "}");
    
    connect(m_inputEdit, &QTextEdit::textChanged, this, &AIPanel::onTextChanged);
    
    m_inputLayout->addWidget(m_inputEdit, 1);
    
    // Send button
    m_sendButton = new QPushButton("Send", this);
    m_sendButton->setFixedSize(70, 36);
    m_sendButton->setEnabled(false);
    m_sendButton->setStyleSheet(
        "QPushButton { "
        "  background-color: #0e639c; "
        "  color: white; "
        "  border: none; "
        "  border-radius: 6px; "
        "  font-weight: bold; "
        "} "
        "QPushButton:hover { background-color: #1177bb; } "
        "QPushButton:pressed { background-color: #0d5a8f; } "
        "QPushButton:disabled { "
        "  background-color: #3c3c3c; "
        "  color: #808080; "
        "}");
    connect(m_sendButton, &QPushButton::clicked, this, &AIPanel::onSendClicked);
    m_inputLayout->addWidget(m_sendButton);
    
    // Stop button (hidden initially)
    m_stopButton = new QPushButton("Stop", this);
    m_stopButton->setFixedSize(70, 36);
    m_stopButton->setHidden(true);
    m_stopButton->setStyleSheet(
        "QPushButton { "
        "  background-color: #f44747; "
        "  color: white; "
        "  border: none; "
        "  border-radius: 6px; "
        "  font-weight: bold; "
        "} "
        "QPushButton:hover { background-color: #ff5555; }");
    connect(m_stopButton, &QPushButton::clicked, this, &AIPanel::onStopClicked);
    m_inputLayout->addWidget(m_stopButton);
    
    // Settings button
    m_settingsButton = new QToolButton(this);
    m_settingsButton->setText("⚙️");
    m_settingsButton->setFixedSize(36, 36);
    m_settingsButton->setToolTip("Settings");
    m_settingsButton->setStyleSheet(
        "QToolButton { "
        "  background-color: #2d2d30; "
        "  border: 1px solid #3c3c3c; "
        "  border-radius: 6px; "
        "  font-size: 16px; "
        "} "
        "QToolButton:hover { background-color: #3c3c3c; }");
    connect(m_settingsButton, &QToolButton::clicked, this, &AIPanel::onSettingsClicked);
    m_inputLayout->addWidget(m_settingsButton);
}

void AIPanel::setupSettingsPanel() {
    m_settingsPanel = new QWidget(this);
    auto* layout = new QVBoxLayout(m_settingsPanel);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);
    
    auto* titleLabel = new QLabel("AI Settings", m_settingsPanel);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #d4d4d4;");
    layout->addWidget(titleLabel);
    
    // Model selection
    layout->addWidget(new QLabel("Model:", m_settingsPanel));
    m_modelCombo = new QComboBox(m_settingsPanel);
    m_modelCombo->addItem("Llama-3.2-3B-Instruct", "models/llama-3.2-3b-instruct.Q4_K_M.gguf");
    m_modelCombo->addItem("Llama-3.2-7B-Instruct", "models/llama-3.2-7b-instruct.Q4_K_M.gguf");
    m_modelCombo->addItem("Mistral-7B-Instruct", "models/mistral-7b-instruct-v0.3.Q4_K_M.gguf");
    m_modelCombo->addItem("Phi-3-Mini", "models/phi-3-mini-4k-instruct.Q4_K_M.gguf");
    m_modelCombo->setStyleSheet(
        "QComboBox { "
        "  background-color: #2d2d30; "
        "  color: #d4d4d4; "
        "  border: 1px solid #3c3c3c; "
        "  border-radius: 4px; "
        "  padding: 6px 10px; "
        "} "
        "QComboBox::drop-down { "
        "  border: none; "
        "  width: 20px; "
        "} "
        "QComboBox QAbstractItemView { "
        "  background-color: #2d2d30; "
        "  color: #d4d4d4; "
        "  border: 1px solid #3c3c3c; "
        "  selection-background-color: #0e639c; "
        "}");
    connect(m_modelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AIPanel::onModelSelected);
    layout->addWidget(m_modelCombo);
    
    // Temperature
    layout->addWidget(new QLabel("Temperature:", m_settingsPanel));
    m_temperatureCombo = new QComboBox(m_settingsPanel);
    m_temperatureCombo->addItem("Creative (1.0)", 10);
    m_temperatureCombo->addItem("Balanced (0.7)", 7);
    m_temperatureCombo->addItem("Precise (0.3)", 3);
    m_temperatureCombo->addItem("Deterministic (0.0)", 0);
    m_temperatureCombo->setCurrentIndex(1);
    m_temperatureCombo->setStyleSheet(m_modelCombo->styleSheet());
    layout->addWidget(m_temperatureCombo);
    
    // Max tokens
    layout->addWidget(new QLabel("Max Tokens:", m_settingsPanel));
    m_maxTokensCombo = new QComboBox(m_settingsPanel);
    m_maxTokensCombo->addItem("Short (512)", 512);
    m_maxTokensCombo->addItem("Medium (2048)", 2048);
    m_maxTokensCombo->addItem("Long (4096)", 4096);
    m_maxTokensCombo->addItem("Very Long (8192)", 8192);
    m_maxTokensCombo->setCurrentIndex(1);
    m_maxTokensCombo->setStyleSheet(m_modelCombo->styleSheet());
    layout->addWidget(m_maxTokensCombo);
    
    layout->addStretch();
    
    // Action buttons
    auto* buttonLayout = new QHBoxLayout();
    
    QPushButton* saveBtn = new QPushButton("Save Settings", m_settingsPanel);
    saveBtn->setStyleSheet(
        "QPushButton { "
        "  background-color: #0e639c; "
        "  color: white; "
        "  border: none; "
        "  border-radius: 4px; "
        "  padding: 8px 16px; "
        "}");
    buttonLayout->addWidget(saveBtn);
    
    QPushButton* backBtn = new QPushButton("Back", m_settingsPanel);
    backBtn->setStyleSheet(
        "QPushButton { "
        "  background-color: #3c3c3c; "
        "  color: #d4d4d4; "
        "  border: none; "
        "  border-radius: 4px; "
        "  padding: 8px 16px; "
        "}");
    connect(backBtn, &QPushButton::clicked, [this]() {
        m_stackedWidget->setCurrentIndex(0);
    });
    buttonLayout->addWidget(backBtn);
    
    layout->addLayout(buttonLayout);
}

void AIPanel::setLlamaClient(LlamaClient* client) {
    if (m_ownClient && m_llamaClient) {
        delete m_llamaClient;
    }
    
    m_llamaClient = client;
    m_ownClient = false;
    
    if (m_llamaClient) {
        connect(m_llamaClient, &LlamaClient::tokenReceived,
                this, &AIPanel::onTokenReceived);
        connect(m_llamaClient, &LlamaClient::responseComplete,
                this, &AIPanel::onResponseComplete);
        connect(m_llamaClient, &LlamaClient::toolCallRequested,
                this, &AIPanel::onToolCallRequested);
        connect(m_llamaClient, &LlamaClient::errorOccurred,
                this, &AIPanel::onErrorOccurred);
        connect(m_llamaClient, &LlamaClient::generationStarted,
                this, &AIPanel::onGenerationStarted);
        connect(m_llamaClient, &LlamaClient::generationStopped,
                this, &AIPanel::onGenerationStopped);
        connect(m_llamaClient, &LlamaClient::modelLoaded,
                this, &AIPanel::onModelLoaded);
    }
}

void AIPanel::registerIdeTools() {
    if (!m_llamaClient) return;
    
    // Register built-in IDE tools
    
    // 1. Read File Tool
    ToolDefinition readFileTool;
    readFileTool.name = "read_file";
    readFileTool.description = "Read the contents of a file in the workspace";
    readFileTool.parameters = QJsonObject{
        {"type", "object"},
        {"properties", QJsonObject{
            {"path", QJsonObject{
                {"type", "string"},
                {"description", "The relative or absolute path to the file"}
            }},
            {"limit", QJsonObject{
                {"type", "integer"},
                {"description", "Maximum number of lines to read (default: 100)"}
            }}
        }},
        {"required", QJsonArray{"path"}}
    };
    m_llamaClient->registerTool(readFileTool);
    
    // 2. Write File Tool
    ToolDefinition writeFileTool;
    writeFileTool.name = "write_file";
    writeFileTool.description = "Write content to a file (creates or overwrites)";
    writeFileTool.parameters = QJsonObject{
        {"type", "object"},
        {"properties", QJsonObject{
            {"path", QJsonObject{
                {"type", "string"},
                {"description", "The relative or absolute path to the file"}
            }},
            {"content", QJsonObject{
                {"type", "string"},
                {"description", "The content to write to the file"}
            }}
        }},
        {"required", QJsonArray{"path", "content"}}
    };
    m_llamaClient->registerTool(writeFileTool);
    
    // 3. Search in Files Tool
    ToolDefinition searchTool;
    searchTool.name = "search_in_files";
    searchTool.description = "Search for a pattern in all files of the workspace using ripgrep";
    searchTool.parameters = QJsonObject{
        {"type", "object"},
        {"properties", QJsonObject{
            {"pattern", QJsonObject{
                {"type", "string"},
                {"description", "The search pattern (regex supported)"}
            }},
            {"caseSensitive", QJsonObject{
                {"type", "boolean"},
                {"description", "Whether the search is case-sensitive"}
            }},
            {"filePattern", QJsonObject{
                {"type", "string"},
                {"description", "Optional glob pattern to filter files"}
            }}
        }},
        {"required", QJsonArray{"pattern"}}
    };
    m_llamaClient->registerTool(searchTool);
    
    // 4. List Directory Tool
    ToolDefinition listDirTool;
    listDirTool.name = "list_directory";
    listDirTool.description = "List contents of a directory";
    listDirTool.parameters = QJsonObject{
        {"type", "object"},
        {"properties", QJsonObject{
            {"path", QJsonObject{
                {"type", "string"},
                {"description", "The directory path to list"}
            }},
            {"recursive", QJsonObject{
                {"type", "boolean"},
                {"description", "Whether to list recursively"}
            }}
        }},
        {"required", QJsonArray{"path"}}
    };
    m_llamaClient->registerTool(listDirTool);
    
    // 5. Run Command Tool
    ToolDefinition runCommandTool;
    runCommandTool.name = "run_terminal_command";
    runCommandTool.description = "Execute a command in the integrated terminal";
    runCommandTool.parameters = QJsonObject{
        {"type", "object"},
        {"properties", QJsonObject{
            {"command", QJsonObject{
                {"type", "string"},
                {"description", "The shell command to execute"}
            }},
            {"workingDirectory", QJsonObject{
                {"type", "string"},
                {"description", "Working directory for the command"}
            }}
        }},
        {"required", QJsonArray{"command"}}
    };
    m_llamaClient->registerTool(runCommandTool);
    
    // 6. Get Selection Tool
    ToolDefinition getSelectionTool;
    getSelectionTool.name = "get_editor_selection";
    getSelectionTool.description = "Get the currently selected text in the active editor";
    getSelectionTool.parameters = QJsonObject{
        {"type", "object"},
        {"properties", QJsonObject{}}
    };
    m_llamaClient->registerTool(getSelectionTool);
    
    // 7. Insert Code Tool
    ToolDefinition insertCodeTool;
    insertCodeTool.name = "insert_code_at_cursor";
    insertCodeTool.description = "Insert code at the current cursor position in the active editor";
    insertCodeTool.parameters = QJsonObject{
        {"type", "object"},
        {"properties", QJsonObject{
            {"code", QJsonObject{
                {"type", "string"},
                {"description", "The code to insert"}
            }}
        }},
        {"required", QJsonArray{"code"}}
    };
    m_llamaClient->registerTool(insertCodeTool);
    
    // 8. Get Diagnostics Tool
    ToolDefinition getDiagnosticsTool;
    getDiagnosticsTool.name = "get_file_diagnostics";
    getDiagnosticsTool.description = "Get LSP diagnostics (errors, warnings) for a file";
    getDiagnosticsTool.parameters = QJsonObject{
        {"type", "object"},
        {"properties", QJsonObject{
            {"uri", QJsonObject{
                {"type", "string"},
                {"description", "The file URI to get diagnostics for"}
            }}
        }},
        {"required", QJsonArray{"uri"}}
    };
    m_llamaClient->registerTool(getDiagnosticsTool);
    
    // 9. Git Status Tool
    ToolDefinition gitStatusTool;
    gitStatusTool.name = "get_git_status";
    gitStatusTool.description = "Get the Git status of the workspace or a specific file";
    getDiagnosticsTool.parameters = QJsonObject{
        {"type", "object"},
        {"properties", QJsonObject{
            {"path", QJsonObject{
                {"type", "string"},
                {"description", "Optional file path to check status for"}
            }}
        }}
    };
    m_llamaClient->registerTool(gitStatusTool);
}

void AIPanel::sendMessage(const QString& message) {
    if (message.trimmed().isEmpty() || !m_llamaClient) return;
    
    // Add user message to chat
    addMessageToChat(message, "user");
    
    // Send to LLM
    m_llamaClient->sendRequest(message, true);
    
    // Clear input
    m_inputEdit->clear();
    onTextChanged();
}

void AIPanel::clearChat() {
    // Remove all message bubbles
    while (m_chatLayout->count() > 1) { // Keep the stretch
        auto* item = m_chatLayout->takeAt(0);
        if (item->widget()) {
            delete item->widget();
        }
    }
    
    m_messageHistory.clear();
    m_currentBubble = nullptr;
}

void AIPanel::focusInput() {
    m_inputEdit->setFocus();
}

void AIPanel::onToolExecuted(const ToolResult& result) {
    if (m_llamaClient) {
        m_llamaClient->submitToolResult(result);
    }
}

void AIPanel::onTokenReceived(const QString& token) {
    if (m_currentBubble) {
        m_currentBubble->appendContent(token);
        scrollToBottom();
    }
}

void AIPanel::onResponseComplete(const QString& fullResponse) {
    if (m_currentBubble) {
        m_currentBubble->setFinished(true);
        m_currentBubble = nullptr;
    }
    
    m_generating = false;
    m_sendButton->setEnabled(true);
    m_stopButton->setHidden(true);
    m_sendButton->setHidden(false);
    m_inputEdit->setEnabled(true);
    
    m_statusLabel->setText("Ready");
    m_progressBar->setHidden(true);
}

void AIPanel::onToolCallRequested(const ToolCall& toolCall) {
    addToolCallWidget(toolCall);
    emit executeToolRequested(toolCall);
}

void AIPanel::onErrorOccurred(const QString& errorMessage) {
    QMessageBox::warning(this, "AI Error", errorMessage);
    
    m_generating = false;
    m_sendButton->setEnabled(true);
    m_stopButton->setHidden(true);
    m_sendButton->setHidden(false);
    m_inputEdit->setEnabled(true);
    
    m_statusLabel->setText("Error");
    m_progressBar->setHidden(true);
}

void AIPanel::onGenerationStarted() {
    m_generating = true;
    m_sendButton->setHidden(true);
    m_stopButton->setHidden(false);
    m_inputEdit->setEnabled(false);
    
    m_statusLabel->setText("Generating...");
    m_progressBar->setHidden(false);
    
    // Create streaming message bubble
    m_currentBubble = new ChatMessageBubble("", "assistant", true, this);
    
    // Insert before the stretch
    int count = m_chatLayout->count();
    m_chatLayout->insertWidget(count - 1, m_currentBubble);
    
    scrollToBottom();
}

void AIPanel::onGenerationStopped() {
    m_generating = false;
    m_sendButton->setEnabled(true);
    m_stopButton->setHidden(true);
    m_sendButton->setHidden(false);
    m_inputEdit->setEnabled(true);
}

void AIPanel::onModelLoaded(bool success) {
    if (success) {
        m_statusLabel->setText("Model loaded and ready");
    } else {
        m_statusLabel->setText("Failed to load model");
    }
}

void AIPanel::onSendClicked() {
    sendMessage(m_inputEdit->toPlainText());
}

void AIPanel::onStopClicked() {
    if (m_llamaClient) {
        m_llamaClient->abortGeneration();
    }
}

void AIPanel::onTextChanged() {
    bool hasText = !m_inputEdit->toPlainText().trimmed().isEmpty();
    m_sendButton->setEnabled(hasText && !m_generating);
}

void AIPanel::onClearClicked() {
    clearChat();
}

void AIPanel::onSettingsClicked() {
    m_stackedWidget->setCurrentIndex(1);
}

void AIPanel::onModelSelected(int index) {
    Q_UNUSED(index);
    // Would reload model with new settings in a full implementation
}

void AIPanel::onExportChat() {
    QString fileName = QFileDialog::getSaveFileName(
        this, "Export Chat", "", "Markdown Files (*.md);;All Files (*)");
    
    if (fileName.isEmpty()) return;
    
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "# Monolith AI Chat\n\n";
        out << "*Exported: " << QDateTime::currentDateTime().toString() << "*\n\n";
        
        for (const auto& msg : m_messageHistory) {
            out << "## " << msg.role.toUpper() << "\n\n";
            out << msg.content << "\n\n";
        }
        
        file.close();
    }
}

void AIPanel::onImportChat() {
    QString fileName = QFileDialog::getOpenFileName(
        this, "Import Chat", "", "Markdown Files (*.md);;All Files (*)");
    
    if (fileName.isEmpty()) return;
    
    // Parse and load chat session
    // Implementation would parse markdown and recreate messages
}

void AIPanel::addMessageToChat(const QString& content, const QString& role, bool isStreaming) {
    auto* bubble = new ChatMessageBubble(content, role, isStreaming, this);
    
    // Store in history
    ChatMessage msg;
    msg.role = role;
    msg.content = content;
    m_messageHistory.append(msg);
    
    // Insert before the stretch
    int count = m_chatLayout->count();
    m_chatLayout->insertWidget(count - 1, bubble);
    
    scrollToBottom();
}

void AIPanel::updateLastMessage(const QString& content) {
    if (!m_messageHistory.isEmpty()) {
        m_messageHistory.last().content = content;
    }
}

void AIPanel::addToolCallWidget(const ToolCall& toolCall) {
    auto* toolWidget = new ToolResultWidget(toolCall, this);
    
    // Insert before the stretch
    int count = m_chatLayout->count();
    m_chatLayout->insertWidget(count - 1, toolWidget);
    
    scrollToBottom();
}

void AIPanel::scrollToBottom() {
    QScrollBar* bar = m_scrollArea->verticalScrollBar();
    bar->setValue(bar->maximum());
}

void AIPanel::executeBuiltInTool(const ToolCall& toolCall) {
    qDebug() << "Executing tool:" << toolCall.name;
    
    // In a full implementation, this would:
    // 1. Call the appropriate IDE subsystem
    // 2. Execute the tool asynchronously
    // 3. Return results via ToolResult
    
    // For now, emit signal for external handling
    emit executeToolRequested(toolCall);
}

QJsonObject AIPanel::buildToolSchemas() {
    QJsonArray toolsArr;
    
    if (m_llamaClient) {
        for (const auto& tool : m_llamaClient->registeredTools()) {
            toolsArr.append(tool.toJson());
        }
    }
    
    QJsonObject result;
    result["tools"] = toolsArr;
    return result;
}

} // namespace monolith
