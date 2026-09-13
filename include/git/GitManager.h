// GitManager.h - Libgit2 Workspace Wrapper
#ifndef MONOLITH_GIT_GITMANAGER_H
#define MONOLITH_GIT_GITMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QVector>
#include <QDateTime>
#include <memory>

// Forward declare libgit2 types
typedef struct git_repository git_repository;
typedef struct git_commit git_commit;
typedef struct git_branch git_branch;
typedef struct git_diff git_diff;
typedef struct git_status_list git_status_list;

namespace monolith {

/**
 * @brief Git status flags for files
 */
enum class FileStatus {
    Unmodified = 0,
    New          = (1 << 0),      // Newly added file
    Modified     = (1 << 1),      // File content changed
    Deleted      = (1 << 2),      // File was deleted
    Renamed      = (1 << 3),      // File was renamed
    TypeChange   = (1 << 4),      // File type changed
    Untracked    = (1 << 5),      // Untracked file
    Ignored      = (1 << 6),      // Ignored file
    Conflicted   = (1 << 7),      // Conflicted state
    
    IndexNew     = (1 << 8),      // Added to index
    IndexModified = (1 << 9),     // Modified in index
    IndexDeleted = (1 << 10),     // Deleted from index
    IndexRenamed = (1 << 11),     // Renamed in index
    IndexTypeChange = (1 << 12)   // Type changed in index
};

Q_DECLARE_FLAGS(FileStatusFlags, FileStatus)

/**
 * @brief Information about a git status entry
 */
struct GitStatusEntry {
    QString filePath;
    QString oldFilePath;  // For renames
    FileStatusFlags status;
    bool isStaged;
};

/**
 * @brief Commit information
 */
struct GitCommit {
    QString sha;
    QString shortSha;
    QString message;
    QString body;
    QString authorName;
    QString authorEmail;
    QDateTime authorDate;
    QString committerName;
    QString committerEmail;
    QDateTime committerDate;
    QStringList parents;
    int parentCount;
};

/**
 * @brief Branch information
 */
struct GitBranch {
    QString name;
    QString fullName;  // refs/heads/main
    QString upstream;
    QString upstreamRemote;
    bool isHead;
    bool isRemote;
    int ahead;
    int behind;
};

/**
 * @brief Diff hunk for display
 */
struct DiffHunk {
    int oldStart;
    int oldLines;
    int newStart;
    int newLines;
    QString header;
    QString content;  // Unified diff format
};

/**
 * @brief Line blame information
 */
struct GitBlameLine {
    int lineNumber;
    QString commitSha;
    QString author;
    QString authorEmail;
    QDateTime commitTime;
    QString message;
    int previousLineNumber;  // For moved/copied lines
};

/**
 * @brief Repository statistics
 */
struct GitStats {
    int totalCommits;
    int totalBranches;
    int totalRemotes;
    int uncommittedChanges;
    int stagedChanges;
    int untrackedFiles;
    QString currentBranch;
    QString headSha;
};

/**
 * @brief High-level Git manager using libgit2
 * 
 * Provides complete Git integration including:
 * - Repository discovery and management
 * - Status tracking with file overlays
 * - Staging/unstaging changes
 * - Commit creation
 * - Branch management
 * - Remote operations (fetch, pull, push)
 * - Diff viewing
 * - Blame annotations
 */
class GitManager : public QObject
{
    Q_OBJECT

public:
    explicit GitManager(QObject* parent = nullptr);
    ~GitManager() override;

    /**
     * @brief Open a repository at the given path
     * @param path Path to repository or subdirectory
     * @return true if successful
     */
    bool openRepository(const QString& path);

    /**
     * @brief Close the current repository
     */
    void closeRepository();

    /**
     * @brief Check if a repository is open
     */
    bool isOpen() const { return m_repository != nullptr; }

    /**
     * @brief Get the repository root path
     */
    QString repositoryPath() const { return m_repositoryPath; }

    /**
     * @brief Get the current branch name
     */
    QString currentBranch() const;

    /**
     * @brief Get repository statistics
     */
    GitStats getStats() const;

    /**
     * @brief Get status of all files in the working directory
     */
    QVector<GitStatusEntry> getStatus() const;

