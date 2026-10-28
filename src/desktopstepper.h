// SPDX-License-Identifier: MIT
#pragma once
#include <QtWidgets>

// Desktop presentation: the original spin box retains its value model,
// validation, locale, precision, stepping rules, signals and keyboard editor.
// Install only inside a tool/settings scope, never on the calculator keypad.
namespace DesktopStepper {
inline QPalette presentationPalette(const QWidget *widget) {
    for(auto *p=widget;p;p=p->parentWidget()) {
        const auto stored=p->property("twPalette");
        if(p->property("twScope").toBool()&&stored.isValid())return qvariant_cast<QPalette>(stored);
    }
    return widget->palette();
}

class Button final:public QAbstractButton {
public:
    Button(bool increase,QWidget *parent):QAbstractButton(parent),increase_(increase) {
        setObjectName(increase?"desktopStepIncrease":"desktopStepDecrease");
        setText(increase?QStringLiteral("+"):QString(QChar(0x2212)));
        setMinimumSize(0,0);setFocusPolicy(Qt::StrongFocus);setAutoRepeat(false);
        setProperty("twStepperButton",true);
    }
    QSize sizeHint()const override{const int side=qMax(26,fontMetrics().height()+10);return {side,side};}
protected:
    void paintEvent(QPaintEvent*)override {
        const auto pal=presentationPalette(this);
        const auto group=isEnabled()?QPalette::Active:QPalette::Disabled;
        QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(pal.color(isDown()?QPalette::Base:QPalette::Button));
        painter.setPen(QPen(pal.color(hasFocus()||isDown()?QPalette::Highlight:QPalette::Mid),hasFocus()?2.:1.));
        painter.drawRoundedRect(QRectF(rect()).adjusted(1,1,-1,-1),7,7);
        painter.setPen(QPen(pal.color(group,isDown()?QPalette::Text:QPalette::ButtonText),2.4,Qt::SolidLine,Qt::RoundCap));
        const QPointF center=QRectF(rect()).center();const qreal radius=qBound(7.,fontMetrics().height()*.3,11.);
        painter.drawLine(center+QPointF(-radius,0),center+QPointF(radius,0));
        if(increase_)painter.drawLine(center+QPointF(0,-radius),center+QPointF(0,radius));
    }
private:bool increase_;
};

class Controller final:public QObject {
public:
    explicit Controller(QAbstractSpinBox *spin):QObject(spin),spin_(spin),originalMinimumHeight_(spin->minimumHeight()),originalMinimum_(spin->minimumSize()),originalPolicy_(spin->sizePolicy()),originalSymbols_(spin->buttonSymbols()),originalStyle_(spin->styleSheet()) {
        setObjectName("desktopStepperController");
        spin_->setProperty("desktopStepper",true);spin_->setProperty("twOwnIndicator",true);
        spin_->setButtonSymbols(QAbstractSpinBox::NoButtons);
        // Let the responsive second row become reachable: QSpinBox's native
        // minimumSizeHint otherwise keeps the former inline padding forever.
        auto policy=spin_->sizePolicy();policy.setHorizontalPolicy(QSizePolicy::Ignored);spin_->setSizePolicy(policy);
        decrease_=new Button(false,spin);increase_=new Button(true,spin);
        connect(decrease_,&QAbstractButton::clicked,this,[this]{spin_->stepDown();refresh();});
        connect(increase_,&QAbstractButton::clicked,this,[this]{spin_->stepUp();refresh();});
        if(auto *integer=qobject_cast<QSpinBox*>(spin))connect(integer,&QSpinBox::valueChanged,this,[this]{refresh();});
        if(auto *decimal=qobject_cast<QDoubleSpinBox*>(spin))connect(decimal,&QDoubleSpinBox::valueChanged,this,[this]{refresh();});
        spin_->installEventFilter(this);
        if(auto *editor=spin_->findChild<QLineEdit*>())connect(editor,&QLineEdit::textChanged,this,[this]{refresh();});
        layout();refresh();decrease_->show();increase_->show();
    }
    void setActive(bool active){
        if(active_==active)return;active_=active;
        if(!active){decrease_->hide();increase_->hide();spin_->setButtonSymbols(originalSymbols_);spin_->setMinimumSize(originalMinimum_);spin_->setSizePolicy(originalPolicy_);spin_->setStyleSheet(originalStyle_);}
        else{spin_->setButtonSymbols(QAbstractSpinBox::NoButtons);auto policy=originalPolicy_;policy.setHorizontalPolicy(QSizePolicy::Ignored);spin_->setSizePolicy(policy);layout();refresh();decrease_->show();increase_->show();}
    }
    void refresh() {
        if(!active_)return;
        const bool editable=spin_->isEnabled()&&!spin_->isReadOnly();
        bool down=editable,up=editable;
        if(auto *integer=qobject_cast<QSpinBox*>(spin_)){
            down&=integer->minimum()!=integer->maximum()&&(integer->wrapping()||integer->value()>integer->minimum());
            up&=integer->minimum()!=integer->maximum()&&(integer->wrapping()||integer->value()<integer->maximum());
        }else if(auto *decimal=qobject_cast<QDoubleSpinBox*>(spin_)){
            down&=decimal->minimum()!=decimal->maximum()&&(decimal->wrapping()||decimal->value()>decimal->minimum());
            up&=decimal->minimum()!=decimal->maximum()&&(decimal->wrapping()||decimal->value()<decimal->maximum());
        }
        // A pending valid edit must first be interpreted by the native step
        // operation, even when the last committed value was at a boundary.
        if(editable&&spin_->hasAcceptableInput())if(auto *editor=spin_->findChild<QLineEdit*>();editor&&editor->isModified())down=up=true;
        if(decrease_->isEnabled()!=down)decrease_->setEnabled(down);
        if(increase_->isEnabled()!=up)increase_->setEnabled(up);
        decrease_->setAccessibleName(spin_->accessibleName()+QStringLiteral(" ")+decrease_->text());
        increase_->setAccessibleName(spin_->accessibleName()+QStringLiteral(" ")+increase_->text());
    }
protected:
    bool eventFilter(QObject*,QEvent *event)override {
        if(!active_)return false;
        switch(event->type()) {
        case QEvent::Resize:case QEvent::Show:case QEvent::FontChange:case QEvent::StyleChange:
            layout();refresh();break;
        case QEvent::EnabledChange:case QEvent::ReadOnlyChange:case QEvent::PaletteChange:
            refresh();decrease_->update();increase_->update();break;
        case QEvent::Paint:
            refresh();break;
        default:break;
        }
        return false;
    }
private:
    void layout() {
        if(layingOut_||!active_)return;
        layingOut_=true;
        const int side=qMax(26,spin_->fontMetrics().height()+10);
        const bool narrow=spin_->width()<2*side+20+spin_->fontMetrics().horizontalAdvance(QStringLiteral("000"));
        const int editHeight=qMax(side+4,spin_->fontMetrics().height()+12);
        const int minimumHeight=qMax(originalMinimumHeight_,editHeight+(narrow?side+4:0));
        if(spin_->minimumHeight()!=minimumHeight)spin_->setMinimumHeight(minimumHeight);
        if(spin_->minimumWidth()<2*side+6)spin_->setMinimumWidth(2*side+6);
        const QString css=originalStyle_+QStringLiteral("\nQAbstractSpinBox[desktopStepper=\"true\"] { padding:%1; min-height:%2px; } QAbstractSpinBox[desktopStepper=\"true\"] > QLineEdit { padding:0; border:none; min-height:0; background:transparent; }")
            .arg(narrow?QStringLiteral("0px 4px %1px 4px").arg(side+4):QStringLiteral("0px %1px 0px %1px").arg(side+4)).arg(editHeight-2);
        if(spin_->styleSheet()!=css)spin_->setStyleSheet(css);
        const int width=spin_->width(),height=spin_->height();
        if(narrow){
            const int leftWidth=(width-6)/2;
            decrease_->setGeometry(2,height-side-2,leftWidth,side);
            increase_->setGeometry(leftWidth+4,height-side-2,width-leftWidth-6,side);
        }else{
            decrease_->setGeometry(2,2,side,qMax(side,height-4));
            increase_->setGeometry(width-side-2,2,side,qMax(side,height-4));
        }
        decrease_->raise();increase_->raise();layingOut_=false;
    }
    QAbstractSpinBox *spin_;Button *decrease_,*increase_;int originalMinimumHeight_;QSize originalMinimum_;QSizePolicy originalPolicy_;QAbstractSpinBox::ButtonSymbols originalSymbols_;QString originalStyle_;bool layingOut_=false,active_=true;
};

inline void install(QAbstractSpinBox *spin) {
    if(!spin||spin->property("desktopStepper").toBool()||spin->buttonSymbols()==QAbstractSpinBox::NoButtons)return;
    for(auto *parent=spin->parentWidget();parent;parent=parent->parentWidget())if(parent->objectName()=="calculatorPanel")return;
    // Other subclasses, e.g. dates, have distinct section-stepping semantics.
    if(!qobject_cast<QSpinBox*>(spin)&&!qobject_cast<QDoubleSpinBox*>(spin))return;
    new Controller(spin);
}
inline void setActive(QAbstractSpinBox *spin,bool active){
    if(!spin)return;if(active)install(spin);
    if(auto *controller=spin->findChild<QObject*>(QStringLiteral("desktopStepperController"),Qt::FindDirectChildrenOnly))static_cast<Controller*>(controller)->setActive(active);
}
}
