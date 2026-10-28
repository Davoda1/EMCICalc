// SPDX-License-Identifier: MIT
#pragma once
#include "inwindowdialog.h"
inline void showNativeHelp(QWidget *owner,const QString &title,const QString &html,const QString &closeText) {
    InWindowDialog dialog(owner);dialog.setWindowTitle(title);auto *layout=new QVBoxLayout(&dialog);
    auto *browser=new QTextBrowser;browser->setOpenExternalLinks(false);browser->setHtml(html);layout->addWidget(browser,1);
    auto *close=new QPushButton(closeText);layout->addWidget(close);QObject::connect(close,&QPushButton::clicked,&dialog,&QDialog::accept);
    dialog.resize(qMin(560,qMax(280,owner->window()->width()-32)),qMin(580,qMax(240,owner->window()->height()-32)));dialog.exec();
}
