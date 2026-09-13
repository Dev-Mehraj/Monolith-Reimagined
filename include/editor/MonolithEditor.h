// MonolithEditor.h - High-Performance Code Editor
#ifndef MONOLITH_EDITOR_MONOLITHEDITOR_H
#define MONOLITH_EDITOR_MONOLITHEDITOR_H

#include <QWidget>
#include <QString>
#include <QTextDocument>
#include <QMap>
#include <vector>
#include <memory>

// Forward declarations
class QScintilla;
class QGraphicsView;
class QLabel;
class QCompleter;
class QMenu;

#ifdef USE_QSCINTILLA
class QsciScintilla;
class QsciLexer;
#endif

namespace monolith {

class LspClient;
class MinimapWidget;
class BreadcrumbBar;

/**
 * @brief Cursor position and selection info for multi-cursor support
 */
struct CursorState {
    int line;
    int column;
    int anchorLine;
    int anchorColumn;
    bool hasSelection;
};

/**
 * @brief Code folding range
 */
struct FoldRange {
    int startLine;
    int endLine;
    bool isFolded;
};

/**
 * @brief Diagnostic item from LSP
 */
struct Diagnostic {
    enum Severity {
        Error = 0,
        Warning = 1,
        Information = 2,
        Hint = 3
    };
    
    int startLine;
    int startColumn;
    int endLine;
    int endColumn;
    Severity severity;
    QString message;
    QString source;
    QString code;
};

/**
 * @brief Main code editor widget with LSP integration
 * 
 * Wraps QScintilla with additional VS Code features:
 * - Minimap preview
 * - Breadcrumb navigation
 * - Multi-cursor editing
 * - LSP autocomplete and diagnostics
 * - Git blame annotations
 * - Code folding
 */
class MonolithEditor : public QWidget
{
    Q_OBJECT

public:
    explicit MonolithEditor(QWidget* parent = nullptr);
    ~MonolithEditor() override;

    /**
     * @brief Load a file into the editor
     */
    bool loadFile(const QString& filePath);

    /**
     * @brief Save the current file
     */
    bool save();

    /**
     * @brief Save to a specific path
     */
    bool saveAs(const QString& filePath);

    /**
     * @brief Get the current file path
     */
    QString filePath() const { return m_filePath; }

    /**
     * @brief Check if document has unsaved changes
     */
    bool isModified() const;

    /**
     * @brief Set the language mode for syntax highlighting
     */
    void setLanguage(const QString& languageId);

    /**
     * @brief Get the current cursor position
     */
    CursorState cursorState() const;

    /**
     * @brief Add an additional cursor (multi-cursor editing)
     */
    void addCursor(int line, int column);

    /**
     * @brief Remove all additional cursors
     */
    void clearAdditionalCursors();

    /**
     * @brief Toggle fold at the given line
     */
    void toggleFold(int line);

    /**
     * @brief Fold all regions of a specific type
     */
    void foldAll();

    /**
     * @brief Unfold all regions
     */
    void unfoldAll();

    /**
     * @brief Go to definition at cursor
     */
    void goToDefinition();

    /**
     * @brief Show hover tooltip at cursor
     */
    void showHover();

    /**
     * @brief Trigger autocomplete manually
     */
    void triggerAutocomplete();

    /**
     * @brief Format the entire document
     */
    void formatDocument();

    /**
     * @brief Format the current selection
     */
    void formatSelection();

    /**
     * @brief Set LSP client for language features
     */
    void setLspClient(LspClient* client);

    /**
     * @brief Update diagnostics from LSP
     */
    void updateDiagnostics(const QList<Diagnostic>& diagnostics);

    /**
     * @brief Set git blame data for current file
     */
    void setGitBlame(const QMap<int, QString>& blameData);

    /**
     * @brief Get the minimap widget
     */
    MinimapWidget* minimap() const { return m_minimap; }

    /**
     * @brief Get the breadcrumb bar
     */
    BreadcrumbBar* breadcrumbBar() const { return m_breadcrumbBar; }

signals:
    void fileSaved(const QString& filePath);
    void fileLoaded(const QString& filePath);
    void modificationChanged(bool changed);
    void cursorPositionChanged(int line, int column);
    void selectionChanged();
    void languageChanged(const QString& languageId);
    void requestGoToDefinition(const QString& uri, int line, int column);
    void requestHover(const QString& uri, int line, int column);
    void requestCompletion(const QString& uri, int line, int column);
    void requestFormat(const QString& uri, const QString& rangeType);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private slots:
    void onTextChanged();
    void onCursorPositionChanged(int line, int index);
    void onCompletionSelected(const QString& text);
    void applyCompletion(const QVariant& completionItem);
    void showCompletionPopup(const QPoint& pos, const QStringList& completions);
    void hideCompletionPopup();
    void updateMinimap();
    void updateBreadcrumbs();

private:
    void setupEditor();
    void setupMinimap();
    void setupBreadcrumb();
    void setupConnections();
    void setupSyntaxHighlighter();
    void updateTitle();
    QString detectLanguageFromExtension(const QString& extension);
    void applyTheme();

#ifdef USE_QSCINTILLA
    QsciScintilla* m_editor;
    QsciLexer* m_lexer;
#else
    QWidget* m_editor;  // Placeholder when QScintilla not available
#endif
    
    MinimapWidget* m_minimap;
    BreadcrumbBar* m_breadcrumbBar;
    QLabel* m_hoverLabel;
    QMenu* m_completionMenu;
    
    LspClient* m_lspClient;
    
    QString m_filePath;
    QString m_languageId;
    bool m_dirty;
    
    QVector<CursorState> m_cursors;
    QMap<int, FoldRange> m_foldRanges;
    QList<Diagnostic> m_diagnostics;
    QMap<int, QString> m_gitBlame;
    
    // Completion state
    int m_completionStartLine;
    int m_completionStartColumn;
    int m_completionEndColumn;
};

} // namespace monolith

#endif // MONOLITH_EDITOR_MONOLITHEDITOR_H
