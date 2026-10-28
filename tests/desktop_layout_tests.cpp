// SPDX-License-Identifier: MIT
#include "mainwindow.h"
#include "workspacehub.h"
#include "calculatorthemes.h"
#include "fontscale.h"
#include "i18n.h"
#include <QtTest/QTest>
#include <iostream>
class DesktopAcceptanceTests {
 int checks_=0,failures_=0;
 void check(bool ok,const QString &message){++checks_;if(!ok)++failures_;qInfo().noquote()<<(ok?"PASS":"FAIL")<<message;}
 void settle(){QCoreApplication::processEvents();QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);QTest::qWait(30);}
 bool palette(MainWindow &window,int page,const QString &context){
  window.showPage(page);QApplication::processEvents();auto *widget=window.pages_->currentWidget();const auto expected=calculatorThemeDefinition(window.calculatorTheme_).body;
  auto report=[&](const char *when){qInfo()<<"PALETTE"<<context<<page<<when<<"actual"<<widget->palette().color(QPalette::Window)<<"expected"<<expected<<"stored"<<qvariant_cast<QPalette>(widget->property("twPalette")).color(QPalette::Window)<<"application"<<qApp->palette().color(QPalette::Window);};
  report("immediate");settle();report("settled");const bool ok=widget->palette().color(QPalette::Window)==expected;window.showPage(0);return ok;
 }
public:
 int run(){
  MainWindow window(true);window.show();QElapsedTimer ready;ready.start();while(!(window.catalogReady_&&window.registersReady_)&&ready.elapsed()<15000)settle();check(window.catalogReady_&&window.registersReady_,"worker and catalog ready");window.saveTimer_->stop();
  for(const auto &sample:QStringList{QStringLiteral("設定文字サイズ表示"),QStringLiteral("设置字体大小显示")}){QTextLayout shaped(sample,window.font());shaped.beginLayout();auto line=shaped.createLine();if(line.isValid())line.setLineWidth(500);shaped.endLayout();bool covered=!shaped.glyphRuns().isEmpty();QStringList families;for(const auto &run:shaped.glyphRuns()){families<<run.rawFont().familyName();for(auto index:run.glyphIndexes())covered&=index!=0;}check(covered,"registered fallback renders every glyph: "+sample);qInfo()<<"GLYPH FAMILIES"<<sample<<families;}
  window.design_="Calculator";window.calculatorTheme_="Classic";window.applyDesign();window.syncSettingsControls();window.resize(320,540);window.showPage(0);settle();
  for(int page:{7,8})check(palette(window,page,"initial Classic"),"initial Classic auxiliary palette "+QString::number(page));
  auto *graphs=window.workbenchHub_->graphs;auto *tabs=graphs->findChild<QTabWidget*>("graphTabs");check(tabs&&tabs->count()==4,"four actual graph tabs");if(!tabs)return 1;
  for(const auto &language:languageCodes()){
   window.language_=language;window.updateTranslations();window.showPage(0);window.resize(320,540);window.showGraph();settle();
   for(auto *scroll:graphs->findChildren<QScrollArea*>())if(!scroll->isVisibleTo(graphs)&&scroll->horizontalScrollBar()->maximum())qInfo()<<"HIDDEN SCROLL"<<language<<scroll->objectName()<<scroll->size()<<scroll->viewport()->size()<<scroll->horizontalScrollBar()->maximum();
   for(int tab=0;tab<tabs->count();++tab){
    tabs->setCurrentIndex(tab);settle();bool fits=graphs->isVisible()&&graphs->width()<=320&&graphs->height()<=window.pages_->height();
    for(auto *scroll:graphs->findChildren<QScrollArea*>())if(scroll->isVisibleTo(graphs)){
     const bool local=scroll->horizontalScrollBar()->maximum()==0;fits&=local;
     if(!local)qWarning()<<"VISIBLE SCROLL"<<language<<tab<<scroll->objectName()<<"graph"<<graphs->size()<<"viewport"<<scroll->viewport()->size()<<"body"<<scroll->widget()->size()<<"maximum"<<scroll->horizontalScrollBar()->maximum();
    }
    for(auto *button:graphs->findChildren<QAbstractButton*>())if(button->isVisibleTo(graphs)){
     const QRect bounds(button->mapTo(graphs,QPoint()),button->size());const bool local=bounds.left()>=0&&bounds.right()<graphs->width();fits&=local;
     if(!local)qWarning()<<"VISIBLE BUTTON"<<language<<tab<<button->objectName()<<button->text()<<"bounds"<<bounds<<"graph"<<graphs->size();
    }
    check(fits,QString("actual visible graph tab %1 fits320 in %2").arg(tab).arg(language));
    if(!fits)window.grab().save(QDir::tempPath()+"/emcicalc-layout-"+language+'-'+QString::number(tab)+".png");
   }
   window.showPage(0);for(int page:{3,6,7,8}){window.showPage(page);settle();window.showPage(0);}
   for(auto view:{CalculatorTools::View::Statistics,CalculatorTools::View::Matrix}){window.catalogTools_->setView(view);window.showPage(7);settle();window.showPage(0);}window.catalogTools_->setView(CalculatorTools::View::Constants);
  }
  window.language_="en";window.updateTranslations();for(int page:{7,8})check(palette(window,page,"Classic after locales"),"Classic auxiliary palette after all locales "+QString::number(page));
  for(const auto &skin:calculatorThemeNames()){
   window.calculatorTheme_=skin;window.applyTheme();settle();for(int page:{7,8})check(palette(window,page,skin),skin+" auxiliary palette "+QString::number(page));
  }
  window.showPage(0);window.saveTimer_->stop();std::cout<<checks_-failures_<<'/'<<checks_<<" actual desktop layout checks passed\n";return failures_?1:0;
 }
};
int main(int argc,char **argv){QTemporaryDir profile;QVector<QByteArray> args;for(int i=0;i<argc;++i)args<<argv[i];args<<"--config-root"<<profile.path().toUtf8();QVector<char*> pointers;for(auto &arg:args)pointers<<arg.data();int count=pointers.size();pointers<<nullptr;QApplication app(count,pointers.data());app.setStyle("Fusion");QApplication::setApplicationName("EMCICalc");DesktopFont::initialize(app);return DesktopAcceptanceTests().run();}
