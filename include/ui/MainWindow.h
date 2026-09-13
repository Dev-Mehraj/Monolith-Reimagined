// MainWindow.h - Main IDE Window with Docking System
#ifndef MONOLITH_UI_MAINWINDOW_H
#define MONOLITH_UI_MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QString>
#include <memory>

// Forward declarations
namespace ads {
    class CDockManager;
    class CDockWidget;
}

class QStackedWidget;
class QSplitter;
class QAction;
class QMenu;
class QToolBar;
class QLabel;

namespace monolith {

class MonolithEditor;
class CommandPalette;
class ActivityBar;
class SideBar;
class StatusBar;
class BreadcrumbBar;
class WorkspaceExplorer;
class MonolithTerminal;
class GitManager;
class LspClient;

/**
 * @brief Main application window implementing VS Code-style layout
 * 
 * Uses Qt Advanced Docking System for flexible panel arrangement.
 * Manages Activity Bar, SideBar, Editor Area, Bottom Panel, and Status Bar.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    /**
     * @brief Open a file in the editor
     */
    void openFile(const QString& filePath);

    /**
     * @brief Open a folder as workspace
     */
    void openFolder(const QString& folderPath);

    /**
     * @brief Get the currently active editor
     */
    MonolithEditor* activeEditor() const { return m_activeEditor; }

    /**
     * @brief Show the command palette
     */
    void showCommandPalette();

    /**
     * @brief Toggle sidebar visibility
     */
    void toggleSideBar();

    /**
     * @brief Toggle terminal panel
     */
    void toggleTerminal();

    /**
     * @brief Register an action with the command palette
     */
    void registerAction(const QString& id, const QString& label, std::function<void()> callback);

    /**
     * @brief Get the Git manager instance
     */
    GitManager* gitManager() const { return m_gitManager; }

    /**
     * @brief Get the LSP client instance
     */
    LspClient* lspClient() const { return m_lspClient; }

signals:
    void fileOpened(const QString& filePath);
    void workspaceChanged(const QString& path);
    void activeEditorChanged(MonolithEditor* editor);
    void statusBarMessage(const QString& message, int timeout = 0);

protected:
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onEditorTabChanged(int index);
    void onGitStatusChanged();
    void onLspDiagnosticsReceived(const QString& uri, const QList<QVariant>& diagnostics);

private:
    void setupDockManager();
    void setupActivityBar();
    void setupSideBar();
    void setupEditorArea();
    void setupBottomPanel();
    void setupStatusBar();
    void setupMenuBar();
    void setupToolBar();
    void createConnections();
    void loadLayout();
    void saveLayout();

    // Core components
    ads::CDockManager* m_dockManager;
    MonolithEditor* m_activeEditor;
    CommandPalette* m_commandPalette;
    
    // UI Panels
    ActivityBar* m_activityBar;
    SideBar* m_sideBar;
    StatusBar* m_statusBar;
    BreadcrumbBar* m_breadcrumbBar;
    QWidget* m_editorContainer;
    QWidget* m_bottomPanel;
    
    // Managers
    GitManager* m_gitManager;
    LspClient* m_lspClient;
    
    // Dock widgets
    ads::CDockWidget* m_explorerDock;
    ads::CDockWidget* m_searchDock;
    ads::CDockWidget* m_scmDock;
    ads::CDockWidget* m_debugDock;
    ads::CDockWidget* m_extensionsDock;
    ads::CDockWidget* m_terminalDock;
    ads::CDockWidget* m_outputDock;
    ads::CDockWidget* m_problemsDock;
    ads::CDockWidget* m_debugConsoleDock;
    
    // Actions
    QMap<QString, QAction*> m_actions;
    
    // State
    QString m_currentWorkspace;
    bool m_sideBarVisible;
};

} // namespace monolith

#endif // MONOLITH_UI_MAINWINDOW_H