    /**
     * @brief Get status of a single file
     */
    FileStatusFlags getFileStatus(const QString& filePath) const;

    /**
     * @brief Stage a file
     */
    bool stageFile(const QString& filePath);

    /**
     * @brief Unstage a file
     */
    bool unstageFile(const QString& filePath);

    /**
     * @brief Stage all modified files
     */
    bool stageAll();

    /**
     * @brief Unstage all files
     */
    bool unstageAll();

    /**
     * @brief Create a commit
     * @param message Commit message
     * @param authorName Author name
     * @param authorEmail Author email
     * @return The commit SHA or empty string on failure
     */
    QString commit(const QString& message, const QString& authorName, const QString& authorEmail);

    /**
     * @brief Discard changes in working directory
     */
    bool discardChanges(const QString& filePath);

    /**
     * @brief Discard all changes
     */
    bool discardAllChanges();

    /**
     * @brief Get list of local branches
     */
    QVector<GitBranch> getLocalBranches() const;

    /**
     * @brief Get list of remote branches
     */
    QVector<GitBranch> getRemoteBranches() const;

    /**
     * @brief Get all branches
     */
    QVector<GitBranch> getAllBranches() const;

    /**
     * @brief Create a new branch
     */
    bool createBranch(const QString& name, const QString& startPoint = "HEAD");

    /**
     * @brief Delete a branch
     */
    bool deleteBranch(const QString& name);

    /**
     * @brief Checkout a branch
     */
    bool checkoutBranch(const QString& name);

    /**
     * @brief Get the upstream branch for current HEAD
     */
    QString getUpstreamBranch() const;

    /**
     * @brief Fetch from remote
     */
    bool fetch(const QString& remote = "origin");

    /**
     * @brief Pull from remote
     */
    bool pull(const QString& remote = "origin", const QString& branch = "");

    /**
     * @brief Push to remote
     */
    bool push(const QString& remote = "origin", const QString& branch = "", bool force = false);

    /**
     * @brief Get commit history
     * @param maxCount Maximum number of commits to return
     */
    QVector<GitCommit> getLog(int maxCount = 50) const;

    /**
     * @brief Get details of a specific commit
     */
    GitCommit getCommit(const QString& sha) const;

    /**
     * @brief Generate diff between two commits
     */
    QVector<DiffHunk> getDiff(const QString& oldSha, const QString& newSha, const QString& filePath = {}) const;

    /**
     * @brief Generate diff of staged changes
     */
    QVector<DiffHunk> getStagedDiff(const QString& filePath = {}) const;

    /**
     * @brief Generate diff of unstaged changes
     */
    QVector<DiffHunk> getUnstagedDiff(const QString& filePath = {}) const;

    /**
     * @brief Get blame for a file
     */
    QVector<GitBlameLine> getBlame(const QString& filePath) const;

    /**
     * @brief Get line blame for a specific line
     */
    GitBlameLine getBlameLine(const QString& filePath, int lineNumber) const;

    /**
     * @brief Check if a path is ignored by .gitignore
     */
    bool isIgnored(const QString& filePath) const;

    /**
     * @brief Add an untracked file to the index
     */
    bool addUntracked(const QString& filePath);

    /**
     * @brief Get list of remotes
     */
    QStringList getRemotes() const;

    /**
     * @brief Add a remote
     */
    bool addRemote(const QString& name, const QString& url);

    /**
     * @brief Remove a remote
     */
    bool removeRemote(const QString& name);

signals:
    void repositoryOpened(const QString& path);
    void repositoryClosed();
    void statusChanged();
    void branchChanged(const QString& branchName);
    void headMoved(const QString& sha);
    void fetchCompleted(const QString& remote);
    void pullCompleted();
    void pushCompleted();
    void errorOccurred(const QString& message);

private:
    void cleanup();
    QString resolvePath(const QString& path) const;
    FileStatusFlags convertStatus(unsigned int flags) const;
    GitCommit parseCommit(git_commit* commit) const;

    git_repository* m_repository;
    QString m_repositoryPath;
    mutable git_status_list* m_cachedStatus;
    mutable bool m_statusDirty;
};

} // namespace monolith

Q_DECLARE_OPERATORS_FOR_FLAGS(monolith::FileStatusFlags)

#endif // MONOLITH_GIT_GITMANAGER_H
