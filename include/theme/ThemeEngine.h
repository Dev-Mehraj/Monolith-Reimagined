// ThemeEngine.h - Theme Management Engine
#ifndef MONOLITH_THEME_THEMEENGINE_H
#define MONOLITH_THEME_THEMEENGINE_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QColor>
#include <QPalette>

namespace monolith {

/**
 * @brief Color token structure for theme colors
 */
struct ColorToken {
    QString id;
    QColor color;
    QColor hsla;  // For transparency variations
    QString description;
};

/**
 * @brief Theme definition with all color tokens
 */
struct ThemeDefinition {
    QString name;
    QString baseTheme;  // "dark" or "light"
    QMap<QString, ColorToken> colors;
    QMap<QString, QString> tokenColors;  // Syntax highlighting colors
};

/**
 * @brief Manages application theming and color schemes
 * 
 * Provides:
 * - Theme loading from QSS files
 * - Dynamic color palette generation
 * - Token color customization for syntax highlighting
 * - Theme switching with signal notification
 */
class ThemeEngine : public QObject
{
    Q_OBJECT

public:
    explicit ThemeEngine(QObject* parent = nullptr);
    ~ThemeEngine() override;

    /**
     * @brief Load a theme by name
     * @param themeName Name of the theme (e.g., "dark_vs", "light_vs")
     * @return true if theme loaded successfully
     */
    bool loadTheme(const QString& themeName);

    /**
     * @brief Set the current theme
     * @param themeName Theme name to activate
     */
    void setTheme(const QString& themeName);

    /**
     * @brief Get the current theme name
     */
    QString currentTheme() const { return m_currentTheme; }

    /**
     * @brief Get available themes
     */
    QStringList availableThemes() const;

    /**
     * @brief Get a color by token ID
     * @param tokenId Color token identifier (e.g., "editor.background")
     * @return The color value
     */
    QColor getColor(const QString& tokenId) const;

    /**
     * @brief Get a color with modified alpha
     * @param tokenId Color token identifier
     * @param alpha Alpha value (0-255)
     * @return Color with specified alpha
     */
    QColor getColorWithAlpha(const QString& tokenId, int alpha) const;

    /**
     * @brief Generate a QPalette from the current theme
     */
    QPalette generatePalette() const;

    /**
     * @brief Get the stylesheet content for current theme
     */
    QString stylesheet() const { return m_currentStylesheet; }

    /**
     * @brief Check if current theme is dark
     */
    bool isDarkTheme() const { return m_isDarkTheme; }

    /**
     * @brief Register a custom color token
     */
    void registerColorToken(const QString& id, const QColor& color, 
                            const QString& description = {});

    /**
     * @brief Update syntax highlighting color for a token type
     */
    void setTokenColor(const QString& tokenType, const QColor& color);

signals:
    void themeChanged(const QString& themeName);
    void colorChanged(const QString& tokenId);

private:
    void setupDefaultDarkTheme();
    void setupDefaultLightTheme();
    QString loadStylesheet(const QString& themeName);
    void parseColorDefinitions();

    QString m_currentTheme;
    QString m_currentStylesheet;
    bool m_isDarkTheme;
    
    QMap<QString, ColorToken> m_colors;
    QMap<QString, QString> m_tokenColors;
    
    static const QString DEFAULT_DARK_THEME;
    static const QString DEFAULT_LIGHT_THEME;
};

} // namespace monolith

#endif // MONOLITH_THEME_THEMEENGINE_H
