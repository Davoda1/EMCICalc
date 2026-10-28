// SPDX-License-Identifier: MIT
// Actual native Calculator MENU hierarchy, keyboard input and responsive geometry.
#include "mainwindow.h"
#include "workspacehub.h"
#include "fontscale.h"
#include "i18n.h"
#include <QtTest/QTest>
#include <iostream>

class DesktopAcceptanceTests {
 int checks_=0,failures_=0;QString output_;
 void check(bool ok,const QString &message){++checks_;if(!ok)++failures_;qInfo().noquote()<<(ok?"PASS":"FAIL")<<message;}
 void settle(){for(int i=0;i<3;++i){QApplication::processEvents();QCoreApplication::sendPostedEvents(nullptr,QEvent::LayoutRequest);QTest::qWait(4);}}
 QPushButton *card(MainWindow&w,const char *id){return w.workbenchHub_->findChild<QPushButton*>(QString::fromLatin1(id));}
 void capture(MainWindow&w,const QString&name){if(!output_.isEmpty())check(w.grab().save(QDir(output_).filePath(name+".png")),"native menu capture "+name);}
 void geometry(MainWindow&w,const QString&context){
  auto *hub=w.workbenchHub_;bool scrollFits=true,cardsFit=true,overlap=false;QVector<QRect>rectangles;
  for(auto *scroll:hub->findChildren<QScrollArea*>())if(scroll->isVisibleTo(hub)){scrollFits&=scroll->horizontalScrollBar()->maximum()==0&&scroll->widget()->width()<=scroll->viewport()->width();}
  for(auto *button:hub->findChildren<QPushButton*>())if(button->property("destination").isValid()&&button->isVisibleTo(hub)){
   const QRect bounds(button->mapTo(hub,QPoint()),button->size());cardsFit&=bounds.left()>=0&&bounds.right()<hub->width()&&button->height()>=button->heightForWidth(button->width());
   for(const auto &other:rectangles)overlap|=bounds.intersects(other);rectangles<<bounds;
   cardsFit&=!button->accessibleName().isEmpty()&&!button->accessibleName().contains('<');
  }
  check(hub->isVisible()&&hub->width()<=w.width()&&hub->height()<=w.pages_->height(),context+": menu stays in its client host");
  check(scrollFits&&cardsFit&&!overlap&&!rectangles.isEmpty(),context+": readable nonoverlapping cards, local vertical scrolling, accessible names");
  if(auto *switcher=hub->findChild<QPushButton*>("workspaceMenuLayout");switcher&&switcher->isVisibleTo(hub))check(switcher->height()>=switcher->heightForWidth(switcher->width()),context+": layout selector shows its complete wrapped text");
  if(w.auxMenu_->isVisible()){QStyleOptionButton option;option.initFrom(w.auxMenu_);option.text=w.auxMenu_->text();const auto content=w.auxMenu_->style()->subElementRect(QStyle::SE_PushButtonContents,&option,w.auxMenu_);check(content.width()>=w.auxMenu_->fontMetrics().horizontalAdvance(option.text)&&content.height()>=w.auxMenu_->fontMetrics().height(),context+": auxiliary MENU caption fits the native content rectangle");}
 }
public:
 int run(QString output){output_=output;if(!output_.isEmpty())QDir().mkpath(output_);MainWindow w(true);w.show();QElapsedTimer ready;ready.start();while((!w.catalogReady_||!w.registersReady_)&&ready.elapsed()<15000)settle();check(w.catalogReady_&&w.registersReady_,"native worker and catalog ready");
  w.design_="Calculator";w.calculatorTheme_="Classic";w.applyDesign();w.syncSettingsControls();w.resize(320,540);w.showWorkspace("home");settle();
  auto *tools=card(w,"category_tools"),*matrix=card(w,"destination_matrix"),*data=card(w,"destination_data"),*finance=card(w,"destination_finance");check(tools&&matrix&&data&&finance,"stable MENU selector identities");if(!tools||!matrix||!data||!finance)return 1;
  auto *identity=w.workbenchHub_->matrix;const auto initialState=w.workbenchHub_->saveState();tools->setFocus();QTest::keyClick(tools,Qt::Key_Return);settle();check(w.workbenchHub_->currentDestination()=="category:tools"&&matrix->hasFocus(),"Enter opens category and focuses its first visible row");
  QTest::keyClick(matrix,Qt::Key_Down);check(data->hasFocus(),"Down moves to next visible submenu row");QTest::keyClick(data,Qt::Key_Tab);check(finance->hasFocus(),"Tab follows the visible submenu order");QTest::keyClick(finance,Qt::Key_Up);check(data->hasFocus(),"Up remains among visible submenu rows");
  QTest::keyClick(data,Qt::Key_Return);settle();check(w.workbenchHub_->currentDestination()=="data", "Enter activates the focused workbench row");w.goBack();settle();check(w.workbenchHub_->currentDestination()=="category:tools"&&data->hasFocus(),"Back restores category context and selected row focus");
  matrix->setFocus();QTest::keyClick(matrix,Qt::Key_Space);settle();check(w.workbenchHub_->currentDestination()=="matrix","native Space activates a focused row");w.goBack();w.goBack();settle();check(w.workbenchHub_->currentDestination()=="home"&&tools->hasFocus(),"Back restores the original category card");
  geometry(w,"Classic home320");capture(w,"calculator-menu-classic-320");QTest::mouseClick(tools,Qt::LeftButton);settle();geometry(w,"Classic tools320");capture(w,"calculator-submenu-tools-classic-320");w.goBack();w.workbenchHub_->setMenuLayout("quick");settle();
  auto *quick=card(w,"quick_calculate"),*details=card(w,"quick_details");check(quick&&details&&quick->y()==details->y()&&quick->x()!=details->x(),"Quick access uses two columns at320 without shrinking labels");quick->setFocus();QTest::keyClick(quick,Qt::Key_Right);check(details->hasFocus(),"Right moves to next visible quick-access tile");geometry(w,"Quick access320");capture(w,"calculator-menu-quick-320");w.workbenchHub_->setMenuLayout("large");
  QElapsedTimer navigation;navigation.start();for(int i=0;i<100;++i){w.workbenchHub_->open("category:tools");w.workbenchHub_->open("matrix");w.workbenchHub_->open("home");}const auto milliseconds=navigation.elapsed();check(w.workbenchHub_->matrix==identity&&w.workbenchHub_->saveState().value("matrix")==initialState.value("matrix"),"300 menu transitions preserve page identity and drafts");qInfo()<<"MENU TRANSITIONS300"<<milliseconds<<"ms";
  for(const auto &language:languageCodes()){w.language_=language;w.updateTranslations();for(const auto *destination:{"home","category:calculate","category:tools","category:saved","category:personal"}){w.workbenchHub_->open(destination);settle();geometry(w,language+" "+destination);}}
  w.language_="en";w.updateTranslations();w.showWorkspace("home");
  for(const auto &skin:calculatorThemeNames()){w.calculatorTheme_=skin;w.applyTheme();settle();check(w.workbenchHub_->palette().color(QPalette::Window)==calculatorThemeDefinition(skin).body,skin+": menu follows Calculator body palette");geometry(w,skin+" home");}
  w.calculatorTheme_="Classic";w.fontSizeBox_->setCurrentIndex(w.fontSizeBox_->findData("Large"));settle();
  for(const auto &size:{QSize(320,540),QSize(420,900),QSize(900,540)}){w.resize(size);for(const auto *destination:{"home","category:tools","category:personal"}){w.workbenchHub_->open(destination);settle();geometry(w,QString("Large %1x%2 %3").arg(size.width()).arg(size.height()).arg(destination));} }
  w.resize(320,540);w.workbenchHub_->open("category:personal");settle();capture(w,"calculator-submenu-personal-large-320");w.workbenchHub_->open("home");settle();capture(w,"calculator-menu-large-320");
  w.design_="Desktop";w.applyDesign();w.resize(680,540);w.showWorkspace("home");settle();check(w.workbenchHub_->matrix==identity&&w.workbenchHub_->findChild<QPushButton*>("category_tools")==tools,"return to Desktop preserves navigation and workbench widget identities");geometry(w,"Desktop restored680");
  w.saveTimer_->stop();std::cout<<checks_-failures_<<'/'<<checks_<<" native Calculator menu checks passed\n";return failures_?1:0;
 }
};
int main(int argc,char **argv){QTemporaryDir profile;if(!profile.isValid())return 2;QVector<QByteArray>args;for(int i=0;i<argc;++i)args<<argv[i];args<<"--config-root"<<profile.path().toUtf8();QVector<char*>pointers;for(auto &a:args)pointers<<a.data();int count=pointers.size();pointers<<nullptr;QApplication app(count,pointers.data());app.setStyle("Fusion");QApplication::setApplicationName("EMCICalc");DesktopFont::initialize(app);QString output;const int at=args.indexOf("--screenshots");if(at>=0&&at+1<args.size())output=QString::fromUtf8(args[at+1]);return DesktopAcceptanceTests().run(output);}
