// SPDX-License-Identifier: MIT
#pragma once
#include <QtWidgets>

// Mathematical tools stay inside their owner's client area. This deliberately
// uses a child widget and a scoped event loop, not QDialog's native-window
// modality. File pickers continue to use the platform's native dialog API.
class InWindowDialog : public QDialog {
public:
    explicit InWindowDialog(QWidget *owner)
        : QDialog(clientHost(owner),Qt::Widget),host_(parentWidget()) {
        // QDialog interprets zero constructor flags as Qt::Dialog, so remove
        // that implicit window type explicitly after its construction.
        setWindowFlags(Qt::Widget);
        setObjectName("inWindowDialog");setAutoFillBackground(true);
        setFocusPolicy(Qt::StrongFocus);setAttribute(Qt::WA_DeleteOnClose,false);
        overlay_=new QWidget(host_);overlay_->setObjectName("inWindowModalOverlay");
        overlay_->setAttribute(Qt::WA_NoMousePropagation);overlay_->setAutoFillBackground(true);
        auto overlayPalette=host_->palette();auto shade=overlayPalette.color(QPalette::Window);
        shade.setAlpha(205);overlayPalette.setColor(QPalette::Window,shade);overlay_->setPalette(overlayPalette);overlay_->hide();
        heading_=new QLabel(this);heading_->setObjectName("inWindowDialogTitle");heading_->setWordWrap(true);
        heading_->setTextFormat(Qt::PlainText);auto titleFont=font();titleFont.setBold(true);heading_->setFont(titleFont);
        heading_->setAttribute(Qt::WA_TransparentForMouseEvents);
        setPalette(host_->palette());
    }
    ~InWindowDialog() override {
        qApp->removeEventFilter(this);if(loop_)loop_->quit();
        if(active()==this)activeStorage()=previousDialog_;
        delete overlay_;
    }
    void setWindowTitle(const QString &title) {
        QDialog::setWindowTitle(title);heading_->setText(title);setAccessibleName(title);
        if(running_)arrange();
    }
    static InWindowDialog *active(){return activeStorage().data();}
    int exec() override {
        if(running_||!host_)return Rejected;
        running_=true;completed_=false;setResult(Rejected);previousFocus_=QApplication::focusWidget();
        previousDialog_=active();activeStorage()=this;preferredSize_=size();
        if(layout()){bodyMargins_=layout()->contentsMargins();layout()->setSizeConstraint(QLayout::SetNoConstraint);}
        setMinimumSize(0,0);qApp->installEventFilter(this);arrange();
        overlay_->show();overlay_->raise();show();raise();focusFirst();
        QEventLoop loop;loop_=&loop;if(!completed_)loop.exec(QEventLoop::DialogExec);loop_=nullptr;
        qApp->removeEventFilter(this);hide();overlay_->hide();running_=false;
        if(active()==this)activeStorage()=previousDialog_;
        if(previousFocus_&&previousFocus_->isVisible()&&previousFocus_->isEnabled())previousFocus_->setFocus(Qt::OtherFocusReason);
        return result();
    }
    void done(int value) override {
        if(completed_)return;completed_=true;setResult(value);hide();if(overlay_)overlay_->hide();
        if(loop_)loop_->quit();emit finished(value);if(value==Accepted)emit accepted();else if(value==Rejected)emit rejected();
    }
    void accept() override{done(Accepted);}
    void reject() override{done(Rejected);}
protected:
    bool eventFilter(QObject *watched,QEvent *event) override {
        if(!running_)return false;
        if(watched==host_){if(event->type()==QEvent::Resize)arrange();else if(event->type()==QEvent::Close||event->type()==QEvent::Hide)reject();}
        if(active()!=this)return false;
        if(belongsTo(watched,this)){
            // The main window also has an Escape shortcut. Reserve the key
            // here so QDialog (or an open combo popup) receives its key press.
            if(event->type()==QEvent::ShortcutOverride){const auto *key=static_cast<QKeyEvent*>(event);if(key->key()==Qt::Key_Escape&&key->modifiers()==Qt::NoModifier){event->accept();return true;}}
            return false;
        }
        // Keep combo popups and child menus usable; they retain an QObject
        // ancestry inside this card even if Qt gives their popup a window.
        const auto widget=qobject_cast<QWidget*>(watched);
        const bool sameOwner=widget?widget->window()==host_->window():belongsTo(watched,host_->window());
        if(!sameOwner)return false;
        switch(event->type()) {
        case QEvent::KeyPress:case QEvent::KeyRelease:case QEvent::Shortcut:case QEvent::ShortcutOverride:
        case QEvent::MouseButtonPress:case QEvent::MouseButtonRelease:case QEvent::MouseButtonDblClick:
        case QEvent::Wheel:case QEvent::TouchBegin:case QEvent::TouchUpdate:case QEvent::TouchEnd:
            event->accept();return true;
        case QEvent::FocusIn:
            QTimer::singleShot(0,this,[this]{if(running_&&active()==this)focusFirst();});break;
        default:break;
        }
        return false;
    }
    bool focusNextPrevChild(bool next) override {
        QWidget *candidate=QApplication::focusWidget();if(!candidate||!belongsTo(candidate,this))candidate=this;
        QWidget *start=candidate;
        do {candidate=next?candidate->nextInFocusChain():candidate->previousInFocusChain();
            if(candidate!=this&&belongsTo(candidate,this)&&candidate->isVisible()&&candidate->isEnabled()&&(candidate->focusPolicy()&Qt::TabFocus)){
                candidate->setFocus(next?Qt::TabFocusReason:Qt::BacktabFocusReason);return true;
            }
        }while(candidate&&candidate!=start);
        setFocus();return true;
    }
    void resizeEvent(QResizeEvent *event) override {QDialog::resizeEvent(event);arrangeHeading();}
    void paintEvent(QPaintEvent *event) override {QDialog::paintEvent(event);QPainter painter(this);painter.setPen(palette().color(QPalette::Mid));painter.drawRect(rect().adjusted(0,0,-1,-1));}
private:
    static QWidget *clientHost(QWidget *owner){QWidget *host=owner->window();if(auto *main=qobject_cast<QMainWindow*>(host))if(main->centralWidget())host=main->centralWidget();return host;}
    static QPointer<InWindowDialog>&activeStorage(){static QPointer<InWindowDialog> value;return value;}
    static bool belongsTo(QObject *object,const QObject *ancestor){for(;object;object=object->parent())if(object==ancestor)return true;return false;}
    void focusFirst(){if(auto *focused=QApplication::focusWidget())if(belongsTo(focused,this))return;setFocus();focusNextPrevChild(true);}
    void arrangeHeading(){
        if(!heading_)return;const int titleHeight=heading_->text().isEmpty()?0:QFontMetrics(heading_->font()).boundingRect(QRect(0,0,qMax(20,width()-24),10000),Qt::TextWordWrap,heading_->text()).height()+16;
        heading_->setGeometry(12,8,qMax(20,width()-24),qMax(0,titleHeight-16));heading_->setVisible(titleHeight>0);
        if(layout()&&running_)layout()->setContentsMargins(bodyMargins_.left(),bodyMargins_.top()+titleHeight,bodyMargins_.right(),bodyMargins_.bottom());
    }
    void arrange(){
        if(!host_)return;overlay_->setGeometry(host_->rect());const auto available=host_->size()-QSize(24,24);
        const QSize target(qMax(1,qMin(qMin(preferredSize_.width(),maximumWidth()),available.width())),qMax(1,qMin(qMin(preferredSize_.height(),maximumHeight()),available.height())));
        setGeometry((host_->width()-target.width())/2,(host_->height()-target.height())/2,target.width(),target.height());arrangeHeading();
    }
    QPointer<QWidget> host_,overlay_,previousFocus_;QPointer<InWindowDialog> previousDialog_;
    QLabel *heading_=nullptr;QEventLoop *loop_=nullptr;QSize preferredSize_;QMargins bodyMargins_;
    bool running_=false,completed_=false;
};
