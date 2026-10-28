// SPDX-License-Identifier: MIT
#pragma once
#include <QString>
#include <functional>
QString translateWorkspace(const QString &language,const char *english);
QString translateWorkspaceOutput(QString input,const std::function<QString(const char*)>&translate);
