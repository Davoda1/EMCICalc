// SPDX-License-Identifier: MIT
#pragma once

#include <QByteArray>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QSaveFile>
#include <QString>
#include <QUrl>

// Native desktop save dialog and atomic local file replacement.
namespace EmciExport {
enum class Result { Saved, Cancelled, Failed };

inline Result save(QWidget *parent, const QString &title,
                   const QString &suggestedName, const QString &filter,
                   const QByteArray &bytes, QString *error = nullptr,
                   QString *savedPath = nullptr)
{
    if (error) error->clear();
    if (savedPath) savedPath->clear();
    QString hint = suggestedName;

    QString path = QFileDialog::getSaveFileName(parent, title, hint, filter);
    if (path.isEmpty()) return Result::Cancelled;
    if (QFileInfo(path).suffix().isEmpty()) {
        const QString suffix = QFileInfo(suggestedName).suffix();
        if (!suffix.isEmpty()) path += QLatin1Char('.') + suffix;
    }
    QSaveFile destination(path);
    if (!destination.open(QIODevice::WriteOnly)
        || destination.write(bytes) != bytes.size() || !destination.commit()) {
        if (error) *error = destination.errorString();
        return Result::Failed;
    }
    if (savedPath) *savedPath = path;
    return Result::Saved;
}
}
