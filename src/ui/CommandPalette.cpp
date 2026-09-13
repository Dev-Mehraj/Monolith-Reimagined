// CommandPalette.cpp - Fuzzy Search Command Palette Implementation
#include "ui/CommandPalette.h"
#include "ui/FuzzyMatcher.h"
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QTimer>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QDebug>

namespace monolith {

CommandPalette::CommandPalette(QWidget* parent)
    : QDialog(parent, Qt::Popup | Qt::FramelessWindowHint)
    , m_searchEdit(nullptr)
    , m_resultList(nullptr)
    , m_statusLabel(nullptr)
    , m_layout(nullptr)
    , m_mode(Mode::Command)
    , m_filterTimer(new QTimer(this))
    , m_pendingUpdate(false)
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setMinimumWidth(600);
    setMaximumWidth(800);
    setMaximumHeight(400);
    
    setupUI();
    setupConnections();
    
    // Debounce filter updates for performance
    m_filterTimer->setSingleShot(true);
    m_filterTimer->setInterval(50);
    connect(m_filterTimer, &QTimer::timeout, this, &CommandPalette::updateFilter);
    
    loadRecentFiles();
}

CommandPalette::~CommandPalette()
{
}

void CommandPalette::setupUI()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(1, 1, 1, 1);
    m_layout->setSpacing(0);
    
    // Search input
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(">");
    m_searchEdit->setMinimumHeight(40);
    m_searchEdit->setClearButtonEnabled(true);
    m_layout->addWidget(m_searchEdit);
    
    // Result list
    m_resultList = new QListWidget(this);
    m_resultList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_resultList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_resultList->setMouseTracking(true);
    m_layout->addWidget(m_resultList);
    
    // Status label
    m_statusLabel = new QLabel("", this);
    m_statusLabel->setStyleSheet("color: #858585; padding: 4px 8px; font-size: 11px;");
    m_layout->addWidget(m_statusLabel);
}

void CommandPalette::setupConnections()
{
    connect(m_searchEdit, &QLineEdit::textChanged, 
            this, [this](const QString& text) {
        m_currentFilter = text;
        m_filterTimer->start();  // Debounce
    });
    
    connect(m_resultList, &QListWidget::itemActivated,
            this, &CommandPalette::onItemActivated);
    
    connect(m_resultList, &QListWidget::itemClicked,
            this, &CommandPalette::onItemClicked);
}

void CommandPalette::registerCommand(const QString& id, const QString& label,
                                      std::function<void()> callback,
                                      const QString& category,
                                      const QString& shortcut)
{
    CommandItem item;
    item.id = id;
    item.label = label;
    item.description = category;
    item.category = category;
    item.shortcut = shortcut;
    item.score = 0;
    item.callback = std::move(callback);
    
    m_commands.append(item);
}

void CommandPalette::unregisterCommand(const QString& id)
{
    m_commands.removeIf([id](const CommandItem& item) {
        return item.id == id;
    });
}

void CommandPalette::setFiles(const QStringList& files)
{
    m_files.clear();
    
    for (const QString& path : files) {
        FileItem item;
        item.path = path;
        
        QFileInfo fi(path);
        item.fileName = fi.fileName();
        
        if (!m_workspaceRoot.isEmpty() && path.startsWith(m_workspaceRoot)) {
            item.relativePath = path.mid(m_workspaceRoot.length() + 1);
        } else {
            item.relativePath = path;
        }
        
        item.size = fi.size();
        item.lastModified = fi.lastModified();
        item.score = 0;
        
        m_files.append(item);
    }
}

void CommandPalette::setSymbols(const QVector<SymbolItem>& symbols)
{
    m_symbols = symbols;
}

void CommandPalette::showCommandMode()
{
    m_mode = Mode::Command;
    updatePlaceholder();
    m_searchEdit->clear();
    m_searchEdit->setFocus();
    filterCommands();
    updateList();
    
    showDialog();
}

void CommandPalette::showFileMode()
{
    m_mode = Mode::File;
    updatePlaceholder();
    m_searchEdit->clear();
    m_searchEdit->setFocus();
    m_filteredFiles = m_files;  // Show all initially
    updateList();
    
    showDialog();
}

void CommandPalette::showSymbolMode()
{
    m_mode = Mode::Symbol;
    updatePlaceholder();
    m_searchEdit->clear();
    m_searchEdit->setFocus();
    filterSymbols();
    updateList();
    
    showDialog();
}

void CommandPalette::setWorkspaceRoot(const QString& path)
{
    m_workspaceRoot = path;
}

void CommandPalette::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    m_searchEdit->setFocus();
    m_searchEdit->selectAll();
}

void CommandPalette::hideEvent(QHideEvent* event)
{
    QDialog::hideEvent(event);
    clear();
}

