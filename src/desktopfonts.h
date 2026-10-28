// SPDX-License-Identifier: MIT
#pragma once
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QStringList>

// Register licensed glyph fallbacks without replacing the user's Qt/OS font.
// One shared startup path also covers native integration-test executables.
namespace DesktopFonts {
inline void registerBundledFonts(){
 static bool registered=false;if(registered)return;registered=true;
 const auto binary=QCoreApplication::applicationDirPath();
 const QStringList directories{binary+"/fonts",binary+"/../share/emcicalc/fonts",binary+"/share/emcicalc/fonts",QFileInfo(QString::fromUtf8(__FILE__)).absoluteDir().filePath("../assets/fonts")};
 for(const QString &name:{QStringLiteral("NotoSansCJKjp-Regular.otf"),QStringLiteral("NotoSansCJKsc-Regular.otf")})for(const auto &directory:directories){const auto path=QDir(directory).filePath(name);if(QFileInfo::exists(path)&&QFontDatabase::addApplicationFont(path)>=0)break;}
}
}
