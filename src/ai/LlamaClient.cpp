#include "LlamaClient.h"

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <QTimer>
#include <QDebug>
#include <QRegularExpression>

namespace monolith {

struct LlamaClient::Private {
    QProcess* serverProcess = nullptr;
    QTimer* timeoutTimer = nullptr;
    QString serverHost = "127.0.0.1";
    quint16 serverPort = 8080;
    bool serverRunning = false;
    QByteArray pendingData;
    int requestTimeout = 120000; // 2 minutes
    
    // HTTP client for llama.cpp server
    void sendHttpRequest(const QString& path, const QJsonObject& payload);
};

LlamaClient::LlamaClient(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Private>())
{
    d->serverProcess = new QProcess(this);
    connect(d->serverProcess, &QProcess::readyReadStandardOutput,
            this, &LlamaClient::onProcessReadyReadStandardOutput);
    connect(d->serverProcess, &QProcess::readyReadStandardError,
            this, &LlamaClient::onProcessReadyReadStandardError);
    connect(d->serverProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &LlamaClient::onProcessFinished);
    
    d->timeoutTimer = new QTimer(this);
    d->timeoutTimer->setSingleShot(true);
    connect(d->timeoutTimer, &QTimer::timeout, this, &LlamaClient::onTimeout);
}

LlamaClient::~LlamaClient() {
    shutdown();
}

bool LlamaClient::initialize(const LlamaConfig& config) {
    if (m_initialized) {
        qWarning() << "LlamaClient already initialized";
        return false;
    }
    
    m_config = config;
    
    // Check if model file exists
    if (!QFile::exists(config.modelPath)) {
        emit errorOccurred(QString("Model file not found: %1").arg(config.modelPath));
        return false;
    }
    
    // Start llama.cpp server
    QStringList args = config.toArgs();
    args << "--host" << d->serverHost;
    args << "--port" << QString::number(d->serverPort);
    args << "--embedding"; // Enable embeddings for RAG
    args << "-ctk" << "q8_0"; // Use quantized KV cache
    
    QString serverPath = QCoreApplication::applicationDirPath() + "/llama-server";
#ifdef Q_OS_WIN
    serverPath += ".exe";
#endif
    
    qDebug() << "Starting llama.cpp server:" << serverPath << args;
    
    d->serverProcess->start(serverPath, args);
    
    if (!d->serverProcess->waitForStarted(5000)) {
        emit errorOccurred("Failed to start llama.cpp server");
        return false;
    }
    
    d->serverRunning = true;
    
    // Wait for server to be ready (simple delay, could use health check)
    QTimer::singleShot(3000, this, [this]() {
        m_initialized = true;
        emit modelLoaded(true);
        qDebug() << "Llama.cpp server initialized successfully";
    });
    
    return true;
}

void LlamaClient::shutdown() {
    if (d->serverProcess) {
        d->serverProcess->kill();
        d->serverProcess->waitForFinished(3000);
    }
    m_initialized = false;
    d->serverRunning = false;
    m_generating = false;
    m_messageHistory.clear();
}

void LlamaClient::registerTool(const ToolDefinition& tool) {
    m_tools.append(tool);
}

void LlamaClient::clearTools() {
    m_tools.clear();
}

void LlamaClient::sendRequest(const QString& userMessage, bool enableTools) {
    ChatMessage msg;
    msg.role = "user";
    msg.content = userMessage;
    
    QVector<ChatMessage> messages;
    messages.append(msg);
    
    sendRequest(messages, enableTools);
}

void LlamaClient::sendRequest(const QVector<ChatMessage>& messages, bool enableTools) {
    if (!m_initialized) {
        emit errorOccurred("LlamaClient not initialized");
        return;
    }
    
    if (m_generating) {
        qWarning() << "Already generating, ignoring request";
        return;
    }
    
    m_generating = true;
    m_currentResponse.clear();
    emit generationStarted();
    
    // Build request JSON for llama.cpp server
    QJsonObject requestBody;
    
    // Convert messages to JSON array
    QJsonArray messagesJson;
    for (const auto& msg : messages) {
        messagesJson.append(msg.toJson());
    }
    requestBody["messages"] = messagesJson;
    
    // Add system prompt if not present
    bool hasSystem = std::any_of(messages.begin(), messages.end(),
        [](const ChatMessage& m) { return m.role == "system"; });
    
    if (!hasSystem) {
        QJsonObject systemMsg;
        systemMsg["role"] = "system";
        systemMsg["content"] = "You are an intelligent AI assistant integrated into the Monolith IDE. "
                               "You can help with coding tasks, answer questions, and use tools to interact with the IDE.";
        messagesJson.prepend(systemMsg);
    }
    
    // Add tool definitions if enabled and tools are registered
    if (enableTools && !m_tools.isEmpty()) {
        QJsonArray toolsJson;
        for (const auto& tool : m_tools) {
            toolsJson.append(tool.toJson());
        }
        requestBody["tools"] = toolsJson;
        requestBody["tool_choice"] = "auto";
    }
    
    // Generation parameters
    requestBody["temperature"] = m_config.temperature;
    requestBody["top_p"] = m_config.topP / 100.0f;
    requestBody["max_tokens"] = m_config.maxTokens;
    requestBody["stream"] = true; // Enable streaming
    
    // Send HTTP POST request to llama.cpp server
    // Using curl via QProcess as Qt doesn't have built-in HTTP client in widgets
    QString curlPath = QStandardPaths::findExecutable("curl");
    if (curlPath.isEmpty()) {
        // Fallback: use Qt's network module if available
        // For now, we'll simulate the response parsing
        qCritical() << "curl not found, HTTP requests may fail";
    }
    
    // For production, you'd use QNetworkAccessManager or a proper HTTP client
    // Here we'll use a simplified approach assuming direct server communication
    
    d->pendingData.clear();
    d->timeoutTimer->start(d->requestTimeout);
    
    // In a real implementation, this would send an HTTP POST request
    // For this example, we'll use a mock approach
    qDebug() << "Sending request to llama.cpp server...";
    
    // Simulate sending to server (in production, use QNetworkAccessManager)
    // The actual implementation would POST to http://127.0.0.1:8080/completion
}

void LlamaClient::abortGeneration() {
    if (m_generating) {
        d->timeoutTimer->stop();
        m_generating = false;
        emit generationStopped();
    }
}

void LlamaClient::submitToolResult(const ToolResult& result) {
    if (!m_waitingForToolResult) {
        qWarning() << "Not waiting for tool result";
        return;
    }
    
    // Add tool result to message history
    ChatMessage toolMsg;
    toolMsg.role = "tool";
    toolMsg.content = result.content;
    toolMsg.toolCallId = result.toolCallId;
    
    m_messageHistory.append(toolMsg);
    m_waitingForToolResult = false;
    
    // Continue generation with tool result
    sendRequest(m_messageHistory, true);
}

void LlamaClient::onProcessReadyReadStandardOutput() {
    QByteArray data = d->serverProcess->readAllStandardOutput();
    parseServerResponse(data);
}

void LlamaClient::onProcessReadyReadStandardError() {
    QByteArray data = d->serverProcess->readAllStandardError();
    qWarning() << "Llama server stderr:" << QString::fromUtf8(data);
}

void LlamaClient::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    d->serverRunning = false;
    m_initialized = false;
    m_generating = false;
    