void CommandPalette::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
        case Qt::Key_Down:
            selectNext();
            break;
            
        case Qt::Key_Up:
            selectPrevious();
            break;
            
        case Qt::Key_Return:
        case Qt::Key_Enter:
            executeCurrent();
            break;
            
        case Qt::Key_Escape:
            reject();
            break;
            
        default:
            QDialog::keyPressEvent(event);
    }
}

void CommandPalette::focusOutEvent(QFocusEvent* event)
{
    // Close when focus is lost (click outside)
    if (!m_resultList->hasFocus() && !m_searchEdit->hasFocus()) {
        hide();
    }
    QDialog::focusOutEvent(event);
}

void CommandPalette::onTextChanged(const QString& text)
{
    m_currentFilter = text;
    updateFilter();
}

void CommandPalette::onItemActivated(QListWidgetItem* item)
{
    executeItem(item);
}

void CommandPalette::onItemClicked(QListWidgetItem* item)
{
    m_resultList->setCurrentItem(item);
}

void CommandPalette::updateFilter()
{
    switch (m_mode) {
        case Mode::Command:
            filterCommands();
            break;
        case Mode::File:
            filterFiles();
            break;
        case Mode::Symbol:
            filterSymbols();
            break;
    }
    updateList();
}

void CommandPalette::selectNext()
{
    int currentRow = m_resultList->currentRow();
    if (currentRow < m_resultList->count() - 1) {
        m_resultList->setCurrentRow(currentRow + 1);
    } else if (m_resultList->count() > 0) {
        m_resultList->setCurrentRow(0);  // Wrap around
    }
}

void CommandPalette::selectPrevious()
{
    int currentRow = m_resultList->currentRow();
    if (currentRow > 0) {
        m_resultList->setCurrentRow(currentRow - 1);
    } else if (m_resultList->count() > 0) {
        m_resultList->setCurrentRow(m_resultList->count() - 1);  // Wrap around
    }
}

void CommandPalette::executeCurrent()
{
    QListWidgetItem* currentItem = m_resultList->currentItem();
    if (currentItem) {
        executeItem(currentItem);
    } else if (m_resultList->count() > 0) {
        // Execute first item if nothing selected
        executeItem(m_resultList->item(0));
    }
}

void CommandPalette::filterCommands()
{
    m_filteredCommands.clear();
    
    QString pattern = m_currentFilter;
    if (pattern.startsWith('>')) {
        pattern = pattern.mid(1);
    }
    pattern = pattern.trimmed();
    
    if (pattern.isEmpty()) {
        m_filteredCommands = m_commands;
        return;
    }
    
    for (const auto& cmd : m_commands) {
        int score = FuzzyMatcher::match(pattern, cmd.label);
        
        // Also search in description/category
        if (score == 0 && !cmd.category.isEmpty()) {
            score = FuzzyMatcher::match(pattern, cmd.category);
        }
        
        if (score > 0) {
            CommandItem item = cmd;
            item.score = score;
            m_filteredCommands.append(item);
        }
    }
    
    // Sort by score
    std::sort(m_filteredCommands.begin(), m_filteredCommands.end());
}

void CommandPalette::filterFiles()
{
    m_filteredFiles.clear();
    
    QString pattern = m_currentFilter.trimmed();
    
    if (pattern.isEmpty()) {
        m_filteredFiles = m_files;
        return;
    }
    
    for (const auto& file : m_files) {
        // Match against filename first (higher priority)
        int fileNameScore = FuzzyMatcher::match(pattern, file.fileName);
        
        // Match against relative path
        int pathScore = FuzzyMatcher::match(pattern, file.relativePath);
        
        // Use the better score
        int score = std::max(fileNameScore, pathScore);
        
        // Bonus for exact substring matches
        if (file.fileName.contains(pattern, Qt::CaseInsensitive)) {
            score += 10;
        }
        
        if (score > 0) {
            FileItem item = file;
            item.score = score;
            m_filteredFiles.append(item);
        }
    }
    
    // Sort by score
    std::sort(m_filteredFiles.begin(), m_filteredFiles.end());
    
    // Limit results for performance
    if (m_filteredFiles.size() > 100) {
        m_filteredFiles.resize(100);
    }
}

void CommandPalette::filterSymbols()
{
    m_filteredSymbols.clear();
    
    QString pattern = m_currentFilter.trimmed();
    
    if (pattern.isEmpty()) {
        m_filteredSymbols = m_symbols;
        return;
    }
    
    for (const auto& sym : m_symbols) {
        int score = FuzzyMatcher::match(pattern, sym.name);
        
        // Also search in container name
        if (score == 0 && !sym.containerName.isEmpty()) {
            score = FuzzyMatcher::match(pattern, sym.containerName);
        }
        
        if (score > 0) {
            SymbolItem item = sym;
            item.score = score;
            m_filteredSymbols.append(item);
        }
    }
    
    std::sort(m_filteredSymbols.begin(), m_filteredSymbols.end());
}

