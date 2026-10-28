// SPDX-License-Identifier: MIT
#include "workspacehub.h"
#include "desktopworkbench.h"
#include <QPainterPath>
namespace {
struct Destination {const char*id;const char*title;const char*category;};
const Destination destinations[]={
 {"calculate","Calculate","calculate"},{"details","Details","calculate"},{"guided","Algebra / Calculus","calculate"},{"steps","Steps","calculate"},{"graphs","Graph / Table","calculate"},
 {"matrix","Matrix","tools"},{"data","Data / Statistics","tools"},{"finance","Finance","tools"},{"programmer","Programmer","tools"},{"toolbox","Toolbox","tools"},
 {"history","History","saved"},{"definitions","Definitions","saved"},{"export","Export workspace","saved"},{"import","Import workspace","saved"},
 {"workflows","Input modes","personal"},{"keypad","Keypad designer","personal"},{"settings","Settings","personal"},{"guide","Guide","personal"}
};
struct Category{const char*id;const char*title;QStringList hints;};
const Category categories[]={
 {"calculate","Calculate & explore",{"Calculate","Details","Algebra / Calculus","Steps","Graph / Table"}},
 {"tools","Data & tools",{"Matrix","Data / Statistics","Finance","Programmer"}},
 {"saved","Saved workspace",{"History","Definitions","Export workspace","Import workspace"}},
 {"personal","Personalize",{"Input modes","Keypad designer","Settings","Guide"}}
};
class NavigationSwitch final:public QPushButton {
public:
 bool calculator=false;
 explicit NavigationSwitch(const QString &text):QPushButton(text){setMinimumWidth(0);setProperty("twRole","secondary");}
 QSize minimumSizeHint()const override{return {0,DesktopWorkbench::controlHeight(this)};}
 QSize sizeHint()const override{const int width=calculator?fontMetrics().horizontalAdvance(text())+44:160;return {width,heightForWidth(width)};}
 bool hasHeightForWidth()const override{return true;}
 int heightForWidth(int width)const override{if(!calculator)return qMax(DesktopWorkbench::controlHeight(this),DesktopWorkbench::wrappedHeight(text(),font(),qMax(20,width-28))+12);QStyleOptionButton option;initStyleOption(&option);option.rect=QRect(0,0,width,100);const auto area=style()->subElementRect(QStyle::SE_PushButtonContents,&option,this).adjusted(8,2,-8,-2);return qMax(DesktopWorkbench::controlHeight(this),DesktopWorkbench::wrappedHeight(text(),font(),qMax(20,area.width()))+100-area.height()+2);}
protected:
 void paintEvent(QPaintEvent*)override{QStyleOptionButton option;initStyleOption(&option);const auto caption=option.text;option.text.clear();QStylePainter painter(this);painter.drawControl(QStyle::CE_PushButton,option);const auto area=style()->subElementRect(QStyle::SE_PushButtonContents,&option,this).adjusted(8,2,-8,-2);const auto *scope=DesktopWorkbench::scopeFor(this);const auto palette=scope->property("twPalette").isValid()?qvariant_cast<QPalette>(scope->property("twPalette")):scope->palette();DesktopWorkbench::drawWrapped(painter,area,caption,font(),palette.color(isEnabled()?QPalette::Active:QPalette::Disabled,QPalette::ButtonText),Qt::AlignCenter);}
 void keyPressEvent(QKeyEvent *event)override{if(event->modifiers()==Qt::NoModifier&&(event->key()==Qt::Key_Return||event->key()==Qt::Key_Enter)){click();event->accept();return;}QPushButton::keyPressEvent(event);}
};
class NavigationCard final:public QPushButton {
public:
 QString subtitle,iconKind;
 bool compact=false,calculator=false,listRow=false;
 explicit NavigationCard(QWidget *parent=nullptr):QPushButton(parent){setProperty("twRole","navigation");setMinimumWidth(0);setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);setCursor(Qt::PointingHandCursor);}
 QSize minimumSizeHint()const override{return calculator?QSize(72,listRow?48:compact?72:76):QSize(80,compact?80:(subtitle.isEmpty()?76:104));}
 QSize sizeHint()const override{return {compact?170:310,heightForWidth(compact?170:310)};}
 bool hasHeightForWidth()const override{return true;}
 int heightForWidth(int width)const override{
  if(calculator){QFont titleFont=font();titleFont.setWeight(QFont::DemiBold);const int available=qMax(28,width-(compact?24:72));const int titleHeight=DesktopWorkbench::wrappedHeight(text(),titleFont,available);if(listRow)return qMax(50,titleHeight+24);if(compact)return qMax(74,titleHeight+46);const int details=subtitle.isEmpty()?0:DesktopWorkbench::wrappedHeight(subtitle,font(),available);return qMax(78,titleHeight+details+(details?30:24));}
  const int available=qMax(28,width-(compact?28:98));QFont titleFont=DesktopWorkbench::enlarged(this,compact?1.:1.12);titleFont.setWeight(QFont::DemiBold);
  const int titleHeight=DesktopWorkbench::wrappedHeight(text(),titleFont,available);
  if(compact)return qMax(88,titleHeight+58);
  const int detailHeight=subtitle.isEmpty()?0:DesktopWorkbench::wrappedHeight(subtitle,font(),available);
  return qMax(subtitle.isEmpty()?78:112,30+titleHeight+(detailHeight?8+detailHeight:0));
 }
protected:
 void paintEvent(QPaintEvent*)override{
  if(calculator){paintCalculator();return;}
  QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing);const auto*scope=DesktopWorkbench::scopeFor(this);const auto c=DesktopWorkbench::colors(scope->property("twPalette").isValid()?qvariant_cast<QPalette>(scope->property("twPalette")):scope->palette());
  painter.setPen(QPen(hasFocus()?c.accent:c.border,hasFocus()?2:1));painter.setBrush(isDown()?c.raised:c.surface);painter.drawRoundedRect(QRectF(rect()).adjusted(1,1,-1,-1),12,12);
  const QRectF icon(compact?14:16,compact?13:(height()-42)/2.,compact?26:42,compact?26:42);QColor badge=c.accent;badge.setAlpha(c.dark?35:20);painter.setPen(Qt::NoPen);painter.setBrush(badge);painter.drawRoundedRect(icon.adjusted(-4,-4,4,4),8,8);
  painter.setPen(QPen(c.accent,1.8,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));painter.setBrush(Qt::NoBrush);const QRectF ink=icon.adjusted(compact?3:9,compact?3:9,compact?-3:-9,compact?-3:-9);
  if(iconKind=="calculate"){painter.drawRoundedRect(ink,3,3);painter.drawLine(QPointF(ink.left()+4,ink.top()+6),QPointF(ink.right()-4,ink.top()+6));for(int row=0;row<2;++row)for(int col=0;col<2;++col)painter.drawPoint(QPointF(ink.left()+6+col*10,ink.top()+13+row*6));}
  else if(iconKind=="tools"){for(int i=0;i<3;++i){painter.drawLine(QPointF(ink.left()+i*ink.width()/2,ink.top()),QPointF(ink.left()+i*ink.width()/2,ink.bottom()));painter.drawLine(QPointF(ink.left(),ink.top()+i*ink.height()/2),QPointF(ink.right(),ink.top()+i*ink.height()/2));}}
  else if(iconKind=="saved"){painter.drawRoundedRect(ink,3,3);for(int i=0;i<3;++i)painter.drawLine(QPointF(ink.left()+5,ink.top()+6+i*6),QPointF(ink.right()-5,ink.top()+6+i*6));}
  else {for(int i=0;i<3;++i){const double y=ink.top()+4+i*8;const double x=ink.left()+5+(i%2)*10;painter.drawLine(QPointF(ink.left(),y),QPointF(ink.right(),y));painter.setBrush(c.surface);painter.drawEllipse(QPointF(x,y),2.7,2.7);}}
  QFont titleFont=DesktopWorkbench::enlarged(this,compact?1.:1.12);titleFont.setWeight(QFont::DemiBold);painter.setFont(titleFont);painter.setPen(c.text);
  const int left=compact?14:76;const int top=compact?48:(subtitle.isEmpty()?qMax(14,(height()-DesktopWorkbench::wrappedHeight(text(),titleFont,qMax(20,width()-left-22)))/2):16);const int available=qMax(20,width()-left-(compact?14:22));
  const int titleHeight=DesktopWorkbench::wrappedHeight(text(),titleFont,available);DesktopWorkbench::drawWrapped(painter,QRect(left,top,available,titleHeight+2),text(),titleFont,c.text,Qt::AlignLeft|Qt::AlignVCenter);
  if(!compact&&!subtitle.isEmpty()){painter.setFont(font());painter.setPen(c.muted);DesktopWorkbench::drawWrapped(painter,QRect(left,top+titleHeight+8,available,qMax(16,height()-top-titleHeight-20)),subtitle,font(),c.muted);}
  if(!compact){painter.setPen(QPen(c.muted,1.8,Qt::SolidLine,Qt::RoundCap));const QPointF center(width()-15,height()/2.);painter.drawLine(center+QPointF(-3,-5),center+QPointF(2,0));painter.drawLine(center+QPointF(2,0),center+QPointF(-3,5));}
 }
 void focusInEvent(QFocusEvent*event)override{QPushButton::focusInEvent(event);if(calculator)for(auto *p=parentWidget();p;p=p->parentWidget())if(auto *scroll=qobject_cast<QScrollArea*>(p)){scroll->ensureWidgetVisible(this,8,8);break;}update();}
 void keyPressEvent(QKeyEvent*event)override{
  if(calculator&&event->modifiers()==Qt::NoModifier){
   if(event->key()==Qt::Key_Return||event->key()==Qt::Key_Enter){click();event->accept();return;}
   const int key=event->key();if(key==Qt::Key_Up||key==Qt::Key_Down||key==Qt::Key_Left||key==Qt::Key_Right){NavigationCard*next=nullptr;double best=1e20;const auto here=geometry().center();for(auto *widget:parentWidget()->findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly))if(auto *card=dynamic_cast<NavigationCard*>(widget);card&&card!=this&&card->isVisibleTo(parentWidget())&&card->isEnabled()){const auto delta=card->geometry().center()-here;const double primary=(key==Qt::Key_Up?-delta.y():key==Qt::Key_Down?delta.y():key==Qt::Key_Left?-delta.x():delta.x());const double secondary=(key==Qt::Key_Up||key==Qt::Key_Down)?std::abs(delta.x()):std::abs(delta.y());if(primary>0&&primary+4*secondary<best){best=primary+4*secondary;next=card;}}if(next)next->setFocus(Qt::TabFocusReason);event->accept();return;}
  }
  QPushButton::keyPressEvent(event);
 }
