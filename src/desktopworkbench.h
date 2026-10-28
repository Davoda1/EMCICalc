// SPDX-License-Identifier: MIT
#pragma once
#include <QtWidgets>
#include <cmath>
#include "desktopstepper.h"
#include "editcue.h"
// Native desktop presentation shared by tool pages. Palette updates are scoped,
// idempotent, and do not reconstruct pages or change calculator geometry.
namespace DesktopWorkbench {
inline int controlHeight(const QWidget *w){return qMax(28,w->fontMetrics().height()+12);}
inline QFont enlarged(const QWidget *w,qreal ratio){QFont f=w->font();if(f.pointSizeF()>0)f.setPointSizeF(f.pointSizeF()*ratio);else if(f.pixelSize()>0)f.setPixelSize(qRound(f.pixelSize()*ratio));return f;}
struct Colors {
    QColor background, surface, raised, text, muted, border, accent, accentText;
    QColor backgroundText, buttonText, link, surfaceLink, mutedBackground;
    bool dark = false;
};
inline double luminance(const QColor &color) {
    auto channel=[](double c){return c<=0.04045?c/12.92:std::pow((c+0.055)/1.055,2.4);};
    return .2126*channel(color.redF())+.7152*channel(color.greenF())+.0722*channel(color.blueF());
}
inline QColor readableInk(QColor ink,const QColor &background) {
    const auto contrast=[](const QColor&a,const QColor&b){const double x=luminance(a),y=luminance(b);return (qMax(x,y)+.05)/(qMin(x,y)+.05);};
    if(ink.isValid()&&contrast(ink,background)>=4.5)return ink;
    const QColor darkInk("#111820"),lightInk("#ffffff");
    return contrast(darkInk,background)>contrast(lightInk,background)?darkInk:lightInk;
}
inline QColor accentInk(QColor accent,const QColor &background) {
    const auto contrast=[](const QColor&a,const QColor&b){const double x=luminance(a),y=luminance(b);return (qMax(x,y)+.05)/(qMin(x,y)+.05);};
    for(int i=0;i<30&&contrast(accent,background)<4.5;++i)accent=luminance(background)<.179?accent.lighter(110):accent.darker(110);
    return contrast(accent,background)>=4.5?accent:readableInk(accent,background);
}
inline Colors colors(const QPalette &source) {
    Colors c;c.dark=luminance(source.color(QPalette::Window))<.32;
    // Follow the interface palette's actual body, content and key colors. The
    // caller supplies these roles from the calculator skin, not its LCD colors.
    c.background=source.color(QPalette::Window);
    c.surface=source.color(QPalette::Base);
    c.raised=source.color(QPalette::Button);
    c.text=readableInk(source.color(QPalette::Text),c.surface);
    c.backgroundText=readableInk(source.color(QPalette::WindowText),c.background);
    c.buttonText=readableInk(source.color(QPalette::ButtonText),c.raised);
    c.border=source.color(QPalette::Mid);
    c.accent=source.color(QPalette::Highlight);
    c.accentText=readableInk(source.color(QPalette::HighlightedText),c.accent);
    c.link=accentInk(c.accent,c.background);
    c.surfaceLink=accentInk(c.accent,c.surface);
    // Disabled/secondary content stays in the skin's hue without introducing
    // an unrelated blue-grey palette.
    c.muted=QColor::fromRgbF(c.text.redF()*.70+c.surface.redF()*.30,
                           c.text.greenF()*.70+c.surface.greenF()*.30,
                           c.text.blueF()*.70+c.surface.blueF()*.30);
    c.mutedBackground=QColor::fromRgbF(c.backgroundText.redF()*.70+c.background.redF()*.30,
                                     c.backgroundText.greenF()*.70+c.background.greenF()*.30,
                                     c.backgroundText.blueF()*.70+c.background.blueF()*.30);
    return c;
}
inline const QWidget *scopeFor(const QWidget *widget) {for(auto*w=widget;w;w=w->parentWidget())if(w->property("twScope").toBool())return w;return widget;}
inline QColor linkInk(const QWidget *widget,const QPalette &palette) {
    for(auto *parent=widget;parent;parent=parent->parentWidget()) {
        if(parent->property("twRole").toByteArray()=="card")return accentInk(palette.color(QPalette::Highlight),palette.color(parent->backgroundRole()));
        if(parent->property("twScope").toBool())break;
    }
    return accentInk(palette.color(QPalette::Highlight),palette.color(QPalette::Window));
}
inline int wrappedHeight(const QString &text,const QFont &font,int width) {
    QTextLayout layout(text,font);QTextOption option;option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);layout.setTextOption(option);layout.beginLayout();qreal height=0;
    while(true){auto line=layout.createLine();if(!line.isValid())break;line.setLineWidth(qMax(1,width));line.setPosition(QPointF(0,height));height+=line.height();}layout.endLayout();return qCeil(height);
}
inline void drawWrapped(QPainter &painter,const QRectF &area,const QString &text,const QFont &font,const QColor &ink,Qt::Alignment alignment=Qt::AlignLeft|Qt::AlignTop) {
    QTextLayout layout(text,font);QTextOption option;option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);option.setAlignment(alignment&Qt::AlignHorizontal_Mask);layout.setTextOption(option);layout.beginLayout();qreal height=0;
    while(true){auto line=layout.createLine();if(!line.isValid())break;line.setLineWidth(qMax(1.,area.width()));line.setPosition(QPointF(0,height));height+=line.height();}layout.endLayout();painter.save();painter.setClipRect(area);painter.setPen(ink);const qreal y=area.top()+((alignment&Qt::AlignVCenter)?qMax(0.,(area.height()-height)/2):0);layout.draw(&painter,QPointF(area.left(),y));painter.restore();
}
inline void role(QWidget *widget,const char *name) {
    if(widget->property("twRole").toByteArray()==name)return;
    widget->setProperty("twRole",name);
    widget->style()->unpolish(widget);widget->style()->polish(widget);widget->update();
}