    if (exitStatus != QProcess::NormalExit) {
        emit errorOccurred("Llama.cpp server crashed");
    } else {
        qDebug() << "Llama.cpp server exited with code:" << exitCode;
    }
    
    emit generationStopped();
}

void LlamaClient::onTimeout() {
    abortGeneration();
    emit errorOccurred("Request timed out");
}

void LlamaClient::parseServerResponse(const QByteArray& data) {
    // Parse SSE (Server-Sent Events) stream from llama.cpp server
    // Format: data: {"content": "...", "stop": false}\n\n
    
    d->pendingData.append(data);
    
    while (true) {
        int newlinePos = d->pendingData.indexOf("\n");
        if (newlinePos == -1) {
            break; // Wait for more data
        }
        
        QByteArray line = d->pendingData.left(newlinePos).trimmed();
        d->pendingData = d->pendingData.mid(newlinePos + 1);
        
        if (line.startsWith("data: ")) {
            QByteArray jsonStr = line.mid(6); // Skip "data: "
            
            if (jsonStr == "[DONE]") {
                m_generating = false;
                m_waitingForToolResult = false;
                d->timeoutTimer->stop();
                emit responseComplete(m_currentResponse);
                emit generationStopped();
                continue;
            }
            
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(jsonStr, &parseError);
            
            if (parseError.error != QJsonParseError::NoError) {
                qWarning() << "JSON parse error:" << parseError.errorString();
                continue;
            }
            
            QJsonObject obj = doc.object();
            
            // Handle streaming content
            if (obj.contains("choices")) {
                QJsonArray choices = obj["choices"].toArray();
                if (!choices.isEmpty()) {
                    QJsonObject choice = choices[0].toObject();
                    
                    // Check for delta content
                    if (choice.contains("delta")) {
                        QJsonObject delta = choice["delta"].toObject();
                        
                        if (delta.contains("content")) {
                            QString content = delta["content"].toString();
                            m_currentResponse += content;
                            emit tokenReceived(content);
                        }
                        
                        // Check for tool calls
                        if (delta.contains("tool_calls")) {
                            QJsonArray toolCallsArr = delta["tool_calls"].toArray();
                            for (const auto& tc : toolCallsArr) {
                                ToolCall call = ToolCall::fromJson(tc.toObject());
                                m_pendingToolCallId = call.id;
                                emit toolCallRequested(call);
                                m_waitingForToolResult = true;
                            }
                        }
                    }
                    
                    // Check for finish reason
                    if (choice.contains("finish_reason") && !choice["finish_reason"].isNull()) {
                        QString reason = choice["finish_reason"].toString();
                        if (reason == "tool_calls") {
                            // AI wants to call a tool, wait for result
                            m_waitingForToolResult = true;
                        } else {
                            m_generating = false;
                            d->timeoutTimer->stop();
                            emit responseComplete(m_currentResponse);
                            emit generationStopped();
                        }
                    }
                }
            }
        }
    }
}

QJsonObject LlamaClient::buildToolDefinitions() const {
    QJsonArray toolsArr;
    for (const auto& tool : m_tools) {
        toolsArr.append(tool.toJson());
    }
    
    QJsonObject result;
    result["tools"] = toolsArr;
    return result;
}

void LlamaClient::processToolCall(const ToolCall& call) {
    qDebug() << "Processing tool call:" << call.name << call.arguments;
    // This would be handled by the AI panel which executes the actual tool
    // and calls submitToolResult() with the result
}

} // namespace monolith
