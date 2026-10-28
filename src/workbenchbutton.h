// SPDX-License-Identifier: MIT
#pragma once
#include "desktopworkbench.h"
// Matrix/Data and guided forms retain their native QPushButton semantics and
// horizontal spacing, sharing the style-aware text sizing and painting path.
class WorkbenchButton final:public QPushButton {
public:
 explicit WorkbenchButton(const QString&text,QWidget*parent=nullptr):QPushButton(text,parent){setMinimumWidth(0);setMinimumHeight(DesktopWorkbench::controlHeight(this));setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);}
 QSize minimumSizeHint()const override{return {0,heightForWidth(qMax(160,fontMetrics().horizontalAdvance(text())+128))};}
 QSize sizeHint()const override{return {160,heightForWidth(160)};}
 bool hasHeightForWidth()const override{return true;}
 int heightForWidth(int width)const override{QStyleOptionButton option;initStyleOption(&option);return DesktopWorkbench::buttonCaptionHeight(this,option,width,QMargins(4,2,4,2));}
protected:
 void paintEvent(QPaintEvent*)override{QStyleOptionButton option;initStyleOption(&option);auto blank=option;blank.text.clear();QStylePainter painter(this);painter.drawControl(QStyle::CE_PushButton,blank);DesktopWorkbench::paintButtonCaption(painter,this,option,QMargins(4,2,4,2));}
};
