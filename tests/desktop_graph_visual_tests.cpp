// SPDX-License-Identifier: MIT
// Full MainWindow graph-control rendering checks.
// This fixture uses native application widgets and the normal engine worker;
// its explicitly entered expression is a UI example and is never plotted.
#include "mainwindow.h"
#include "workspacehub.h"
#include "desktopworkbench.h"
#include "fontscale.h"
#include <QtTest/QTest>
#include <QJsonDocument>
#include <QSysInfo>
#include <iostream>

class DesktopAcceptanceTests {
 int checks_=0,failures_=0;
 QString output_,initialStyle_;
 QJsonArray scenes_;
 void check(bool ok,const QString &message){++checks_;if(!ok)++failures_;qInfo().noquote()<<(ok?"PASS":"FAIL")<<message;}
 void settle(){for(int i=0;i<3;++i){QApplication::processEvents();QCoreApplication::sendPostedEvents(nullptr,QEvent::LayoutRequest);QTest::qWait(5);}}
 void reveal(QWidget *widget){for(auto *p=widget->parentWidget();p;p=p->parentWidget())if(auto *scroll=qobject_cast<QScrollArea*>(p))scroll->ensureWidgetVisible(widget,8,8);settle();}
 static QJsonArray rectangle(const QRect &r){return {r.x(),r.y(),r.width(),r.height()};}
 QJsonArray inspect(MainWindow &window,const QString &context){
  auto *graphs=window.workbenchHub_->graphs;QJsonArray captions;bool fit=true;int count=0;
  for(auto *scroll:graphs->findChildren<QScrollArea*>())if(scroll->isVisibleTo(graphs))fit&=scroll->horizontalScrollBar()->maximum()==0&&scroll->widget()->width()<=scroll->viewport()->width();
  for(auto *button:graphs->findChildren<QPushButton*>())if(dynamic_cast<DesktopWorkbench::ActionButton*>(button)&&button->isVisibleTo(graphs)){
   QStyleOptionButton option;option.initFrom(button);option.text=button->text();
   if(button->menu())option.features|=QStyleOptionButton::HasMenu;
   if(button->isDown())option.state|=QStyle::State_Sunken;
   if(button->isChecked())option.state|=QStyle::State_On;
   const auto content=DesktopWorkbench::buttonCaptionRect(button,option);
   const auto textHeight=DesktopWorkbench::wrappedHeight(button->text(),button->font(),qMax(1,content.width()));
   const QRect bounds(button->mapTo(graphs,QPoint()),button->size());
   const bool captionFits=button->height()>=button->heightForWidth(button->width())&&content.height()>=textHeight&&bounds.left()>=0&&bounds.right()<graphs->width();
   fit&=captionFits;++count;
   captions.append(QJsonObject{{"text",button->text()},{"name",button->objectName()},{"bounds",rectangle(bounds)},{"content",rectangle(content)},{"text_height",textHeight},{"required_height",button->heightForWidth(button->width())},{"fits",captionFits}});
  }
  check(fit&&count>0,context+": full-app layout supplies complete styled caption height and local scrolling");
  return captions;
 }
 void capture(MainWindow &window,const QString &name,const QString &example){
  const auto captions=inspect(window,name);const QFontInfo font(window.font());
  const QString file=name+".png";
  if(!output_.isEmpty())check(window.grab().save(QDir(output_).filePath(file)),"native application capture "+file);
  scenes_.append(QJsonObject{{"file",file},{"skin",window.calculatorTheme_},{"font_choice",window.fontSizeMode_},{"font_family",font.family()},{"font_point_size",font.pointSizeF()},{"device_pixel_ratio",window.devicePixelRatioF()},{"client_size",QJsonArray{window.width(),window.height()}},{"example_provenance",example},{"captions",captions}});
 }
public:
 int run(const QString &output,const QString &initialStyle){
  output_=output;initialStyle_=initialStyle;if(!output_.isEmpty())QDir().mkpath(output_);
  MainWindow window(true);window.show();window.activateWindow();
  QElapsedTimer ready;ready.start();while((!window.catalogReady_||!window.registersReady_)&&ready.elapsed()<15000)settle();
  check(window.catalogReady_&&window.registersReady_,"fresh profile engine worker and function catalog ready");
  window.saveTimer_->stop();
  auto *graphs=window.workbenchHub_->graphs;auto *tabs=graphs->findChild<QTabWidget*>("graphTabs");
  auto *add=graphs->findChild<QPushButton*>("graphAddCurve");auto *edit=graphs->findChild<QPushButton*>("graphKeyboard");
  auto *expression=graphs->findChild<QLineEdit*>("graphExpression");auto *kind=graphs->findChild<QComboBox*>("graphKind");
  check(tabs&&add&&edit&&expression&&kind,"actual MainWindow Graph controls present");
  if(!tabs||!add||!edit||!expression||!kind)return 1;
  const auto emptyGraph=graphs->saveState();const auto history=window.history_;
  const QString answer=window.answerState_.canonical,last=window.lastSerialized_;
  check(emptyGraph.value("series").toArray().isEmpty()&&window.input_->text().isEmpty()&&window.committedResult_.isEmpty(),"fresh profile has no graph or calculation sample");
  window.design_="Calculator";window.calculatorTheme_="Classic";window.applyDesign();window.syncSettingsControls();window.resize(477,900);window.showGraph();settle();
  for(const QString &skin:QStringList{"Classic","Modern Light"}){
   window.calculatorTheme_=skin;window.applyTheme();window.syncSettingsControls();
   check(graphs->restoreState(emptyGraph),skin+": restore blank graph configuration without evaluation");tabs->setCurrentIndex(0);window.showGraph();window.activateWindow();settle();
   const QString suffix=skin=="Classic"?"classic":"modern-light";
   capture(window,"graph-empty-"+suffix,"Blank graph from a fresh temporary profile; no sample curve or calculation.");
   reveal(add);QTest::mouseClick(add,Qt::LeftButton);settle();
   check(tabs->currentIndex()==1&&expression->hasFocus(),skin+": mouse Add curve opens Series and focuses expression");
   QTest::keyClicks(expression,"x^2+y^2");kind->setFocus();QTest::keyClick(kind,Qt::Key_End);settle();
   check(kind->currentIndex()==int(EmciGraph::Kind::Surface3D)&&expression->text()=="x^2+y^2",skin+": keyboard selects surface type while preserving typed expression");
   reveal(edit);edit->setFocus();QTest::keyClick(edit,Qt::Key_Space);settle();
   check(expression->hasFocus()&&expression->text()=="x^2+y^2",skin+": keyboard Edit expression retains native input and draft");
   QPushButton *fold=nullptr;for(auto *candidate:graphs->findChildren<QPushButton*>())if(candidate->property("graphFoldCaption").toString()=="Curve appearance")fold=candidate;
   check(fold!=nullptr,skin+": curve appearance disclosure present");
   if(fold){reveal(fold);fold->setFocus();QTest::keyClick(fold,Qt::Key_Space);settle();check(fold->isChecked(),skin+": keyboard opens curve appearance");inspect(window,skin+" expanded disclosure");QTest::mouseClick(fold,Qt::LeftButton);settle();check(!fold->isChecked(),skin+": mouse closes curve appearance");}
   const auto draft=graphs->saveState();
   window.showPage(0);window.showGraph();tabs->setCurrentIndex(1);settle();
   check(graphs->saveState()==draft&&expression->text()=="x^2+y^2",skin+": navigation preserves typed graph draft");
   QPushButton *view=nullptr;for(auto *candidate:graphs->findChildren<QPushButton*>())if(candidate->menu()&&candidate->isVisibleTo(graphs))view=candidate;
   check(view!=nullptr,skin+": native View menu button remains available");
   if(view){bool opened=false;auto *menu=view->menu();QTimer::singleShot(30,menu,[&opened,menu]{opened=menu->isVisible();QTest::keyClick(menu,Qt::Key_Escape);});QTest::mouseClick(view,Qt::LeftButton);QTest::qWait(40);settle();check(opened&&!menu->isVisible(),skin+": mouse opens native View menu and Escape dismisses it");}
   for(auto *scroll:graphs->findChildren<QScrollArea*>())if(scroll->isVisibleTo(graphs))scroll->verticalScrollBar()->setValue(0);
   expression->setFocus();expression->deselect();settle();
   capture(window,"graph-series-"+suffix,"UI test explicitly entered x^2+y^2 and selected z=f(x,y); the example was not plotted or evaluated.");
   check(graphs->result().series.isEmpty()&&window.history_==history&&window.answerState_.canonical==answer&&window.lastSerialized_==last,skin+": editing and navigation do not evaluate or commit history/Ans");
  }
  window.saveTimer_->stop();
  if(!output_.isEmpty()){
   QFile file(QDir(output_).filePath("graph-visual.json"));check(file.open(QIODevice::WriteOnly),"visual provenance manifest writable");
   if(file.isOpen())file.write(QJsonDocument(QJsonObject{{"schema",1},{"product_version","1.0.0"},{"checks",checks_},{"failures",failures_},{"qt",qVersion()},{"platform",QGuiApplication::platformName()},{"operating_system",QSysInfo::prettyProductName()},{"startup_style",initialStyle_},{"effective_style_class",QString::fromLatin1(qApp->style()->metaObject()->className())},{"scale_factor_environment",qEnvironmentVariable("QT_SCALE_FACTOR")},{"provenance","QWidget::grab of actual MainWindow client area on the stated Qt platform. No mockup, no simulated screenshot, and no claim of a Windows runtime unless the stated platform is windows."},{"scenes",scenes_}}).toJson());
  }
  std::cout<<checks_-failures_<<'/'<<checks_<<" Full-app visual checks passed\n";return failures_?1:0;
 }
};

int main(int argc,char **argv){
 QTemporaryDir profile;if(!profile.isValid())return 2;
 QVector<QByteArray> args;for(int i=0;i<argc;++i)args<<argv[i];args<<"--config-root"<<profile.path().toUtf8();
 QVector<char*> pointers;for(auto &arg:args)pointers<<arg.data();int count=pointers.size();pointers<<nullptr;
 QApplication app(count,pointers.data());QApplication::setApplicationName("EMCICalc");
 // Preserve the actual platform default (or explicit Qt -style option).
 // In particular a Windows run must not silently force the Linux Fusion style.
 const QString initialStyle=app.style()->objectName();DesktopFont::initialize(app);
 QString output;const int at=args.indexOf("--screenshots");if(at>=0&&at+1<args.size())output=QString::fromUtf8(args[at+1]);
 return DesktopAcceptanceTests().run(output,initialStyle);
}
