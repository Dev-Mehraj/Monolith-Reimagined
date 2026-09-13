#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QComboBox>
#include <QProgressBar>
#include <QSplitter>
#include <QStackedWidget>
#include <QListWidget>
#include <QToolButton>
#include <QMenu>
#include <QAction>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <memory>
#include <vector>

#include "LlamaClient.h"

namespace monolith {

// Chat message bubble widget
class ChatMessageBubble : public QWidget {
    Q_OBJECT

public:
    explicit ChatMessageBubble(const QString& content, const QString& role, 
                                bool isStreaming = false, QWidget* parent = nullptr);
    
    void appendContent(const QString& content);
    void setFinished(bool finished);
    void addToolCallIndicator(const ToolCall& toolCall);
    
    QString content() const { return m_content; }
    QString role() const { return m_role; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    struct Private;
    std::unique_ptr<Private> d;
    
    QString m_content;
    QString m_role;
    bool m_isStreaming;
    bool m_finished;
};

// Tool execution result display
class ToolResultWidget : public QWidget {
    Q_OBJECT

public:
    explicit ToolResultWidget(const ToolCall& toolCall, QWidget* parent = nullptr);
    
    void setResult(const ToolResult& result);
    void setLoading(bool loading);
    
signals:
    void retryClicked();
    void cancelClicked();

private:
    struct Private;
    std::unique_ptr<Private> d;
};

// Code block with syntax highlighting and copy button
class CodeBlockWidget : public QWidget {
    Q_OBJECT

public:
    explicit CodeBlockWidget(const QString& code, const QString& language, QWidget* parent = nullptr);
    
    QString code() const { return m_code; }
    QString language() const { return m_language; }

signals:
    void copyClicked();
    void insertIntoEditorClicked();

private:
    struct Private;
    std::unique_ptr<Private> d;
    
    QString m_code;
    QString m_language;
};

// Main AI Panel widget
class AIPanel : public QWidget {
    Q_OBJECT

public:
    explicit AIPanel(QWidget* parent = nullptr);
    ~AIPanel();
    
    // Initialize with Llama.cpp client
    void setLlamaClient(LlamaClient* client);
    LlamaClient* llamaClient() const { return m_llamaClient; }
    
    // Register built-in IDE tools
    void registerIdeTools();
    
    // Send a message to the AI
    void sendMessage(const QString& message);
    
    // Clear chat history
    void clearChat();
    
    // Load/save chat session
    void loadSession(const QString& filePath);
    void saveSession(const QString& filePath);
    
    // Focus input
    void focusInput();

public slots:
    // Handle tool execution results from external sources
    void onToolExecuted(const ToolResult& result);

signals:
    // User sent a message
    void messageSent(const QString& message);
    
    // Tool execution requested
    void executeToolRequested(const ToolCall& toolCall);
    
    // Code insertion requested
    void insertCodeRequested(const QString& code, int position);
    
    // File operation requested
    void fileOperationRequested(const QString& action, const QString& filePath);
    
    // Search in workspace requested
    void searchRequested(const QString& query);
    
    // Panel visibility changed
    void visibilityChanged(bool visible);

private slots:
    void onTokenReceived(const QString& token);
    void onResponseComplete(const QString& fullResponse);
    void onToolCallRequested(const ToolCall& toolCall);
    void onErrorOccurred(const QString& errorMessage);
    void onGenerationStarted();
    void onGenerationStopped();
    void onModelLoaded(bool success);
    
    void onSendClicked();
    void onStopClicked();
    void onTextChanged();
    void onClearClicked();
    void onSettingsClicked();
    void onModelSelected(int index);
    void onExportChat();
    void onImportChat();

private:
    struct Private;
    std::unique_ptr<Private> d;
    
    void setupUI();
    void setupToolBar();
    void setupChatArea();
    void setupInputArea();
    void setupSettingsPanel();
    
    void addMessageToChat(const QString& content, const QString& role, bool isStreaming = false);
    void updateLastMessage(const QString& content);
    void addToolCallWidget(const ToolCall& toolCall);
    void scrollToBottom();
    
    void executeBuiltInTool(const ToolCall& toolCall);
    QJsonObject buildToolSchemas();
    
    LlamaClient* m_llamaClient = nullptr;
    bool m_ownClient = false;
    
    // UI components
    QVBoxLayout* m_mainLayout;
    QStackedWidget* m_stackedWidget;
    
    // Chat area
    QWidget* m_chatContainer;
    QVBoxLayout* m_chatLayout;
    QScrollArea* m_scrollArea;
    
    // Input area
    QHBoxLayout* m_inputLayout;
    QTextEdit* m_inputEdit;
    QPushButton* m_sendButton;
    QPushButton* m_stopButton;
    QToolButton* m_attachButton;
    QToolButton* m_settingsButton;
    
    // Status bar
    QHBoxLayout* m_statusLayout;
    QLabel* m_statusLabel;
    QProgressBar* m_progressBar;
    
    // Settings panel
    QWidget* m_settingsPanel;
    QComboBox* m_modelCombo;
    QComboBox* m_temperatureCombo;
    QComboBox* m_maxTokensCombo;
    
    // State
    bool m_generating = false;
    ChatMessageBubble* m_currentBubble = nullptr;
    QVector<ChatMessage> m_messageHistory;
};

} // namespace monolith
