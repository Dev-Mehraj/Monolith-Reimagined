// MonolithApp.cpp - Core Application Implementation
#include "core/MonolithApp.h"
#include "theme/ThemeEngine.h"
#include <QSettings>
#include <QKeyEvent>
#include <QDir>
#include <QDebug>

namespace monolith {

MonolithApp::MonolithApp(int& argc, char** argv)
    : QApplication(argc, argv)
    , m_settings(nullptr)
    , m_themeEngine(nullptr)
{
    // Initialize settings
    m_settings = new QSettings(
        QSettings::IniFormat,
        QSettings::UserScope,
        organizationName(),
        applicationName(),
        this
    );
    
    // Initialize theme engine
    m_themeEngine = new ThemeEngine(this);
    
    // Install global event filter
    installEventFilter(this);
    
    // Setup default keybindings
    setupDefaultKeyBindings();
    
    // Load saved settings
    loadSettings();
}

MonolithApp::~MonolithApp()
{
    saveSettings();
    delete m_settings;
    // m_themeEngine is parented to this, will be deleted automatically
}

void MonolithApp::registerKeyBinding(const QKeySequence& sequence, std::function<void()> action)
{
    if (sequence.isEmpty()) {
        qWarning() << "Cannot register empty key sequence";
        return;
    }
    
    KeyBinding binding;
    binding.sequence = sequence;
    binding.action = std::move(action);
    
    m_keyBindings[sequence] = binding;
    qDebug() << "Registered keybinding:" << sequence.toString(QKeySequence::NativeText);
}

void MonolithApp::unregisterKeyBinding(const QKeySequence& sequence)
{
    if (m_keyBindings.remove(sequence)) {
        qDebug() << "Unregistered keybinding:" << sequence.toString(QKeySequence::NativeText);
    }
}

void MonolithApp::triggerAction(const QString& actionId)
{
    auto it = m_actions.find(actionId);
    if (it != m_actions.end()) {
        it.value()();
        emit actionTriggered(actionId);
    } else {
        qWarning() << "Action not found:" << actionId;
    }
}

bool MonolithApp::eventFilter(QObject* watched, QEvent* event)
{
    // Handle global keyboard shortcuts
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (handleGlobalShortcut(keyEvent)) {
            return true;  // Event handled, stop propagation
        }
    }
    
    return QApplication::eventFilter(watched, event);
}

bool MonolithApp::notify(QObject* receiver, QEvent* event)
{
    try {
        return QApplication::notify(receiver, event);
    } catch (const std::exception& e) {
        qCritical() << "Exception in event handler:" << e.what();
        return false;
    } catch (...) {
        qCritical() << "Unknown exception in event handler";
        return false;
    }
}

bool MonolithApp::handleGlobalShortcut(QKeyEvent* keyEvent)
{
    // Build key sequence from current modifiers + key
    QKeySequence sequence(
        keyEvent->modifiers() | Qt::KeyboardModifier::NoModifier,
        keyEvent->key()
    );
    
    // Try exact match first
    auto it = m_keyBindings.find(sequence);
    if (it != m_keyBindings.end()) {
        it.value().action();
        return true;
    }
    
    // Try with portable text matching
    QString seqStr = sequence.toString(QKeySequence::PortableText);
    for (auto bindIt = m_keyBindings.begin(); bindIt != m_keyBindings.end(); ++bindIt) {
        if (bindIt.key().toString(QKeySequence::PortableText) == seqStr) {
            bindIt.value().action();
            return true;
        }
    }
    
    return false;
}

