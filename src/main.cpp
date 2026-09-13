// main.cpp - Application Entry Point
#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QFile>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QSurfaceFormat>
#include <QSplashScreen>
#include <QTimer>
#include <QDebug>
#include <iostream>

#include "core/MonolithApp.h"
#include "ui/MainWindow.h"
#include "theme/ThemeEngine.h"

#ifdef ENABLE_GIT_INTEGRATION
#include "git/GitManager.h"
#endif

using namespace monolith;

/**
 * @brief Load and apply stylesheet theme
 */
void applyTheme(QApplication& app, const QString& themeName)
{
    QFile themeFile(QString(":/themes/%1.qss").arg(themeName));
    
    if (themeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString stylesheet = QString::fromUtf8(themeFile.readAll());
        app.setStyleSheet(stylesheet);
        themeFile.close();
        qDebug() << "Applied theme:" << themeName;
    } else {
        qWarning() << "Could not load theme file:" << themeName;
        // Try loading from filesystem
        QString fsPath = QCoreApplication::applicationDirPath() + "/themes/" + themeName + ".qss";
        QFile fsFile(fsPath);
        if (fsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString stylesheet = QString::fromUtf8(fsFile.readAll());
            app.setStyleSheet(stylesheet);
            fsFile.close();
            qDebug() << "Applied theme from filesystem:" << fsPath;
        }
    }
}

/**
 * @brief Setup high-DPI scaling for modern displays
 */
void setupHighDpi()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
    
    // Enable OpenGL for better rendering performance
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setAlphaBufferSize(8);
    QSurfaceFormat::setDefaultFormat(format);
}

/**
 * @brief Print version information
 */
void printVersion()
{
    std::cout << "Monolith Reimagined v1.0.0\n";
    std::cout << "Built with Qt " << QT_VERSION_STR << "\n";
    std::cout << "C++20 Modern IDE Architecture\n";
    std::cout << "\n";
    std::cout << "Features:\n";
    std::cout << "  - Language Server Protocol (LSP) support\n";
#ifdef ENABLE_GIT_INTEGRATION
    std::cout << "  - Git integration via libgit2\n";
#endif
    std::cout << "  - Integrated terminal\n";
    std::cout << "  - Extension engine\n";
    std::cout << "  - Advanced docking system\n";
    std::cout << "\n";
}

int main(int argc, char *argv[])
{
    // Set application metadata
    QApplication::setApplicationName("Monolith Reimagined");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("Monolith");
    QApplication::setOrganizationDomain("monolith-ide.dev");
    
    // Setup High DPI before creating QApplication
    setupHighDpi();
    
    // Create application instance
    int result = 0;
    {
        MonolithApp app(argc, argv);
        
        // Setup command line parser
        QCommandLineParser parser;
        parser.setApplicationDescription("Monolith Reimagined - Full-Featured C++ IDE");
        parser.addHelpOption();
        parser.addVersionOption();
        
        // Theme option
        QCommandLineOption themeOption(
            QStringList() << "t" << "theme",
            "Use specified theme (dark_vs, light_vs, dark_plus, etc.)",
            "theme-name",
            "dark_vs"
        );
        parser.addOption(themeOption);
        
        // Workspace/folder option
        QCommandLineOption folderOption(
            QStringList() << "f" << "folder",
            "Open specified folder as workspace",
            "folder-path"
        );
        parser.addOption(folderOption);
        
        // File option
        QCommandLineOption fileOption(
            QStringList() << "file",
            "Open specified file",
            "file-path"
        );
        parser.addOption(fileOption);
        
        // No splash option
        QCommandLineOption noSplashOption(
            "no-splash",
            "Disable splash screen"
        );
        parser.addOption(noSplashOption);
        
        // Version info option
        QCommandLineOption fullVersionOption(
            "full-version",
            "Print full version information and exit"
        );
        parser.addOption(fullVersionOption);
        
        parser.process(app);
        
        // Handle full version request
        if (parser.isSet(fullVersionOption)) {
            printVersion();
            return 0;
        }
        
        // Get options
        QString themeName = parser.value(themeOption);
        QString folderPath = parser.value(folderOption);
        QString filePath = parser.value(fileOption);
        bool showSplash = !parser.isSet(noSplashOption);
        
        // Apply theme
        applyTheme(app, themeName);
        
        // Show splash screen
        QSplashScreen* splash = nullptr;
        if (showSplash) {
            QPixmap splashPixmap(":/images/splash.png");
            if (!splashPixmap.isNull()) {
                splash = new QSplashScreen(splashPixmap, Qt::WindowStaysOnTopHint);
                splash->show();
                app.processEvents();
            }
        }
        
        // Create main window
        MainWindow mainWindow;
        
        // Connect for cleanup on exit
        QObject::connect(&app, &QApplication::aboutToQuit, &mainWindow, [&]() {
            mainWindow.saveLayout();
        });
        
        if (splash) {
            splash->finish(&mainWindow);
            delete splash;
        }
        
        // Show main window
        mainWindow.show();
        
        // Open folder/file if specified
        if (!folderPath.isEmpty()) {
            QTimer::singleShot(500, [&mainWindow, folderPath]() {
                mainWindow.openFolder(folderPath);
            });
        } else if (!filePath.isEmpty()) {
            QTimer::singleShot(500, [&mainWindow, filePath]() {
                mainWindow.openFile(filePath);
            });
        }
        
        // Run event loop
        result = app.exec();
    }
    
    return result;
}