private:
 void paintCalculator(){
  QPainter p(this);p.setRenderHint(QPainter::Antialiasing);const auto *scope=DesktopWorkbench::scopeFor(this);const auto palette=scope->property("twPalette").isValid()?qvariant_cast<QPalette>(scope->property("twPalette")):scope->palette();const auto colors=DesktopWorkbench::colors(palette);
  QColor background=colors.raised;if(isDown())background=colors.background;else if(underMouse())background=QColor::fromRgbF(background.redF()*.92+colors.backgroundText.redF()*.08,background.greenF()*.92+colors.backgroundText.greenF()*.08,background.blueF()*.92+colors.backgroundText.blueF()*.08);
  const QColor ink=DesktopWorkbench::readableInk(colors.buttonText,background),accent=DesktopWorkbench::accentInk(colors.accent,background);const QColor muted=DesktopWorkbench::readableInk(QColor::fromRgbF(ink.redF()*.8+background.redF()*.2,ink.greenF()*.8+background.greenF()*.2,ink.blueF()*.8+background.blueF()*.2),background);
  p.setPen(QPen(hasFocus()?accent:colors.border,hasFocus()?2.:1.));p.setBrush(background);p.drawRoundedRect(QRectF(rect()).adjusted(1,1,-1,-1),8,8);
  const QRectF icon(compact?12:13,compact?11:(height()-24)/2.,24,24);p.save();p.translate(icon.topLeft());p.scale(icon.width()/24.,icon.height()/24.);p.setPen(QPen(accent,1.7,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));p.setBrush(Qt::NoBrush);
  const auto id=property("destination").toString();const auto kind=id.startsWith("category:")?id.mid(9):id;
  if(kind=="graphs"){p.drawLine(3,3,3,21);p.drawLine(3,21,22,21);QPainterPath path;path.moveTo(5,16);path.cubicTo(9,0,13,22,21,5);p.drawPath(path);}
  else if(kind=="details"){p.drawEllipse(QRectF(3,3,18,18));p.drawPoint(12,8);p.drawLine(12,11,12,17);}
  else if(kind=="history"){p.drawEllipse(QRectF(3,3,18,18));p.drawLine(12,6,12,12);p.drawLine(12,12,17,15);}
  else if(kind=="import"||kind=="export"){p.drawLine(4,16,4,21);p.drawLine(4,21,20,21);p.drawLine(20,21,20,16);p.drawLine(12,3,12,16);const int y=kind=="import"?16:3,dy=kind=="import"?-4:4;p.drawLine(8,y+dy,12,y);p.drawLine(12,y,16,y+dy);}
  else if(kind=="data"){p.drawLine(3,21,22,21);for(int i=0;i<3;++i)p.drawRoundedRect(QRectF(5+i*6,14-i*4,3,7+i*4),1,1);}
  else if(kind=="matrix"||kind=="tools"||kind=="keypad"){p.drawRoundedRect(QRectF(3,3,18,18),2,2);for(int i=1;i<3;++i){p.drawLine(3+6*i,3,3+6*i,21);p.drawLine(3,3+6*i,21,3+6*i);}}
  else if(kind=="finance"){p.drawEllipse(QRectF(3,3,18,18));p.drawLine(8,17,16,7);p.drawEllipse(QRectF(7,6,3,3));p.drawEllipse(QRectF(14,15,3,3));}
  else if(kind=="steps"){for(int i=0;i<3;++i){p.drawPoint(4,6+i*6);p.drawLine(9,6+i*6,21,6+i*6);}}
  else if(kind=="definitions"||kind=="guide"){p.drawRoundedRect(QRectF(4,3,17,18),2,2);p.drawLine(8,3,8,21);p.drawLine(11,8,17,8);p.drawLine(11,12,17,12);}
  else if(kind=="saved"){p.drawRoundedRect(QRectF(4,3,16,18),2,2);for(int i=0;i<3;++i)p.drawLine(8,7+i*5,16,7+i*5);}
  else if(kind=="programmer"){p.drawLine(8,5,3,12);p.drawLine(3,12,8,19);p.drawLine(16,5,21,12);p.drawLine(21,12,16,19);p.drawLine(14,4,10,20);}
  else if(kind=="guided"){QPainterPath path;path.moveTo(8,3);path.cubicTo(2,3,8,10,3,12);path.cubicTo(8,14,2,21,8,21);p.drawPath(path);p.translate(24,0);p.scale(-1,1);p.drawPath(path);}
  else if(kind=="personal"||kind=="settings"||kind=="workflows"){for(int i=0;i<3;++i){const int y=6+i*6,x=7+(i%2)*10;p.drawLine(3,y,21,y);p.setBrush(background);p.drawEllipse(QPointF(x,y),2.3,2.3);}}
  else{p.drawRoundedRect(QRectF(4,2,16,20),2,2);p.drawLine(7,7,17,7);for(int r=0;r<2;++r)for(int c=0;c<3;++c)p.drawPoint(7+5*c,12+5*r);}
  p.restore();QFont titleFont=font();titleFont.setWeight(QFont::DemiBold);const int left=compact?12:50,available=qMax(24,width()-left-(compact?12:22));const int titleHeight=DesktopWorkbench::wrappedHeight(text(),titleFont,available);const int details=compact||listRow||subtitle.isEmpty()?0:DesktopWorkbench::wrappedHeight(subtitle,font(),available);const int top=compact?42:qMax(10,(height()-titleHeight-details-(details?6:0))/2);
  DesktopWorkbench::drawWrapped(p,QRect(left,top,available,titleHeight+2),text(),titleFont,ink,Qt::AlignLeft|Qt::AlignVCenter);if(details)DesktopWorkbench::drawWrapped(p,QRect(left,top+titleHeight+6,available,details+2),subtitle,font(),muted);
  if(!compact){p.setPen(QPen(muted,1.6,Qt::SolidLine,Qt::RoundCap));const QPointF c(width()-14,height()/2.);p.drawLine(c+QPointF(-2,-4),c+QPointF(2,0));p.drawLine(c+QPointF(2,0),c+QPointF(-2,4));}
 }
};
void emptyGrid(QGridLayout *layout){while(auto *item=layout->takeAt(0))delete item;}
}
WorkspaceHub::WorkspaceHub(QWidget *parent):QWidget(parent){
 setObjectName("workspaceHub");auto *layout=new QVBoxLayout(this);layout->setContentsMargins(0,0,0,0);layout->setSpacing(0);
 destination_=new QComboBox(this);destination_->setObjectName("workspaceDestination");destination_->hide();pages_=new QStackedWidget;pages_->setMinimumSize(0,0);layout->addWidget(pages_);
 auto add=[this](const QString&id,const char*title,QWidget *page){indices_[id]=pages_->addWidget(page);titles_[id]=QString::fromUtf8(title);destination_->addItem(QString::fromUtf8(title),id);};
 homeBody_=new QWidget;homeBody_->setObjectName("workspaceHomeBody");auto *homeLayout=new QVBoxLayout(homeBody_);homeLayout->setContentsMargins(16,16,16,20);homeLayout->setSpacing(16);auto *heading=new QHBoxLayout;homeHint_=DesktopWorkbench::sectionTitle("Choose a category");heading->addWidget(homeHint_,1);menuSwitch_=new NavigationSwitch("Quick access");menuSwitch_->setObjectName("workspaceMenuLayout");menuSwitch_->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);heading->addWidget(menuSwitch_);homeLayout->addLayout(heading);homeGrid_=new QGridLayout;homeGrid_->setContentsMargins(0,0,0,0);homeGrid_->setSpacing(12);homeLayout->addLayout(homeGrid_);homeLayout->addStretch();add("home","Menu",DesktopWorkbench::scroll(homeBody_));
 categoryBody_=new QWidget;categoryBody_->setObjectName("workspaceCategoryBody");auto *categoryLayout=new QVBoxLayout(categoryBody_);categoryLayout->setContentsMargins(16,16,16,20);categoryLayout->setSpacing(14);categoryHint_=new QLabel;categoryHint_->setProperty("twRole","muted");categoryHint_->setWordWrap(true);categoryLayout->addWidget(categoryHint_);categoryGrid_=new QGridLayout;categoryGrid_->setContentsMargins(0,0,0,0);categoryGrid_->setSpacing(12);categoryLayout->addLayout(categoryGrid_);categoryLayout->addStretch();const int categoryIndex=pages_->addWidget(DesktopWorkbench::scroll(categoryBody_));
 for(const auto &category:categories){const QString id="category:"+QString::fromLatin1(category.id);indices_[id]=categoryIndex;titles_[id]=QString::fromUtf8(category.title);destination_->addItem(category.title,id);auto *card=new NavigationCard(homeBody_);card->setObjectName("category_"+QString::fromLatin1(category.id));card->setProperty("destination",id);card->setProperty("caption",category.title);card->setProperty("hints",category.hints);card->iconKind=category.id;categoryCards_.append(card);connect(card,&QPushButton::clicked,this,[this,id]{open(id);});}
 auto activate=[this](const QString&id){if(id=="export"){if(onExport)onExport();}else if(id=="import"){if(onImport)onImport();}else open(id);};
 for(const auto &entry:destinations){for(bool quick:{false,true}){auto *card=new NavigationCard(quick?homeBody_:categoryBody_);card->compact=quick;card->listRow=!quick;card->iconKind=entry.category;card->setObjectName((quick?"quick_":"destination_")+QString::fromLatin1(entry.id));card->setProperty("destination",entry.id);card->setProperty("caption",entry.title);card->setProperty("category",entry.category);(quick?quickCards_:toolCards_).append(card);connect(card,&QPushButton::clicked,this,[activate,id=QString::fromLatin1(entry.id)]{activate(id);});}}
 details=new ResultDetailsPanel;guided=new GuidedWorkbench;definitions=new DefinitionsPanel;graphs=new GraphWorkbench;steps=new StepsPanel;matrix=new MatrixWorkbench;data=new DataWorkbench;workflows=new InputWorkflowPanel;programmer=new ProgrammerPanel;keypad=new KeypadDesigner;
 add("details","Details",details);add("steps","Steps",steps);add("graphs","Graph / Table",graphs);add("guided","Algebra / Calculus",guided);add("matrix","Matrix",matrix);add("data","Data / Statistics",data);add("definitions","Definitions",definitions);add("workflows","Input modes",workflows);add("programmer","Programmer",programmer);add("keypad","Keypad designer",keypad);
 connect(menuSwitch_,&QPushButton::clicked,this,[this]{setMenuLayout(menuLayout_=="large"?"quick":"large");});connect(destination_,&QComboBox::currentIndexChanged,this,[this](int i){if(i>=0)open(destination_->itemData(i).toString());});
 DesktopWorkbench::apply(this);updateNavigation();showDestination("home",false);
 // Draft persistence includes edits that have not been submitted to the worker.
 auto changed=[this]{if(onChanged)onChanged();};
 for(auto *edit:findChildren<QLineEdit*>())connect(edit,&QLineEdit::textChanged,this,changed);
 for(auto *edit:findChildren<QPlainTextEdit*>())connect(edit,&QPlainTextEdit::textChanged,this,changed);
 for(auto *combo:findChildren<QComboBox*>())connect(combo,&QComboBox::currentIndexChanged,this,changed);
 for(auto *check:findChildren<QCheckBox*>())connect(check,&QCheckBox::toggled,this,changed);

}
QString WorkspaceHub::text(const char *key)const{return translator_?translator_(key):QString::fromUtf8(key);}
QString WorkspaceHub::currentTitle()const{return text(titles_.value(currentDestination_,"Menu").toUtf8().constData());}
QString WorkspaceHub::categoryFor(const QString &id)const{for(const auto &entry:destinations)if(id==entry.id)return "category:"+QString::fromLatin1(entry.category);return {};}
void WorkspaceHub::showDestination(const QString&id,bool notify){
 if(!indices_.contains(id))return;const bool changed=currentDestination_!=id;auto *focused=QApplication::focusWidget();const bool ownedFocus=focused&&pages_->currentWidget()&&(focused==pages_->currentWidget()||pages_->currentWidget()->isAncestorOf(focused));if(changed&&ownedFocus)navigationFocus_[currentDestination_]=focused;
 currentDestination_=id;pages_->setCurrentIndex(indices_.value(id));{QSignalBlocker blocker(destination_);destination_->setCurrentIndex(destination_->findData(id));}arrangeNavigation();
 if(calculatorMode_&&changed&&ownedFocus&&isVisible()){
  auto *remembered=navigationFocus_.value(id).data();if(remembered&&remembered->isVisibleTo(this))remembered->setFocus(Qt::OtherFocusReason);
  else if(id=="home"||id.startsWith("category:")){const auto &cards=id=="home"?(menuLayout_=="large"?categoryCards_:quickCards_):toolCards_;for(auto *card:cards)if(card->isVisibleTo(this)){card->setFocus(Qt::OtherFocusReason);break;}}
 }
 if(changed&&notify){if(onChanged)onChanged();if(onPageChanged)onPageChanged();}
}
void WorkspaceHub::open(QString id){if(indices_.contains(id))showDestination(id);else if(onNavigate)onNavigate(id);}
void WorkspaceHub::setMenuLayout(QString layout){layout=layout=="quick"?"quick":"large";if(menuLayout_==layout)return;menuLayout_=layout;homeColumns_=0;updateNavigation();if(onMenuLayoutChanged)onMenuLayoutChanged(layout);if(onChanged)onChanged();if(onPageChanged)onPageChanged();}
void WorkspaceHub::setCalculatorMode(bool calculator){
 if(calculatorMode_==calculator)return;calculatorMode_=calculator;
 static_cast<NavigationSwitch*>(menuSwitch_)->calculator=calculator;menuSwitch_->updateGeometry();
 for(const auto &cards:{categoryCards_,quickCards_,toolCards_})for(auto *button:cards){auto *card=static_cast<NavigationCard*>(button);card->calculator=calculator;card->updateGeometry();card->update();}
 for(auto *body:{homeBody_,categoryBody_})if(auto *layout=qobject_cast<QVBoxLayout*>(body->layout())){layout->setContentsMargins(calculator?12:16,calculator?10:16,calculator?12:16,calculator?14:20);layout->setSpacing(calculator?10:(body==homeBody_?16:14));}
 homeGrid_->setSpacing(calculator?8:12);categoryGrid_->setSpacing(calculator?6:12);menuSwitch_->setSizePolicy(calculator?QSizePolicy::Fixed:QSizePolicy::Preferred,QSizePolicy::Preferred);menuSwitch_->setMaximumWidth(calculator?qMax(88,menuSwitch_->fontMetrics().horizontalAdvance(menuSwitch_->text())+44):QWIDGETSIZE_MAX);
 homeColumns_=categoryColumns_=0;categoryLayoutId_.clear();updateNavigation();
}
bool WorkspaceHub::goBack(){if(currentDestination_=="home")return false;if(currentDestination_.startsWith("category:")||menuLayout_=="quick")showDestination("home");else{const auto parent=categoryFor(currentDestination_);showDestination(parent.isEmpty()?QStringLiteral("home"):parent);}return true;}
void WorkspaceHub::updateNavigation(){
 const auto homeHint=text(menuLayout_=="large"?"Choose a category":"Quick access");if(homeHint_->text()!=homeHint)homeHint_->setText(homeHint);
 const auto menuSwitch=text(menuLayout_=="large"?"Quick access":"Large buttons");if(menuSwitch_->text()!=menuSwitch)menuSwitch_->setText(menuSwitch);
 const auto menuName=text("Menu layout")+": "+menuSwitch;if(menuSwitch_->accessibleName()!=menuName)menuSwitch_->setAccessibleName(menuName);
 const auto categoryHint=text("Choose a tool");if(categoryHint_->text()!=categoryHint)categoryHint_->setText(categoryHint);
 for(auto *button:categoryCards_){auto *card=static_cast<NavigationCard*>(button);const auto caption=text(card->property("caption").toString().toUtf8().constData());if(card->text()!=caption)card->setText(caption);QStringList hints;for(const auto&hint:card->property("hints").toStringList())hints<<text(hint.toUtf8().constData());QStringList summary=hints;if(calculatorMode_){const auto id=card->property("destination").toString();if(id=="category:calculate")summary={text("Algebra / Calculus"),text("Graph / Table")};else if(id=="category:tools")summary={text("Matrix"),text("Data / Statistics"),text("Finance")};else if(id=="category:saved")summary={text("History"),text("Definitions")};else summary={text("Input modes"),text("Settings"),text("Guide")};}const auto subtitle=summary.join(QStringLiteral(" · "));if(card->subtitle!=subtitle){card->subtitle=subtitle;card->updateGeometry();card->update();}if(card->accessibleName()!=caption)card->setAccessibleName(caption);const auto description=hints.join(QStringLiteral(" · "));if(card->accessibleDescription()!=description)card->setAccessibleDescription(description);}
 for(const auto &list:{quickCards_,toolCards_})for(auto *card:list){const auto caption=text(card->property("caption").toString().toUtf8().constData());if(card->text()!=caption)card->setText(caption);if(card->accessibleName()!=caption)card->setAccessibleName(caption);}
 arrangeNavigation();
}
void WorkspaceHub::arrangeNavigation(){
 if(calculatorMode_)menuSwitch_->setMaximumWidth(qMin(qMax(112,(width()-32)/2),qMax(88,menuSwitch_->fontMetrics().horizontalAdvance(menuSwitch_->text())+44)));
 const int homeWidth=qMax(1,width()-(calculatorMode_?32:40));
 const int cardWidth=calculatorMode_?qMax(124,fontMetrics().horizontalAdvance(QStringLiteral("MMMMMM"))+24):qMax(160,fontMetrics().horizontalAdvance(QStringLiteral("MMMMMMMM"))+28);
 const int columns=menuLayout_=="large"?(homeWidth>=qMax(610,cardWidth*3)?2:1):qBound(1,(homeWidth+12)/(cardWidth+12),4);
 // Switching tools does not alter home membership. Reuse its layout items until
 // the selected menu layout or responsive column count actually changes.
 if(homeColumns_!=columns){emptyGrid(homeGrid_);for(auto *card:categoryCards_)card->setVisible(menuLayout_=="large");for(auto *card:quickCards_)card->setVisible(menuLayout_=="quick");const auto &cards=menuLayout_=="large"?categoryCards_:quickCards_;for(int i=0;i<cards.size();++i)homeGrid_->addWidget(cards[i],i/columns,i%columns);for(int i=0;i<4;++i)homeGrid_->setColumnStretch(i,i<columns?1:0);homeColumns_=columns;}
 // Keep the last category ready while its tool is open. Clearing that hidden
 // grid on every navigation would also force rebuilding it on every Back tap.
 if(!currentDestination_.startsWith("category:"))return;
 const int categoryWidth=qMax(1,width()-(calculatorMode_?32:40));const int categoryColumns=categoryWidth>=(calculatorMode_?600:640)?2:1;
 if(categoryColumns_==categoryColumns&&categoryLayoutId_==currentDestination_)return;
 emptyGrid(categoryGrid_);int i=0;const auto selected=currentDestination_.mid(QStringLiteral("category:").size());for(auto *card:toolCards_){const bool visible=card->property("category").toString()==selected;card->setVisible(visible);if(visible){categoryGrid_->addWidget(card,i/categoryColumns,i%categoryColumns);++i;}}categoryGrid_->setColumnStretch(0,1);categoryGrid_->setColumnStretch(1,categoryColumns==2?1:0);categoryColumns_=categoryColumns;categoryLayoutId_=currentDestination_;
}
void WorkspaceHub::resizeEvent(QResizeEvent *event){QWidget::resizeEvent(event);arrangeNavigation();}
void WorkspaceHub::changeEvent(QEvent *event){QWidget::changeEvent(event);if(event->type()==QEvent::FontChange&&homeGrid_){arrangeNavigation();for(const auto &cards:{categoryCards_,quickCards_,toolCards_})for(auto *card:cards){card->updateGeometry();card->update();}}}
void WorkspaceHub::setResult(const CalcResult&r){details->setResult(r);if(r.ok)steps->setExpression(r.context&&!r.context->frozenExpression.isEmpty()?r.context->frozenExpression:r.input);}
void WorkspaceHub::retranslate(std::function<QString(const char*)>translate){translator_=std::move(translate);destination_->setAccessibleName(text("Workspace destination"));{QSignalBlocker blocker(destination_);for(int i=0;i<destination_->count();++i)destination_->setItemText(i,text(titles_.value(destination_->itemData(i).toString()).toUtf8().constData()));}updateNavigation();details->retranslate(translator_);guided->retranslate(translator_);definitions->retranslate(translator_);matrix->retranslate(translator_);data->retranslate(translator_);workflows->retranslate(translator_);programmer->retranslate(translator_);keypad->retranslate(translator_);graphs->retranslate(translator_);steps->retranslate(translator_);if(onPageChanged)onPageChanged();}
QJsonObject WorkspaceHub::saveState()const{return{{"schema",2},{"destination",currentDestination_},{"graph",graphs->saveState()},{"matrix",matrix->saveState()},{"data",data->saveState()},{"guided",guided->saveState()},{"workflows",workflows->saveState()},{"keypad",keypad->saveState()},{"programmer",programmer->saveState()},{"definitionsDraft",definitions->saveDraft()},{"stepsDraft",steps->saveDraft()}};}
bool WorkspaceHub::restoreState(const QJsonObject&o,QString*error){
 if(o.isEmpty())return true;
 // Validate every component in detached, callback-free widgets before changing
 // the live workspace. A malformed later section cannot partially import data.
 WorkspaceHub candidate;
 if(!candidate.restoreStateInternal(o,error))return false;
 return restoreStateInternal(o,error);
}
bool WorkspaceHub::restoreStateInternal(const QJsonObject&o,QString*error){if(o.isEmpty())return true;if(o.value("schema").toInt()!=2){if(error)*error="Unsupported workspace schema";return false;}if(!graphs->restoreState(o.value("graph").toObject(),error)||!matrix->restoreState(o.value("matrix").toObject(),error)||!data->restoreState(o.value("data").toObject(),error)||!guided->restoreState(o.value("guided").toObject(),error)||!workflows->restoreState(o.value("workflows").toObject(),error))return false;if(o.contains("keypad")&&!keypad->restoreState(o.value("keypad").toObject(),error))return false;if(o.contains("programmer")&&!programmer->restoreState(o.value("programmer").toObject()))return false;if(!definitions->restoreDraft(o.value("definitionsDraft").toObject(),error)||!steps->restoreDraft(o.value("stepsDraft").toObject(),error))return false;const auto destination=o.value("destination").toString("home");showDestination(indices_.contains(destination)?destination:QStringLiteral("home"),false);if(onPageChanged)onPageChanged();return true;}
void WorkspaceHub::cancelWork(){graphs->cancelWork();matrix->cancel();data->cancel();guided->cancel();steps->cancel();workflows->workflows().cancel();}
