#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QVector>
#include <functional>
#include <memory>
#include <optional>

namespace monolith {

// Tool definition for AI agent
struct ToolDefinition {
    QString name;
    QString description;
    QJsonObject parameters; // JSON Schema
    
    QJsonObject toJson() const {
        QJsonObject obj;
        obj["name"] = name;
        obj["description"] = description;
        obj["parameters"] = parameters;
        return obj;
    }
};

// Tool call request from AI
struct ToolCall {
    QString id;
    QString name;
    QJsonObject arguments;
    
    static ToolCall fromJson(const QJsonObject& json) {
        ToolCall call;
        call.id = json["id"].toString();
        call.name = json["name"].toString();
        call.arguments = json["arguments"].isObject() 
            ? json["arguments"].toObject() 
            : QJsonDocument::fromJson(json["arguments"].toString().toUtf8()).object();
        return call;
    }
    
    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["name"] = name;
        obj["arguments"] = arguments;
        return obj;
    }
};

// Tool execution result
struct ToolResult {
    QString toolCallId;
    QString content;
    bool isError = false;
    
    QJsonObject toJson() const {
        QJsonObject obj;
        obj["role"] = "tool";
        obj["content"] = content;
        obj["tool_call_id"] = toolCallId;
        if (isError) {
            obj["is_error"] = true;
        }
        return obj;
    }
};

// Chat message structure
struct ChatMessage {
    QString role; // "system", "user", "assistant", "tool"
    QString content;
    std::optional<QJsonArray> toolCalls;
    std::optional<QString> toolCallId;
    
    QJsonObject toJson() const {
        QJsonObject obj;
        obj["role"] = role;
        obj["content"] = content;
        
        if (toolCalls.has_value()) {
            obj["tool_calls"] = toolCalls.value();
        }
        if (toolCallId.has_value()) {
            obj["tool_call_id"] = toolCallId.value();
        }
        
        return obj;
    }
    
    static ChatMessage fromJson(const QJsonObject& json) {
        ChatMessage msg;
        msg.role = json["role"].toString();
        msg.content = json["content"].toString();
        
        if (json.contains("tool_calls") && !json["tool_calls"].isNull()) {
            msg.toolCalls = json["tool_calls"].toArray();
        }
        if (json.contains("tool_call_id") && !json["tool_call_id"].isNull()) {
            msg.toolCallId = json["tool_call_id"].toString();
        }
        
        return msg;
    }
};

// Streaming response chunk
struct ResponseChunk {
    QString content;
    std::optional<ToolCall> toolCall;
    bool isDone = false;
    QString finishReason;
};

// Llama.cpp inference configuration
struct LlamaConfig {
    QString modelPath;
    int nContext = 4096;
    int nBatch = 512;
    int nThreads = 4;
    int nGpuLayers = 0; // 0 = CPU only
    float temperature = 0.7f;
    int topP = 90;
    int topK = 40;
    int maxTokens = 2048;
    bool useMlock = false;
    bool useMmap = true;
    
    QStringList toArgs() const {
        QStringList args;
        args << "-m" << modelPath;
        args << "-c" << QString::number(nContext);
        args << "-b" << QString::number(nBatch);
        args << "-t" << QString::number(nThreads);
        args << "--temp" << QString::number(temperature);
        args << "--top-p" << QString::number(topP / 100.0f, 'f', 2);
        args << "--top-k" << QString::number(topK);
        args << "-n" << QString::number(maxTokens);
        
        if (nGpuLayers > 0) {
            args << "-ngl" << QString::number(nGpuLayers);
        }
        if (useMlock) {
            args << "--mlock";
        }
        if (!useMmap) {
            args << "--no-mmap";
        }
        
        return args;
    }
};

// Main Llama.cpp client class
class LlamaClient : public QObject {
    Q_OBJECT

public:
    explicit LlamaClient(QObject* parent = nullptr);
    ~LlamaClient();

    // Initialize the Llama.cpp backend
    bool initialize(const LlamaConfig& config);
    void shutdown();
    bool isInitialized() const { return m_initialized; }
    
    // Register tools for agent mode
    void registerTool(const ToolDefinition& tool);
    void clearTools();
    QVector<ToolDefinition> registeredTools() const { return m_tools; }
    
    // Send a chat message and get streaming response
    void sendRequest(const QString& userMessage, bool enableTools = true);
    void sendRequest(const QVector<ChatMessage>& messages, bool enableTools = true);
    
    // Abort current generation
    void abortGeneration();
    
    // Check if currently generating
    bool isGenerating() const { return m_generating; }

signals:
    // Streaming tokens
    void tokenReceived(const QString& token);
    
    // Complete response
    void responseComplete(const QString& fullResponse);
    
    // Tool call requested by AI
    void toolCallRequested(const ToolCall& toolCall);
    
    // Error occurred
    void errorOccurred(const QString& errorMessage);
    
    // Generation started/stopped
    void generationStarted();
    void generationStopped();
    
    // Model loaded
    void modelLoaded(bool success);

public slots:
    // Submit tool result back to AI
    void submitToolResult(const ToolResult& result);

private slots:
    void onProcessReadyReadStandardOutput();
    void onProcessReadyReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onTimeout();

private:
    struct Private;
    std::unique_ptr<Private> d;
    
    void parseServerResponse(const QByteArray& data);
    void buildPrompt(const QVector<ChatMessage>& messages);
    QJsonObject buildToolDefinitions() const;
    void processToolCall(const ToolCall& call);
    
    LlamaConfig m_config;
    QVector<ToolDefinition> m_tools;
    QVector<ChatMessage> m_messageHistory;
    QString m_currentResponse;
    bool m_initialized = false;
    bool m_generating = false;
    bool m_waitingForToolResult = false;
    QString m_pendingToolCallId;
};

} // namespace monolith
