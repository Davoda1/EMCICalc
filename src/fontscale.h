// SPDX-License-Identifier: MIT
#pragma once
#include <QtWidgets>

// Desktop text scaling uses Qt logical fonts. The OS/Qt continues to own DPI.
// Each explicit widget font keeps one unscaled baseline; no rebuild is needed.
namespace DesktopFont {
inline QString normalizeSizeMode(const QString &s) {
    if(s=="fixed"||s=="Fixed size") return QStringLiteral("Medium");
    return s=="Small"||s=="Medium"||s=="Large"?s:QStringLiteral("System");
}
inline QString sizeMode(){return qApp?normalizeSizeMode(qApp->property("desktopFontSizeMode").toString()):QStringLiteral("System");}
inline qreal presetScale(){const auto m=sizeMode();return m=="Small"?.9:m=="Large"?1.2:1.;}
inline bool fixedSize(){return sizeMode()!=QStringLiteral("System");}
inline qreal scale(qreal size){return size*presetScale();}
inline qreal logicalPixels(qreal size){return scale(size);}
inline int pixels(qreal size){return qMax(1,qRound(scale(size)));}
inline qreal fontPixels(const QFont &f){return f.pixelSize()>0?f.pixelSize():QFontInfo(f).pixelSize();}
inline qreal nominalFromPixels(qreal size){return size/presetScale();}
inline QFont scaledFont(QFont f,qreal factor){if(f.pointSizeF()>0)f.setPointSizeF(qMax(.5,f.pointSizeF()*factor));else f.setPixelSize(qMax(1,qRound(f.pixelSize()*factor)));return f;}
inline QFont baseFont(){const auto v=qApp?qApp->property("desktopBaseFont"):QVariant();return v.isValid()?qvariant_cast<QFont>(v):QApplication::font();}
inline QFont font(qreal logical,QFont base){
    const auto *screen=QGuiApplication::primaryScreen();
    base.setPointSizeF(logicalPixels(logical)*72./(screen?screen->logicalDotsPerInchY():96.));return base;
}
inline void setNominal(QWidget *w,qreal logical){w->setProperty("desktopNominalPixels",logical);w->setFont(font(logical,w->font()));}
inline qreal nominal(const QWidget *w){const auto v=w->property("desktopNominalPixels");return v.isValid()?v.toDouble():nominalFromPixels(fontPixels(w->font()));}
inline QFont enlarged(const QWidget *w,qreal factor){return scaledFont(w->font(),factor);}
inline bool &refreshing(){static bool flag=false;return flag;}
inline void remember(QWidget *w,qreal currentFactor){
    if(w->property("desktopNominalPixels").isValid()||w->property("desktopFontDerived").toBool())return;
    if(w->testAttribute(Qt::WA_SetFont)&&!w->property("desktopNominalFont").isValid())
        w->setProperty("desktopNominalFont",scaledFont(w->font(),1./currentFactor));
}
inline void apply(QWidget *root){
    if(!root||refreshing())return;refreshing()=true;
    auto widgets=root->findChildren<QWidget*>();widgets.prepend(root);
    for(auto *w:widgets){
        remember(w,presetScale());
        QFont desired=w->font();
        if(w->property("desktopNominalPixels").isValid())desired=font(w->property("desktopNominalPixels").toDouble(),w->font());
        else if(w->property("desktopNominalFont").isValid())desired=scaledFont(qvariant_cast<QFont>(w->property("desktopNominalFont")),presetScale());
        // Qt propagates FontChange/layout invalidation when a font changes.
        // Repainting every unchanged hidden widget makes simple menu/theme
        // transitions unnecessarily expensive as the workspace grows.
        if(desired!=w->font())w->setFont(desired);
    }refreshing()=false;
}
inline void initialize(QApplication &app){if(!app.property("desktopBaseFont").isValid())app.setProperty("desktopBaseFont",app.font());}
inline void refresh(QApplication &app){
    if(refreshing())return;initialize(app);app.setFont(scaledFont(baseFont(),presetScale()));
    for(auto *w:QApplication::topLevelWidgets())apply(w);
    QToolTip::setFont(scaledFont(baseFont(),presetScale()));
}
inline void setSizeMode(const QString &value){
    if(!qApp)return;initialize(*qApp);const auto mode=normalizeSizeMode(value);
    if(sizeMode()==mode)return;
    const qreal old=presetScale();for(auto *w:QApplication::allWidgets())remember(w,old);
    qApp->setProperty("desktopFontSizeMode",mode);refresh(*qApp);
}
inline void setChoice(const QString &value){setSizeMode(value);}
}
