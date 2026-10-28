// SPDX-License-Identifier: MIT
#pragma once
#include <QLockFile>
#include <QString>

// Desktop archives share the original cross-platform Qt writer-lock protocol.
// Qt checks process identity when deciding whether an abandoned lock is stale.
// A time threshold alone must never steal a long-lived active writer's lock.
class ArchiveWriteLock final {
public:
    explicit ArchiveWriteLock(const QString &path) : lock_(path) { lock_.setStaleLockTime(0); }
    ArchiveWriteLock(const ArchiveWriteLock &) = delete;
    ArchiveWriteLock &operator=(const ArchiveWriteLock &) = delete;
    ~ArchiveWriteLock() { unlock(); }
    bool tryLock(int timeoutMs = 250) {
        busy_ = false; error_.clear();
        const bool acquired = lock_.tryLock(timeoutMs);
        busy_ = !acquired && lock_.error() == QLockFile::LockFailedError;
        if (!acquired && !busy_) error_ = QStringLiteral("The archive writer lock could not be opened.");
        return acquired;
    }
    void unlock() { lock_.unlock(); }
    bool busy() const { return busy_; }
    QString error() const { return error_; }
private:
    QLockFile lock_;
    bool busy_ = false;
    QString error_;
};