void CommandPalette::updateList()
{
    m_resultList->clear();
    
    switch (m_mode) {
        case Mode::Command:
            for (const auto& cmd : m_filteredCommands) {
                QListWidgetItem* item = new QListWidgetItem(cmd.label);
                if (!cmd.description.isEmpty()) {
                    item->setText(cmd.label + "\t" + cmd.description);
                }
                if (!cmd.shortcut.isEmpty()) {
                    item->setText(cmd.label + "\t" + cmd.shortcut);
                }
                item->setData(Qt::UserRole, QVariant::fromValue(cmd.id));
                m_resultList->addItem(item);
            }
            m_statusLabel->setText(QString("%1 commands").arg(m_filteredCommands.size()));
            break;
            
        case Mode::File:
            for (const auto& file : m_filteredFiles) {
                QListWidgetItem* item = new QListWidgetItem(file.fileName);
                item->setText(file.relativePath);
                item->setData(Qt::UserRole, QVariant::fromValue(file.path));
                m_resultList->addItem(item);
            }
            m_statusLabel->setText(QString("%1 files").arg(m_filteredFiles.size()));
            break;
            
        case Mode::Symbol:
            for (const auto& sym : m_filteredSymbols) {
                QListWidgetItem* item = new QListWidgetItem(sym.name);
                item->setText(sym.name + " (" + sym.containerName + ")");
                item->setData(Qt::UserRole, QVariant::fromValue(sym.filePath));
                item->setData(Qt::UserRole + 1, sym.line);
                item->setData(Qt::UserRole + 2, sym.column);
                m_resultList->addItem(item);
            }
            m_statusLabel->setText(QString("%1 symbols").arg(m_filteredSymbols.size()));
            break;
    }
    
    if (m_resultList->count() > 0) {
        m_resultList->setCurrentRow(0);
    }
}

void CommandPalette::executeItem(QListWidgetItem* item)
{
    hide();
    
    switch (m_mode) {
        case Mode::Command: {
            QString commandId = item->data(Qt::UserRole).toString();
            emit commandExecuted(commandId);
            
            // Find and execute callback
            for (const auto& cmd : m_commands) {
                if (cmd.id == commandId && cmd.callback) {
                    cmd.callback();
                    break;
                }
            }
            break;
        }
        
        case Mode::File: {
            QString filePath = item->data(Qt::UserRole).toString();
            emit fileSelected(filePath);
            saveRecentFile(filePath);
            break;
        }
        
        case Mode::Symbol: {
            QString filePath = item->data(Qt::UserRole).toString();
            int line = item->data(Qt::UserRole + 1).toInt();
            int column = item->data(Qt::UserRole + 2).toInt();
            emit symbolSelected(filePath, line, column);
            break;
        }
    }
}

void CommandPalette::updatePlaceholder()
{
    switch (m_mode) {
        case Mode::Command:
            m_searchEdit->setPlaceholderText("> Type a command or search...");
            break;
        case Mode::File:
            m_searchEdit->setPlaceholderText("? Search files by name...");
            break;
        case Mode::Symbol:
            m_searchEdit->setPlaceholderText("@ Search symbols...");
            break;
    }
}

void CommandPalette::loadRecentFiles()
{
    QSettings settings;
    settings.beginGroup("QuickOpen");
    m_recentFiles = settings.value("recentFiles", QStringList()).toStringList();
    settings.endGroup();
}

void CommandPalette::saveRecentFile(const QString& filePath)
{
    // Add to front of list if not already there
    m_recentFiles.removeAll(filePath);
    m_recentFiles.prepend(filePath);
    
    // Trim to max size
    while (m_recentFiles.size() > MAX_RECENT_FILES) {
        m_recentFiles.removeLast();
    }
    
    // Save to settings
    QSettings settings;
    settings.beginGroup("QuickOpen");
    settings.setValue("recentFiles", m_recentFiles);
    settings.endGroup();
}

void CommandPalette::showDialog()
{
    // Position dialog at top center of main window
    if (QWidget* parent = parentWidget()) {
        QRect parentRect = parent->geometry();
        int x = parentRect.x() + (parentRect.width() - width()) / 2;
        int y = parentRect.y() + 50;  // Offset from top
        move(x, y);
    }
    
    show();
    raise();
    activateWindow();
}

void CommandPalette::clear()
{
    m_searchEdit->clear();
    m_resultList->clear();
    m_statusLabel->clear();
    m_filteredCommands.clear();
    m_filteredFiles.clear();
    m_filteredSymbols.clear();
}

} // namespace monolith