void MonolithApp::setupDefaultKeyBindings()
{
    // These would typically be set by MainWindow after construction
    // Placeholder for common VS Code keybindings
    
    // Ctrl+Shift+P - Command Palette
    // Ctrl+P - Quick Open
    // Ctrl+N - New File
    // Ctrl+O - Open File
    // Ctrl+S - Save
    // Ctrl+Shift+S - Save As
    // Ctrl+W - Close Editor
    // Ctrl+` - Toggle Terminal
    // Ctrl+B - Toggle Sidebar
    // Ctrl+M - Toggle Minimap
    // F5 - Start Debugging
    // Shift+F5 - Stop Debugging
    // Ctrl+F5 - Run Without Debugging
    // F9 - Toggle Breakpoint
    // Ctrl+F - Find
    // Ctrl+H - Replace
    // F3 - Find Next
    // Shift+F3 - Find Previous
    // Alt+Left - Go Back
    // Alt+Right - Go Forward
    // Ctrl+G - Go to Line
    // Ctrl+T - Go to Symbol
    // Shift+Alt+F - Format Document
    // Ctrl+K Ctrl+F - Format Selection
    // F12 - Go to Definition
    // Alt+F12 - Peek Definition
    // Shift+F12 - Open Definition to Side
    // Ctrl+D - Select Next Occurrence
    // Ctrl+K Ctrl+D - Move Last Selection to Next Occurrence
    // Ctrl+/ - Toggle Line Comment
    // Shift+Alt+A - Toggle Block Comment
    // Ctrl+Z - Undo
    // Ctrl+Y - Redo
    // Ctrl+X - Cut
    // Ctrl+C - Copy
    // Ctrl+V - Paste
    // Ctrl+A - Select All
    // Ctrl+L - Select Current Line
    // Ctrl+Enter - Insert Line Below
    // Ctrl+Shift+Enter - Insert Line Above
    // Alt+Up/Down - Move Line Up/Down
    // Shift+Alt+Up/Down - Copy Line Up/Down
    // Ctrl+Delete - Delete Word Right
    // Ctrl+Backspace - Delete Word Left
    // Home/End - Go to Beginning/End of Line
    // Ctrl+Home/End - Go to Beginning/End of File
    // Page Up/Down - Scroll Up/Down
    // Ctrl+Page Up/Down - Switch Editor Tab
    // Ctrl+K M - Change Language Mode
    // Ctrl+Space - Trigger Suggest
    // Ctrl+I - Show Hover
    // Ctrl+Q - Show Documentation
}

void MonolithApp::loadSettings()
{
    m_settings->beginGroup("General");
    
    // Theme settings
    QString theme = m_settings->value("theme", "dark_vs").toString();
    if (m_themeEngine) {
        m_themeEngine->setTheme(theme);
    }
    
    // Window geometry
    // This would be restored by MainWindow
    
    m_settings->endGroup();
    
    m_settings->beginGroup("Editor");
    
    // Editor settings
    int fontSize = m_settings->value("fontSize", 14).toInt();
    QString fontFamily = m_settings->value("fontFamily", "Consolas").toString();
    bool minimapEnabled = m_settings->value("minimap/enabled", true).toBool();
    bool lineNumbers = m_settings->value("lineNumbers", true).toBool();
    bool wordWrap = m_settings->value("wordWrap", false).toBool();
    int tabSize = m_settings->value("tabSize", 4).toInt();
    bool insertSpaces = m_settings->value("insertSpaces", true).toBool();
    
    m_settings->endGroup();
    
    m_settings->beginGroup("Workbench");
    
    // Workbench settings
    bool sidebarVisible = m_settings->value("sidebarVisible", true).toBool();
    bool statusBarVisible = m_settings->value("statusBarVisible", true).toBool();
    bool activityBarVisible = m_settings->value("activityBarVisible", true).toBool();
    QString layout = m_settings->value("layout", "default").toString();
    
    m_settings->endGroup();
    
    qDebug() << "Loaded settings successfully";
}

void MonolithApp::saveSettings()
{
    m_settings->beginGroup("General");
    
    // Theme settings
    if (m_themeEngine) {
        m_settings->setValue("theme", m_themeEngine->currentTheme());
    }
    
    m_settings->endGroup();
    
    // Editor and workbench settings would be saved by respective components
    
    m_settings->sync();
    qDebug() << "Saved settings successfully";
}

} // namespace monolith
