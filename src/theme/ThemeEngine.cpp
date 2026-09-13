// ThemeEngine.cpp - Theme Management Implementation
#include "theme/ThemeEngine.h"
#include <QFile>
#include <QDir>
#include <QApplication>
#include <QStandardPaths>
#include <QDebug>

namespace monolith {

const QString ThemeEngine::DEFAULT_DARK_THEME = "dark_vs";
const QString ThemeEngine::DEFAULT_LIGHT_THEME = "light_vs";

ThemeEngine::ThemeEngine(QObject* parent)
    : QObject(parent)
    , m_currentTheme(DEFAULT_DARK_THEME)
    , m_isDarkTheme(true)
{
    setupDefaultDarkTheme();
    loadStylesheet(m_currentTheme);
}

ThemeEngine::~ThemeEngine()
{
}

bool ThemeEngine::loadTheme(const QString& themeName)
{
    QString stylesheetContent = loadStylesheet(themeName);
    
    if (stylesheetContent.isEmpty()) {
        qWarning() << "Failed to load theme:" << themeName;
        return false;
    }
    
    m_currentTheme = themeName;
    m_currentStylesheet = stylesheetContent;
    m_isDarkTheme = (themeName.contains("dark", Qt::CaseInsensitive));
    
    // Apply to application
    QApplication::setStyleSheet(m_currentStylesheet);
    
    emit themeChanged(themeName);
    qDebug() << "Theme loaded:" << themeName;
    
    return true;
}

void ThemeEngine::setTheme(const QString& themeName)
{
    if (m_currentTheme != themeName) {
        loadTheme(themeName);
    }
}

QStringList ThemeEngine::availableThemes() const
{
    QStringList themes;
    
    // Check resource paths
    QDir resDir(":/themes");
    for (const QString& file : resDir.entryList({"*.qss"}, QDir::Files)) {
        themes << QFileInfo(file).baseName();
    }
    
    // Check filesystem paths
    QStringList searchPaths;
    searchPaths << QCoreApplication::applicationDirPath() + "/themes"
                << QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/themes"
                << QDir::homePath() + "/.monolith/themes";
    
    for (const QString& path : searchPaths) {
        QDir dir(path);
        if (dir.exists()) {
            for (const QString& file : dir.entryList({"*.qss"}, QDir::Files)) {
                QString baseName = QFileInfo(file).baseName();
                if (!themes.contains(baseName)) {
                    themes << baseName;
                }
            }
        }
    }
    
    // Add defaults if not found
    if (!themes.contains(DEFAULT_DARK_THEME)) {
        themes.prepend(DEFAULT_DARK_THEME);
    }
    
    return themes;
}

QColor ThemeEngine::getColor(const QString& tokenId) const
{
    auto it = m_colors.find(tokenId);
    if (it != m_colors.end()) {
        return it.value().color;
    }
    
    // Return default colors for common tokens
    if (tokenId == "editor.background") {
        return m_isDarkTheme ? QColor("#1e1e1e") : QColor("#ffffff");
    }
    if (tokenId == "editor.foreground") {
        return m_isDarkTheme ? QColor("#d4d4d4") : QColor("#000000");
    }
    if (tokenId == "editor.lineHighlightBackground") {
        return m_isDarkTheme ? QColor("#282828") : QColor("#f0f0f0");
    }
    
    return QColor();
}

QColor ThemeEngine::getColorWithAlpha(const QString& tokenId, int alpha) const
{
    QColor color = getColor(tokenId);
    if (color.isValid()) {
        color.setAlpha(alpha);
    }
    return color;
}

QPalette ThemeEngine::generatePalette() const
{
    QPalette palette;
    
    QColor bg = getColor("editor.background");
    QColor fg = getColor("editor.foreground");
    QColor highlight = getColor("list.activeSelectionBackground");
    QColor base = getColor("editorWidget.background");
    
    if (!bg.isValid()) bg = m_isDarkTheme ? QColor("#1e1e1e") : QColor("#ffffff");
    if (!fg.isValid()) fg = m_isDarkTheme ? QColor("#d4d4d4") : QColor("#000000");
    if (!highlight.isValid()) highlight = m_isDarkTheme ? QColor("#37373d") : QColor("#007fd4");
    if (!base.isValid()) base = m_isDarkTheme ? QColor("#252526") : QColor("#f3f3f3");
    
    // Set all color groups
    for (QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
        palette.setColor(group, QPalette::Window, bg);
        palette.setColor(group, QPalette::WindowText, fg);
        palette.setColor(group, QPalette::Base, base);
        palette.setColor(group, QPalette::AlternateBase, bg);
        palette.setColor(group, QPalette::Text, fg);
        palette.setColor(group, QPalette::Button, base);
        palette.setColor(group, QPalette::ButtonText, fg);
        palette.setColor(group, QPalette::Highlight, highlight);
        palette.setColor(group, QPalette::HighlightedText, QColor("#ffffff"));
        
        // Disabled state
        if (group == QPalette::Disabled) {
            palette.setColor(group, QPalette::WindowText, fg.darker(150));
            palette.setColor(group, QPalette::Text, fg.darker(150));
        }
    }
    
    return palette;
}

void ThemeEngine::registerColorToken(const QString& id, const QColor& color, 
                                      const QString& description)
{
    ColorToken token;
    token.id = id;
    token.color = color;
    token.description = description;
    m_colors[id] = token;
}

void ThemeEngine::setTokenColor(const QString& tokenType, const QColor& color)
{
    m_tokenColors[tokenType] = color.name();
}

void ThemeEngine::setupDefaultDarkTheme()
{
    // VS Code Dark+ color tokens
    registerColorToken("editor.background", QColor("#1e1e1e"), "Editor background");
    registerColorToken("editor.foreground", QColor("#d4d4d4"), "Editor foreground");
    registerColorToken("editor.lineHighlightBackground", QColor("#282828"), "Current line highlight");
    registerColorToken("editorLineNumber.foreground", QColor("#858585"), "Line numbers");
    registerColorToken("editorLineNumber.activeForeground", QColor("#c6c6c6"), "Active line number");
    registerColorToken("editorCursor.foreground", QColor("#aeafad"), "Cursor color");
    registerColorToken("editor.selectionBackground", QColor("#264f78"), "Selection background");
    registerColorToken("editor.inactiveSelectionBackground", QColor("#3a3d41"), "Inactive selection");
    registerColorToken("editorIndentGuide.background", QColor("#404040"), "Indent guides");
    registerColorToken("editorIndentGuide.activeBackground", QColor("#707070"), "Active indent guide");
    registerColorToken("editorWhitespace.foreground", QColor("#e3e4e2"), "Whitespace characters");
    
    // UI colors
    registerColorToken("sideBar.background", QColor("#252526"), "Sidebar background");
    registerColorToken("sideBar.foreground", QColor("#cccccc"), "Sidebar foreground");
    registerColorToken("sideBarSectionHeader.background", QColor("#252526"), "Sidebar section header");
    registerColorToken("activityBar.background", QColor("#333333"), "Activity bar background");
    registerColorToken("activityBar.foreground", QColor("#ffffff"), "Activity bar icon active");
    registerColorToken("activityBar.inactiveForeground", QColor("#858585"), "Activity bar icon inactive");
    registerColorToken("activityBarBadge.background", QColor("#007acc"), "Activity bar badge");
    registerColorToken("activityBarBadge.foreground", QColor("#ffffff"), "Activity bar badge text");
    
    // Tab colors
    registerColorToken("tab.activeBackground", QColor("#1e1e1e"), "Active tab background");
    registerColorToken("tab.activeForeground", QColor("#ffffff"), "Active tab text");
    registerColorToken("tab.inactiveBackground", QColor("#2d2d2d"), "Inactive tab background");
    registerColorToken("tab.inactiveForeground", QColor("#969696"), "Inactive tab text");
    registerColorToken("tab.border", QColor("#1e1e1e"), "Tab border");
    registerColorToken("tab.activeBorder", QColor("#007fd4"), "Active tab border top");
    
    // Panel colors
    registerColorToken("panel.background", QColor("#1e1e1e"), "Panel background");
    registerColorToken("panel.border", QColor("#3c3c3c"), "Panel border");
    registerColorToken("statusBar.background", QColor("#007acc"), "Status bar background");
    registerColorToken("statusBar.foreground", QColor("#ffffff"), "Status bar foreground");
    registerColorToken("statusBar.debuggingBackground", QColor("#cc6633"), "Debug status bar");
    
    // List colors
    registerColorToken("list.activeSelectionBackground", QColor("#37373d"), "List selected");
    registerColorToken("list.activeSelectionForeground", QColor("#ffffff"), "List selected text");
    registerColorToken("list.hoverBackground", QColor("#2a2d2e"), "List hover");
    registerColorToken("list.focusBackground", QColor("#094771"), "List focus");
    
    // Editor widget colors
    registerColorToken("editorWidget.background", QColor("#252526"), "Editor widget bg");
    registerColorToken("editorWidget.border", QColor("#3c3c3c"), "Editor widget border");
    registerColorToken("input.background", QColor("#3c3c3c"), "Input field background");
    registerColorToken("input.foreground", QColor("#cccccc"), "Input field text");
    registerColorToken("input.border", QColor("#3c3c3c"), "Input field border");
    registerColorToken("inputOption.activeBorder", QColor("#007fd4"), "Input option border");
    
    // Scrollbar
    registerColorToken("scrollbarSlider.background", QColor("#424242"), "Scrollbar slider");
    registerColorToken("scrollbarSlider.hoverBackground", QColor("#4f4f4f"), "Scrollbar hover");
    registerColorToken("scrollbarSlider.activeBackground", QColor("#bfbfbf"), "Scrollbar active");
    
    // Git colors
    registerColorToken("gitDecoration.modifiedResourceForeground", QColor("#e2c08d"), "Git modified");
    registerColorToken("gitDecoration.addedResourceForeground", QColor("#89d185"), "Git added");
    registerColorToken("gitDecoration.deletedResourceForeground", QColor("#f48771"), "Git deleted");
    registerColorToken("gitDecoration.untrackedResourceForeground", QColor("#73c991"), "Git untracked");
    registerColorToken("gitDecoration.conflictingResourceForeground", QColor("#e76865"), "Git conflict");
    
    // Diagnostic colors
    registerColorToken("editorError.foreground", QColor("#f48771"), "Error underline");
    registerColorToken("editorWarning.foreground", QColor("#cca700"), "Warning underline");
    registerColorToken("editorInfo.foreground", QColor("#3794ff"), "Info underline");
    
    // Syntax highlighting (Dark+)
    m_tokenColors["comment"] = "#6a9955";
    m_tokenColors["string"] = "#ce9178";
    m_tokenColors["keyword"] = "#569cd6";
    m_tokenColors["operator"] = "#d4d4d4";
    m_tokenColors["number"] = "#b5cea8";
    m_tokenColors["function"] = "#dcdcaa";
    m_tokenColors["class"] = "#4ec9b0";
    m_tokenColors["type"] = "#4ec9b0";
    m_tokenColors["variable"] = "#9cdcfe";
    m_tokenColors["parameter"] = "#9cdcfe";
    m_tokenColors["property"] = "#9cdcfe";
    m_tokenColors["constant"] = "#569cd6";
    m_tokenColors["tag"] = "#569cd6";
    m_tokenColors["attribute"] = "#9cdcfe";
}

void ThemeEngine::setupDefaultLightTheme()
{
    // VS Code Light+ color tokens
    registerColorToken("editor.background", QColor("#ffffff"), "Editor background");
    registerColorToken("editor.foreground", QColor("#000000"), "Editor foreground");
    registerColorToken("editor.lineHighlightBackground", QColor("#f0f0f0"), "Current line highlight");
    registerColorToken("editorLineNumber.foreground", QColor("#237893"), "Line numbers");
    registerColorToken("editorCursor.foreground", QColor("#000000"), "Cursor color");
    registerColorToken("editor.selectionBackground", QColor("#add6ff"), "Selection background");
    
    // Syntax highlighting (Light+)
    m_tokenColors["comment"] = "#008000";
    m_tokenColors["string"] = "#a31515";
    m_tokenColors["keyword"] = "#0000ff";
    m_tokenColors["operator"] = "#000000";
    m_tokenColors["number"] = "#098658";
    m_tokenColors["function"] = "#795e26";
    m_tokenColors["class"] = "#267f99";
    m_tokenColors["type"] = "#267f99";
    m_tokenColors["variable"] = "#001080";
    m_tokenColors["parameter"] = "#001080";
    m_tokenColors["property"] = "#001080";
    m_tokenColors["constant"] = "#0000ff";
    m_tokenColors["tag"] = "#800000";
}

QString ThemeEngine::loadStylesheet(const QString& themeName)
{
    // Try resource path first
    QString resPath = QString(":/themes/%1.qss").arg(themeName);
    QFile resFile(resPath);
    if (resFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString content = QString::fromUtf8(resFile.readAll());
        resFile.close();
        return content;
    }
    
    // Try filesystem paths
    QStringList searchPaths;
    searchPaths << QCoreApplication::applicationDirPath() + "/themes"
                << QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/themes"
                << QDir::homePath() + "/.monolith/themes";
    
    for (const QString& basePath : searchPaths) {
        QString fsPath = basePath + "/" + themeName + ".qss";
        QFile fsFile(fsPath);
        if (fsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = QString::fromUtf8(fsFile.readAll());
            fsFile.close();
            qDebug() << "Loaded theme from filesystem:" << fsPath;
            return content;
        }
    }
    
    // Return empty if not found
    return QString();
}

void ThemeEngine::parseColorDefinitions()
{
    // This would parse CSS-like color definitions from the stylesheet
    // For now, colors are defined programmatically in setupDefault*Theme()
}

} // namespace monolith
