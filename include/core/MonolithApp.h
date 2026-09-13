// MonolithApp.h - Core Application Singleton
#ifndef MONOLITH_CORE_MONOLITHAPP_H
#define MONOLITH_CORE_MONOLITHAPP_H

#include <QApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QObject>
#include <memory>
#include "utils/Singleton.h"

class QSettings;
class ThemeEngine;

namespace monolith {

/**
 * @brief Global application singleton managing event filtering,
 * keybinding dispatch, and signal routing.
 */
class MonolithApp : public QApplication, public Singleton<MonolithApp>
{
    Q_OBJECT
    friend class Singleton<MonolithApp>;

public:
    explicit MonolithApp(int& argc, char** argv);
    ~MonolithApp() override;

    // Prevent copying
    MonolithApp(const MonolithApp&) = delete;
    MonolithApp& operator=(const MonolithApp&) = delete;

    /**
     * @brief Get the global settings instance
     */
    QSettings* settings() const { return m_settings; }

    /**
     * @brief Get the theme engine
     */
    ThemeEngine* themeEngine() const { return m_themeEngine; }

    /**
     * @brief Register a global keybinding
     * @param sequence Key sequence (e.g., "Ctrl+Shift+P")
     * @param action Slot to invoke
     */
    void registerKeyBinding(const QKeySequence& sequence, std::function<void()> action);

    /**
     * @brief Unregister a keybinding
     */
    void unregisterKeyBinding(const QKeySequence& sequence);

    /**
     * @brief Trigger an action by ID (for command palette integration)
     */
    void triggerAction(const QString& actionId);

signals:
    void actionTriggered(const QString& actionId);
    void themeChanged();
    void workspaceOpened(const QString& path);
    void workspaceClosed();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    bool notify(QObject* receiver, QEvent* event) override;

private:
    bool handleGlobalShortcut(QKeyEvent* keyEvent);
    void setupDefaultKeyBindings();
    void loadSettings();
    void saveSettings();

    QSettings* m_settings;
    ThemeEngine* m_themeEngine;
    
    struct KeyBinding {
        QKeySequence sequence;
        std::function<void()> action;
    };
    
    QMap<QKeySequence, KeyBinding> m_keyBindings;
    QMap<QString, std::function<void()>> m_actions;
};

} // namespace monolith

#endif // MONOLITH_CORE_MONOLITHAPP_H
