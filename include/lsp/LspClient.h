// LspClient.h - Language Server Protocol Client
#ifndef MONOLITH_LSP_LSPCLIENT_H
#define MONOLITH_LSP_LSPCLIENT_H

#include <QObject>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QString>
#include <QUrl>
#include <memory>
#include <functional>

namespace monolith {

/**
 * @brief LSP Message types
 */
enum class MessageType {
    Error = 1,
    Warning = 2,
    Info = 3,
    Log = 4
};

/**
 * @brief Diagnostic severity levels
 */
enum class DiagnosticSeverity {
    Error = 1,
    Warning = 2,
    Information = 3,
    Hint = 4
};

/**
 * @brief Completion item kinds
 */
enum class CompletionItemKind {
    Text = 1,
    Method = 2,
    Function = 3,
    Constructor = 4,
    Field = 5,
    Variable = 6,
    Class = 7,
    Interface = 8,
    Module = 9,
    Property = 10,
    Unit = 11,
    Value = 12,
    Enum = 13,
    Keyword = 14,
    Snippet = 15,
    Color = 16,
    File = 17,
    Reference = 18,
    Folder = 19,
    EnumMember = 20,
    Constant = 21,
    Struct = 22,
    Event = 23,
    Operator = 24,
    TypeParameter = 25
};

/**
 * @brief Position in a text document
 */
struct Position {
    int line;
    int character;
    
    bool operator==(const Position& other) const {
        return line == other.line && character == other.character;
    }
};

/**
 * @brief Range in a text document
 */
struct Range {
    Position start;
    Position end;
};

/**
 * @brief Location reference
 */
struct Location {
    QString uri;
    Range range;
};

/**
 * @brief Diagnostic item
 */
struct LspDiagnostic {
    Range range;
    DiagnosticSeverity severity;
    QString message;
    QString source;
    QString code;
    QString codeDescription;
};

/**
 * @brief Completion item
 */
struct CompletionItem {
    QString label;
    CompletionItemKind kind;
    QString detail;
    QString documentation;
    int sortText;
    QString filterText;
    QString insertText;
    QString textEdit;
    QList<QVariant> additionalTextEdits;
};

/**
 * @brief Hover result
 */
struct Hover {
    QString contents;  // Markdown or plaintext
    Range range;
};

/**
 * @brief Symbol information
 */
struct SymbolInformation {
    QString name;
    int kind;
    Location location;
    QString containerName;
};

/**
 * @brief Async JSON-RPC client for LSP communication
 * 
 * Handles the complete LSP protocol including:
 * - Initialization and capability negotiation
 * - Document synchronization (open, change, save, close)
 * - Diagnostics (publishDiagnostics)
 * - Completions (textDocument/completion)
 * - Hover (textDocument/hover)
 * - Definition (textDocument/definition)
 * - References (textDocument/references)
 * - Rename (textDocument/rename)
 * - Formatting (textDocument/formatting)
 * - Workspace symbols
 */
class LspClient : public QObject
{
    Q_OBJECT

public:
    explicit LspClient(QObject* parent = nullptr);
    ~LspClient() override;

    /**
     * @brief Start the language server process
     * @param command Server executable path
     * @param args Command line arguments
     */
    bool startServer(const QString& command, const QStringList& args = {});

    /**
     * @brief Stop the language server
     */
    void stopServer();

    /**
     * @brief Check if server is running
     */
    bool isRunning() const;

    /**
     * @brief Initialize the LSP session
     * @param rootPath Workspace root path
     * @param capabilities Client capabilities
     */
    void initialize(const QString& rootPath, const QJsonObject& capabilities = {});

    /**
     * @brief Send initialized notification
     */
    void sendInitialized();

    /**
     * @brief Open a text document
     */
    void openDocument(const QString& uri, const QString& text, const QString& languageId);

    /**
     * @brief Close a text document
     */
    void closeDocument(const QString& uri);

