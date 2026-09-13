// CommandPalette.h - Fuzzy Search Command Palette
#ifndef MONOLITH_UI_COMMANDPALETTE_H
#define MONOLITH_UI_COMMANDPALETTE_H

#include <QDialog>
#include <QString>
#include <QStringList>
#include <QVector>
#include <functional>

class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QLabel;
class QVBoxLayout;
class QTimer;

namespace monolith {

/**
 * @brief Command item for the palette
 */
struct CommandItem {
    QString id;
    QString label;
    QString description;
    QString category;
    QString shortcut;
    int score;  // Fuzzy match score
    std::function<void()> callback;
    
    bool operator<(const CommandItem& other) const {
        return score > other.score;  // Higher score = better match
    }
};

/**
 * @brief File item for quick open
 */
struct FileItem {
    QString path;
    QString fileName;
    QString relativePath;
    int score;
    qint64 size;
    QDateTime lastModified;
    
    bool operator<(const FileItem& other) const {
        return score > other.score;
    }
};

/**
 * @brief Symbol item for go-to-symbol
 */
struct SymbolItem {
    QString name;
    QString containerName;
    int kind;  // SymbolKind
    QString filePath;
    int line;
    int column;
    int score;
    
    bool operator<(const SymbolItem& other) const {
        return score > other.score;
    }
};

/**
 * @brief Fuzzy matcher using Smith-Waterman algorithm
 */
class FuzzyMatcher
{
public:
    /**
     * @brief Calculate fuzzy match score
     * @param pattern The search pattern
     * @param text The text to match against
     * @return Score (higher is better, 0 means no match)
     */
    static int match(const QString& pattern, const QString& text);
    
    /**
     * @brief Get match positions for highlighting
     * @param pattern The search pattern
     * @param text The text to match against
     * @return List of matched character positions
     */
    static QVector<int> matchPositions(const QString& pattern, const QString& text);
    
    /**
     * @brief Check if pattern matches text
     */
    static bool matches(const QString& pattern, const QString& text);
    
private:
    static int smithWaterman(const QString& pattern, const QString& text);
    static int calculateBonus(char patternChar, char textChar, int posInText, int textLen);
};

/**
 * @brief Command palette widget with fuzzy search
 * 
 * Implements VS Code-style command palette (Ctrl+Shift+P):
 * - Fuzzy matching for commands, files, and symbols
 * - Quick open (Ctrl+P)
 * - Go to symbol (Ctrl+Shift+O)
 * - Action execution
 */
class CommandPalette : public QDialog
{
    Q_OBJECT

public:
    explicit CommandPalette(QWidget* parent = nullptr);
    ~CommandPalette() override;

    /**
     * @brief Register a command
     * @param id Unique command identifier
     * @param label Display label
     * @param callback Function to execute
     * @param category Optional category for grouping
     * @param shortcut Optional keyboard shortcut display
     */
    void registerCommand(const QString& id, const QString& label, 
                         std::function<void()> callback,
                         const QString& category = {},
                         const QString& shortcut = {});

    /**
     * @brief Unregister a command
     */
    void unregisterCommand(const QString& id);

    /**
     * @brief Add file items for quick open
     * @param files List of file paths
     */
    void setFiles(const QStringList& files);

    /**
     * @brief Add symbol items for go-to-symbol
     * @param symbols List of symbols
     */
    void setSymbols(const QVector<SymbolItem>& symbols);

    /**
     * @brief Show the palette in command mode
     */
    void showCommandMode();

    /**
     * @brief Show the palette in file quick-open mode
     */
    void showFileMode();

    /**
     * @brief Show the palette in symbol mode
     */
    void showSymbolMode();

    /**
     * @brief Set the workspace root for relative path calculation
     */
    void setWorkspaceRoot(const QString& path);

signals:
    void commandExecuted(const QString& commandId);
    void fileSelected(const QString& filePath);
    void symbolSelected(const QString& filePath, int line, int column);

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private slots:
    void onTextChanged(const QString& text);
    void onItemActivated(QListWidgetItem* item);
    void onItemClicked(QListWidgetItem* item);
    void updateFilter();
    void selectNext();
    void selectPrevious();
    void executeCurrent();

private:
    void setupUI();
    void updatePlaceholder();
    void filterCommands();
    void filterFiles();
    void filterSymbols();
    void updateList();
    QString formatMatchHighlight(const QString& text, const QVector<int>& positions);
    void loadRecentFiles();
    void saveRecentFile(const QString& filePath);

    enum class Mode {
        Command,
        File,
        Symbol
    };

    QLineEdit* m_searchEdit;
    QListWidget* m_resultList;
    QLabel* m_statusLabel;
    QVBoxLayout* m_layout;
    
    Mode m_mode;
    QString m_workspaceRoot;
    QString m_currentFilter;
    
    QVector<CommandItem> m_commands;
    QVector<CommandItem> m_filteredCommands;
    QVector<FileItem> m_files;
    QVector<FileItem> m_filteredFiles;
    QVector<SymbolItem> m_symbols;
    QVector<SymbolItem> m_filteredSymbols;
    
    QTimer* m_filterTimer;
    bool m_pendingUpdate;
    
    QStringList m_recentFiles;
    static const int MAX_RECENT_FILES = 20;
};

} // namespace monolith

#endif // MONOLITH_UI_COMMANDPALETTE_H