inline void apply(QWidget *scope,const QPalette &source);
class AppearanceGuard final:public QObject {
public:
 explicit AppearanceGuard(QWidget *scope):QObject(scope),scope_(scope){scope->installEventFilter(this);}
 void refresh(){visit(scope_);}
protected:
 bool eventFilter(QObject *watched,QEvent *event)override{
  if(event->type()==QEvent::ChildAdded||event->type()==QEvent::ChildPolished){
   QPointer<QObject> child=static_cast<QChildEvent*>(event)->child();
   QTimer::singleShot(0,this,[this,child]{if(auto *w=qobject_cast<QWidget*>(child.data()))visit(w);});
  }
  return false;
 }
private:
 void visit(QWidget *widget){
  const auto saved=scope_->property("twPalette");const QPalette canonical=saved.isValid()?qvariant_cast<QPalette>(saved):scope_->palette();
  if(widget!=scope_&&widget->property("twScope").toBool()){apply(widget,canonical);return;}
  if(!watched_.contains(widget)){
   watched_.insert(widget);widget->installEventFilter(this);connect(widget,&QObject::destroyed,this,[this,widget]{watched_.remove(widget);});
   if(auto *combo=qobject_cast<QComboBox*>(widget)){combo->setMinimumWidth(0);combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);combo->setMinimumContentsLength(1);}
   if(auto *spin=qobject_cast<QAbstractSpinBox*>(widget))DesktopStepper::install(spin);
  }
  if(!widget->property("twPreservePalette").toBool()&&widget->palette()!=canonical)widget->setPalette(canonical);
  const auto children=widget->findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly);for(auto *child:children)visit(child);
 }
 QWidget *scope_;QSet<QWidget*> watched_;
};
inline void apply(QWidget *scope,const QPalette &input){
 if(!scope||scope->property("twApplying").toBool())return;
 QPalette source=input;
 // Native theme palettes may leave Link at Qt's default pure blue. Workbench
 // headings and selected tabs use the existing theme accent as readable ink,
 // independently of selection/background colors and application branding.
 for(auto group:{QPalette::Active,QPalette::Inactive,QPalette::Disabled}){
  const auto ink=accentInk(input.color(group,QPalette::Highlight),input.color(group,QPalette::Window));
  source.setColor(group,QPalette::Link,ink);source.setColor(group,QPalette::LinkVisited,ink);
 }
 const auto saved=QVariant::fromValue(source);auto *guard=static_cast<AppearanceGuard*>(scope->property("twAppearanceGuard").value<QObject*>());
 const auto c=colors(source);const QString css=QStringLiteral("QFrame[twRole=\"card\"] { border:1px solid %1; border-radius:6px; } QLabel[twRole=\"title\"] { color:%2; font-weight:600; } QTabBar::tab:selected { color:%2; } QLabel[twRole=\"muted\"] { color:%3; } QLineEdit[twCellEditor=\"true\"]:focus { border:2px solid %4; }").arg(c.border.name(),c.link.name(),readableInk(c.mutedBackground,c.background).name(),EditCue::stroke(source).name());
 bool inheritStyle=false;
 for(auto *parent=scope->parentWidget();parent;parent=parent->parentWidget())if(parent->property("twScope").toBool()){
  inheritStyle=parent->property("twPalette")==saved&&(parent->styleSheet()==css||parent->property("twInheritedCss").toString()==css);break;
 }
 // A nested scope keeps its own canonical palette and repair guard, while
 // inheriting byte-identical rules. Reinstalling the same stylesheet on every
 // workbench page makes Qt repeatedly repolish the same descendant controls.
 const QString ownCss=inheritStyle?QString():css;
 const bool styleChanged=scope->styleSheet()!=ownCss;
 scope->setProperty("twInheritedCss",inheritStyle?css:QString());
 if(scope->property("twPalette")==saved&&guard&&!styleChanged){
  // A parent/application stylesheet can repolish an already styled scope.
  // Reassert its canonical palette without rebuilding unchanged decoration.
  if(scope->palette()!=source){scope->setPalette(source);guard->refresh();scope->setPalette(source);}
  return;
 }
 scope->setProperty("twApplying",true);scope->setProperty("twScope",true);scope->setProperty("twPalette",saved);scope->setPalette(source);scope->setAutoFillBackground(true);
 // Preserve native platform controls. Only semantic card/title decoration is
 // scoped here; combo popups, input, buttons and focus remain native Qt.
 if(styleChanged)scope->setStyleSheet(ownCss);
 if(!guard){guard=new AppearanceGuard(scope);scope->setProperty("twAppearanceGuard",QVariant::fromValue(static_cast<QObject*>(guard)));}
 guard->refresh();scope->setPalette(source);scope->setProperty("twApplying",false);
}
inline void apply(QWidget *scope){if(!scope)return;const auto *parent=scope->parentWidget();const auto *owner=parent?scopeFor(parent):scope;const auto saved=owner->property("twPalette");apply(scope,saved.isValid()?qvariant_cast<QPalette>(saved):owner->palette());}
inline QFrame *card(QWidget *parent=nullptr) {
    auto *frame=new QFrame(parent);frame->setProperty("twRole","card");frame->setAttribute(Qt::WA_StyledBackground);frame->setMinimumWidth(0);return frame;
}
class SectionLabel final:public QLabel {
public:explicit SectionLabel(const QString&text,QWidget*parent=nullptr):QLabel(text,parent){}
    bool hasHeightForWidth()const override{return true;}
    int heightForWidth(int width)const override{return wrappedHeight(text(),font(),qMax(1,width))+2;}
    QSize minimumSizeHint()const override{return {0,fontMetrics().height()+2};}
    QSize sizeHint()const override{return {160,heightForWidth(160)};}
protected:void paintEvent(QPaintEvent*)override{QPainter painter(this);const auto*scope=scopeFor(this);const auto palette=scope->property("twPalette").isValid()?qvariant_cast<QPalette>(scope->property("twPalette")):scope->palette();drawWrapped(painter,rect(),text(),font(),linkInk(this,palette));}
};
inline QLabel *sectionTitle(const QString &text,QWidget *parent=nullptr) {
    auto *label=new SectionLabel(text,parent);label->setProperty("twRole","title");label->setWordWrap(true);label->setMinimumWidth(0);label->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);return label;
}
// Measure and paint the same native content rectangle. A stylesheet already
// contributes its border and padding to SE_PushButtonContents; fixed additions
// such as textHeight+12 cannot account for those insets or focus borders.
inline QRect buttonCaptionRect(const QWidget *button,const QStyleOptionButton &option,const QMargins &extra=QMargins(8,2,8,2)) {
    auto area=button->style()->subElementRect(QStyle::SE_PushButtonContents,&option,button).marginsRemoved(extra);
    if(option.features.testFlag(QStyleOptionButton::HasMenu)){
        const int indicator=button->style()->pixelMetric(QStyle::PM_MenuButtonIndicator,&option,button);
        if(option.direction==Qt::RightToLeft)area.adjust(indicator,0,0,0);else area.adjust(0,0,-indicator,0);
    }
    return area;
}
inline int buttonTextLineHeight(const QString &text,const QFont &font,int width) {
    QTextLayout layout(text,font);QTextOption option;option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);layout.setTextOption(option);layout.beginLayout();int height=0;
    while(true){auto line=layout.createLine();if(!line.isValid())break;line.setLineWidth(qMax(1,width));height+=qCeil(line.height());}layout.endLayout();return height;
}
inline int buttonCaptionHeight(const QWidget *button,QStyleOptionButton option,int width,const QMargins &extra=QMargins(8,2,8,2)) {
    const int probeHeight=qMax(100,controlHeight(button));int height=controlHeight(button);
    option.rect=QRect(0,0,qMax(1,width),probeHeight);
    // Reserve both states so keyboard focus never changes the wrapping or
    // clips text when the active theme uses a thicker focus border.
    for(bool focused:{false,true}){
        option.state.setFlag(QStyle::State_HasFocus,focused);
        const auto area=buttonCaptionRect(button,option,extra);
        // Native labels round each line independently. Also allow one raster
        // edge pixel and the native pressed-label shift on either side; the
        // same room retains the etched disabled shadow on Windows styles.
        const int inkAllowance=2+2*qAbs(button->style()->pixelMetric(QStyle::PM_ButtonShiftVertical,&option,button));
        height=qMax(height,buttonTextLineHeight(option.text,button->font(),area.width())+probeHeight-area.height()+inkAllowance);
    }
    return height;
}
inline void paintButtonCaption(QStylePainter &painter,const QWidget *button,const QStyleOptionButton &option,const QMargins &extra=QMargins(8,2,8,2)) {
    const QRect area=buttonCaptionRect(button,option,extra);
    QTextLayout layout(option.text,button->font());QTextOption textOption;textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);layout.setTextOption(textOption);
    layout.beginLayout();qreal height=0;
    while(true){auto line=layout.createLine();if(!line.isValid())break;line.setLineWidth(qMax(1,area.width()));line.setPosition(QPointF(0,height));height+=qCeil(line.height());}layout.endLayout();
    const qreal top=area.top()+qMax(0.,(area.height()-height)/2);
    painter.save();painter.setClipRect(area);
    for(int i=0;i<layout.lineCount();++i){
        const auto line=layout.lineAt(i);QStyleOptionButton label=option;
        label.rect=QRect(area.left(),qRound(top+line.y()),area.width(),qCeil(line.height()));
        // Native label painting preserves the style's state-dependent ink,
        // including pressed, checked, disabled and inactive palettes. Keep
        // literal ampersands, as the existing wrapped captions did.
        label.text=option.text.mid(line.textStart(),line.textLength()).trimmed().replace('&',QStringLiteral("&&"));label.icon=QIcon();label.features.setFlag(QStyleOptionButton::HasMenu,false);
        painter.drawControl(QStyle::CE_PushButtonLabel,label);
    }
    painter.restore();
}
class ActionButton final:public QPushButton {
public:
    explicit ActionButton(const QString &text,QWidget *parent=nullptr):QPushButton(text,parent){setMinimumSize(0,controlHeight(this));setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);}
    QSize minimumSizeHint()const override{return {0,heightForWidth(qMax(160,fontMetrics().horizontalAdvance(text())+128))};}
    QSize sizeHint()const override{return {160,heightForWidth(160)};}
    bool hasHeightForWidth()const override{return true;}
    int heightForWidth(int width)const override{QStyleOptionButton option;initStyleOption(&option);return buttonCaptionHeight(this,option,width);}
protected:
    void paintEvent(QPaintEvent*)override{QStyleOptionButton option;initStyleOption(&option);auto blank=option;blank.text.clear();QStylePainter painter(this);painter.drawControl(QStyle::CE_PushButton,blank);paintButtonCaption(painter,this,option);}
};
inline QPushButton *action(const QString &text,QWidget *parent=nullptr,bool primary=false) {
    auto *button=new ActionButton(text,parent);button->setProperty("twRole",primary?"primary":"secondary");return button;
}
inline void tuneForm(QFormLayout *form) {
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    form->setLabelAlignment(Qt::AlignLeft);form->setContentsMargins(14,14,14,14);form->setHorizontalSpacing(12);form->setVerticalSpacing(10);
}
inline QScrollArea *scroll(QWidget *body,QWidget *parent=nullptr) {
    auto *area=new QScrollArea(parent);area->setWidgetResizable(true);area->setFrameShape(QFrame::NoFrame);area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);area->setWidget(body);area->setMinimumSize(0,0);return area;
}
}