    /**
     * @brief Notify document changes (incremental or full)
     */
    void changeDocument(const QString& uri, const QString& text, int version);

    /**
     * @brief Save a document
     */
    void saveDocument(const QString& uri, const QString& text = {});

    /**
     * @brief Request completions at position
     */
    void requestCompletion(const QString& uri, int line, int column);

    /**
     * @brief Request hover information
     */
    void requestHover(const QString& uri, int line, int column);

    /**
     * @brief Request definition at position
     */
    void requestDefinition(const QString& uri, int line, int column);

    /**
     * @brief Request type definition at position
     */
    void requestTypeDefinition(const QString& uri, int line, int column);

    /**
     * @brief Request references at position
     */
    void requestReferences(const QString& uri, int line, int column, bool includeDeclaration);

    /**
     * @brief Request document highlights
     */
    void requestDocumentHighlight(const QString& uri, int line, int column);

    /**
     * @brief Request document formatting
     */
    void requestFormatting(const QString& uri, const QVariantMap& options);

    /**
     * @brief Request range formatting
     */
    void requestRangeFormatting(const QString& uri, const Range& range, const QVariantMap& options);

    /**
     * @brief Request rename symbol
     */
    void requestRename(const QString& uri, int line, int column, const QString& newName);

    /**
     * @brief Request workspace symbols
     */
    void requestWorkspaceSymbols(const QString& query);

    /**
     * @brief Execute a command
     */
    void executeCommand(const QString& command, const QVariantList& arguments = {});

    /**
     * @brief Send shutdown request
     */
    void shutdown();

signals:
    void serverStarted();
    void serverStopped();
    void serverError(const QString& error);
    void initialized();
    
    // Diagnostics
    void diagnosticsPublished(const QString& uri, const QList<LspDiagnostic>& diagnostics);
    
    // Completions
    void completionReceived(const QString& requestId, const QList<CompletionItem>& items);
    
    // Hover
    void hoverReceived(const QString& requestId, const Hover& hover);
    
    // Definition
    void definitionReceived(const QString& requestId, const QList<Location>& locations);
    
    // References
    void referencesReceived(const QString& requestId, const QList<Location>& locations);
    
    // Formatting
    void formattingReceived(const QString& requestId, const QList<QVariant>& edits);
    
    // Rename
    void renameReceived(const QString& requestId, const QVariant& workspaceEdit);
    
    // Workspace symbols
    void workspaceSymbolsReceived(const QString& requestId, const QList<SymbolInformation>& symbols);
    
    // Messages from server
    void showMessage(MessageType type, const QString& message);
    void logMessage(MessageType type, const QString& message);

private slots:
    void onProcessStarted();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();

private:
    void sendMessage(const QJsonObject& message);
    void handleIncomingData(const QByteArray& data);
    void parseAndDispatchMessage(const QString& jsonString);
    void handleResponse(const QJsonObject& response);
    void handleNotification(const QString& method, const QJsonObject& params);
    void handleRequest(const QJsonObject& request);
    
    QJsonObject createMessage(const QString& method, const QVariant& params = {});
    QJsonObject createResponse(int id, const QVariant& result = {});
    QJsonObject createErrorResponse(int id, int errorCode, const QString& errorMessage);
    
    QString positionToJson(const Position& pos);
    Position jsonToPosition(const QJsonValue& value);
    QString rangeToJson(const Range& range);
    Range jsonToRange(const QJsonValue& value);

    QProcess* m_process;
    QString m_serverCommand;
    QStringList m_serverArgs;
    
    QByteArray m_buffer;
    int m_messageId;
    bool m_initialized;
    bool m_initializing;
    
    QMap<int, std::function<void(const QJsonValue&)>> m_pendingRequests;
    QMap<QString, int> m_documentVersions;
    
    QJsonObject m_serverCapabilities;
};

} // namespace monolith

#endif // MONOLITH_LSP_LSPCLIENT_H
