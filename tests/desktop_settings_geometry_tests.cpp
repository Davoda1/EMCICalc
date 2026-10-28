// SPDX-License-Identifier: MIT
// Native footer geometry: staged Desktop settings and immediate Calculator settings.
#include "mainwindow.h"
#include "fontscale.h"
#include "calculatorthemes.h"
#include "i18n.h"
#include <QtTest/QTest>
#include <iostream>

class DesktopAcceptanceTests {
 int checks_=0,failures_=0;
 QString directory_;
 QJsonArray cases_,calculatorCases_;
 void check(bool ok,const QString &label){++checks_;if(!ok)++failures_;qInfo().noquote()<<(ok?"PASS":"FAIL")<<label;}
 void settle(){for(int i=0;i<4;++i){QCoreApplication::processEvents();QCoreApplication::sendPostedEvents(nullptr,QEvent::LayoutRequest);QTest::qWait(10);}}
 bool ready(MainWindow &w){QElapsedTimer timer;timer.start();while((!w.registersReady_||!w.catalogReady_)&&timer.elapsed()<15000)settle();return w.registersReady_&&w.catalogReady_;}
 static QJsonArray rectangle(const QRect &r){return {r.x(),r.y(),r.width(),r.height()};}
 void inspect(MainWindow &w,QScrollArea *scroll,const QString &name){
  settle();QJsonArray buttons;bool contained=true,unobscured=true,mouseReachable=true,heights=true;
  const QRect footer(w.settingsActions_->mapTo(scroll,QPoint()),w.settingsActions_->size());
  const auto *bar=scroll->verticalScrollBar();const QRect scrollbar(bar->mapTo(scroll,QPoint()),bar->size());
  for(auto *button:{w.settingsBack_,w.settingsCancel_,w.settingsApply_}){
   button->setFocus(Qt::TabFocusReason);settle();
   const QRect area(button->mapTo(scroll,QPoint()),button->size());
   const QRect local(button->mapTo(w.settingsActions_,QPoint()),button->size());
   const QRegion missing=QRegion(button->rect()).subtracted(button->visibleRegion());
   const bool inside=scroll->rect().contains(area)&&w.settingsActions_->rect().contains(local);
   const bool clear=missing.isEmpty()&&(!bar->isVisible()||!area.intersects(scrollbar));
   bool hits=true;
   for(const auto &point:{QPoint(1,1),QPoint(button->width()-2,1),QPoint(1,button->height()-2),QPoint(button->width()-2,button->height()-2),button->rect().center()}){
    auto *hit=w.childAt(button->mapTo(&w,point));hits&=hit&&(hit==button||button->isAncestorOf(hit));
   }
   const bool tall=button->height()>=button->minimumSizeHint().height();
   contained&=inside;unobscured&=clear;mouseReachable&=hits;heights&=tall;
   buttons.append(QJsonObject{{"name",button->objectName()},{"rect_in_scroll",rectangle(area)},{"visible_bounds",rectangle(button->visibleRegion().boundingRect())},{"minimum_height",button->minimumSizeHint().height()},{"contained",inside},{"unobscured",clear},{"mouse_reachable",hits},{"focused",button->hasFocus()}});
  }
  w.settingsBack_->setFocus(Qt::TabFocusReason);QTest::keyClick(w.settingsBack_,Qt::Key_Tab);const bool cancelFocus=w.settingsCancel_->hasFocus();QTest::keyClick(w.settingsCancel_,Qt::Key_Tab);const bool applyFocus=w.settingsApply_->hasFocus();
  check(contained,name+": complete action rectangles and bottom edges contained");
  check(unobscured,name+": full action surfaces are visible and avoid scrollbar");
  check(mouseReachable,name+": action corners and centers receive mouse hits");
  check(heights,name+": focused action heights meet style/font minimum");
  check(cancelFocus&&applyFocus,name+": Back, Cancel and Apply keyboard focus order");
  check(scroll->horizontalScrollBar()->maximum()==0,name+": no horizontal form scrolling");
  const QString image=name+".png";if(!directory_.isEmpty())check(w.grab().save(QDir(directory_).filePath(image)),name+": native capture saved");
  cases_.append(QJsonObject{{"name",name},{"window_size",QJsonArray{w.width(),w.height()}},{"font",w.fontSizeMode_},{"theme",w.theme_},{"appearance",w.appearance_},{"device_pixel_ratio",w.devicePixelRatioF()},{"viewport",rectangle(scroll->viewport()->geometry())},{"footer",rectangle(footer)},{"scrollbar",rectangle(scrollbar)},{"buttons",buttons},{"image",image}});
 }
 void calculatorCapture(MainWindow &w,QScrollArea *scroll,const QString &name){
  settle();bool fits=scroll->horizontalScrollBar()->maximum()==0&&w.settingsPage_->width()<=scroll->viewport()->width();
  for(const auto &row:w.settingsRows_)if(!row.second->isHidden()){
   const QRect area(row.second->mapTo(w.settingsPage_,QPoint()),row.second->size());fits&=area.left()>=0&&area.right()<w.settingsPage_->width();
  }
  check(fits,name+": all settings fields fit the local viewport");
  check(w.settingsActions_->isHidden()&&!w.settingsDraftActive_,name+": immediate mode has no Desktop footer");
  QStyleOptionButton menu;menu.initFrom(w.auxMenu_);menu.text=w.auxMenu_->text();const auto contents=w.auxMenu_->style()->subElementRect(QStyle::SE_PushButtonContents,&menu,w.auxMenu_);const auto metrics=w.auxMenu_->fontMetrics();
  check(contents.width()>=metrics.horizontalAdvance(menu.text)&&contents.height()>=metrics.height(),name+": MENU caption fits its styled content rectangle");
  const QString image=name+".png";if(!directory_.isEmpty())check(w.grab().save(QDir(directory_).filePath(image)),name+": native Calculator capture saved");
  calculatorCases_.append(QJsonObject{{"name",name},{"window_size",QJsonArray{w.width(),w.height()}},{"font",w.fontSizeMode_},{"skin",w.calculatorTheme_},{"language",w.language_},{"device_pixel_ratio",w.devicePixelRatioF()},{"viewport",rectangle(scroll->viewport()->geometry())},{"image",image},{"fits",fits}});
 }

public:
 int run(const QString &directory){
  directory_=directory;if(!directory_.isEmpty())QDir().mkpath(directory_);
  MainWindow w(true);w.show();check(ready(w),"fresh native profile worker and catalog ready");w.showPage(3);settle();
  auto *scroll=qobject_cast<QScrollArea*>(w.pages_->widget(3));check(scroll!=nullptr,"Settings retains native scroll area");if(!scroll)return 1;
  check(w.design_=="Desktop"&&w.settingsDraftActive_,"Desktop preferences are staged");
  for(const auto &font:QStringList{"System","Small","Medium","Large"}){
   w.resize(680,540);settle();
   const auto previous=w.fontSizeMode_;w.fontSizeBox_->setCurrentIndex(w.fontSizeBox_->findData(font));
   check(w.fontSizeMode_==previous,"font choice stays staged: "+font);w.settingsApply_->click();settle();check(w.fontSizeMode_==font,"Apply commits font: "+font);
   const int precision=w.precision_;w.precisionBox_->setValue(precision+1);check(w.precision_==precision&&w.settingsApply_->isEnabled(),"independent pending edit remains staged: "+font);
   inspect(w,scroll,font.toLower()+"-680x540-after-font-apply");
   for(const auto &size:{QSize(680,540),QSize(1180,760)}){
    w.resize(size);settle();const QString base=font.toLower()+QString("-%1x%2").arg(size.width()).arg(size.height());
    scroll->verticalScrollBar()->setValue(0);inspect(w,scroll,base+"-top");scroll->verticalScrollBar()->setValue(scroll->verticalScrollBar()->maximum());inspect(w,scroll,base+"-bottom");
   }
   w.settingsCancel_->click();check(w.precision_==precision,"Cancel discards pending edit after resize/scroll/focus: "+font);w.showPage(3);settle();
  }
  w.resize(680,540);settle();w.fontSizeBox_->setCurrentIndex(w.fontSizeBox_->findData("Medium"));w.settingsApply_->click();settle();w.precisionBox_->setValue(w.precision_+1);
  inspect(w,scroll,"medium-680x540-after-large");w.settingsCancel_->click();w.showPage(3);settle();
  // The same widget has no footer reservation in accepted immediate mode.
  w.designBox_->setCurrentIndex(w.designBox_->findData("Calculator"));w.settingsApply_->click();w.resize(320,540);settle();
  check(w.handheld_&&!w.settingsDraftActive_&&w.settingsActions_->isHidden(),"Calculator preserves immediate preferences and hides actions");
  w.fontSizeBox_->setCurrentIndex(w.fontSizeBox_->findData("Medium"));settle();check(w.fontSizeMode_=="Medium","Calculator font applies immediately");
  const QRect viewport=scroll->viewport()->geometry();check(viewport.bottom()>=scroll->height()-scroll->frameWidth()-2,"Calculator has no stale Desktop footer margin");
  check(w.settingsTitle_->isHidden()&&w.settingsSections_.size()==4&&w.settingsAboutTitle_->isVisible(),"Calculator uses grouped sections and a single page heading");
  bool sections=true;for(auto *section:w.settingsSections_)sections&=section->isVisible();check(sections,"Appearance, Input, Number notation and Help sections visible");
  w.fontSizeBox_->setFocus();QTest::keyClick(w.fontSizeBox_,Qt::Key_Tab);check(w.languageBox_->hasFocus(),"Calculator Tab follows Font size to Language");
  QTest::keyClick(w.languageBox_,Qt::Key_Tab);check(w.inputStyleBox_->hasFocus(),"Calculator Tab follows Language to Input style");
  QTest::keyClick(w.inputStyleBox_,Qt::Key_Backtab);check(w.languageBox_->hasFocus(),"Calculator Backtab returns Input style to Language");
  QTest::keyClick(w.languageBox_,Qt::Key_Backtab);check(w.fontSizeBox_->hasFocus(),"Calculator Backtab returns Language to Font size");
  auto *path=w.settingsPage_->findChild<QLabel*>("settingsStoragePath");check(path&&path->wordWrap()&&(path->textInteractionFlags()&Qt::TextSelectableByKeyboard),"full saved-data path stays wrapped and keyboard-selectable");
  for(auto *spin:{w.precisionBox_,w.decimalPlacesBox_}){
   scroll->ensureWidgetVisible(spin,0,12);settle();auto *increase=spin->findChild<QAbstractButton*>("desktopStepIncrease");auto *decrease=spin->findChild<QAbstractButton*>("desktopStepDecrease");
   check(increase&&decrease&&increase->isVisible()&&decrease->isVisible(),"numeric settings expose distinct visible plus/minus actions");
   if(increase&&decrease){const int original=spin->value();increase->click();check(spin->value()==original+1,"plus action edits the native numeric setting");decrease->click();check(spin->value()==original,"minus action restores the numeric setting");spin->setFocus();QTest::keyClick(spin,Qt::Key_Up);check(spin->value()==original+1,"numeric setting retains keyboard stepping");QTest::keyClick(spin,Qt::Key_Down);
    spin->setFocus();QTest::keyClick(spin,Qt::Key_Tab);check(decrease->hasFocus(),"numeric editor Tab reaches minus action");QTest::keyClick(decrease,Qt::Key_Tab);check(increase->hasFocus(),"minus action Tab reaches plus action");QTest::keyClick(increase,Qt::Key_Backtab);check(decrease->hasFocus(),"plus action Backtab returns to minus action");}
  }
  const auto originalRows=w.settingsRows_;const auto savedInput=w.input_->text();w.input_->setText("17/29");const auto originalPrecision=w.precisionBox_;const auto originalDecimals=w.decimalPlacesBox_;
  for(int visit=0;visit<4;++visit){w.showPage(0);w.showPage(3);settle();}
  check(w.settingsRows_==originalRows&&w.precisionBox_==originalPrecision&&w.decimalPlacesBox_==originalDecimals&&w.input_->text()=="17/29","page navigation preserves widget identity and input draft");w.input_->setText(savedInput);
  for(const auto &font:QStringList{"System","Medium","Large"}){
   w.fontSizeBox_->setCurrentIndex(w.fontSizeBox_->findData(font));
   for(const auto &size:{QSize(320,540),QSize(480,900),QSize(900,540)}){
    w.resize(size);settle();const QString base="calculator-"+font.toLower()+QString("-%1x%2").arg(size.width()).arg(size.height());
    scroll->verticalScrollBar()->setValue(0);calculatorCapture(w,scroll,base+"-top");scroll->verticalScrollBar()->setValue(scroll->verticalScrollBar()->maximum());calculatorCapture(w,scroll,base+"-about");
   }
  }
  w.fontSizeBox_->setCurrentIndex(w.fontSizeBox_->findData("Medium"));w.resize(320,540);
  for(const auto &skin:calculatorThemeNames()){
   w.calculatorThemeBox_->setCurrentIndex(w.calculatorThemeBox_->findData(skin));settle();scroll->verticalScrollBar()->setValue(0);
   check(w.settingsPage_->palette().color(QPalette::Window)==calculatorThemePalette(skin).color(QPalette::Window),"settings surface follows selected skin: "+skin);
   calculatorCapture(w,scroll,"calculator-skin-"+QString(skin).replace(' ','-').toLower());
  }
  w.calculatorThemeBox_->setCurrentIndex(w.calculatorThemeBox_->findData("Classic"));
  for(const auto &language:languageCodes()){
   w.languageBox_->setCurrentIndex(w.languageBox_->findData(language));settle();scroll->verticalScrollBar()->setValue(0);calculatorCapture(w,scroll,"calculator-language-"+language);
  }
  w.languageBox_->setCurrentIndex(w.languageBox_->findData("en"));w.designBox_->setCurrentIndex(w.designBox_->findData("Desktop"));settle();
  check(w.settingsDraftActive_&&w.settingsTitle_->isVisible()&&w.settingsAboutTitle_->isHidden(),"returning to Desktop restores original heading and staged semantics");
  bool desktopOrder=w.settingsForm_->rowCount()==w.settingsRows_.size();for(int row=0;row<w.settingsRows_.size();++row)desktopOrder&=w.settingsForm_->itemAt(row,QFormLayout::FieldRole)->widget()==w.settingsRows_[row].second;
  check(desktopOrder,"returning to Desktop restores exact original field order");
  check(w.precisionBox_->buttonSymbols()==QAbstractSpinBox::UpDownArrows&&w.decimalPlacesBox_->buttonSymbols()==QAbstractSpinBox::UpDownArrows,"Desktop restores native spin controls");
  w.fontSizeBox_->setFocus();QTest::keyClick(w.fontSizeBox_,Qt::Key_Tab);check(w.decimalPlacesBox_->hasFocus()||w.decimalPlacesBox_->isAncestorOf(QApplication::focusWidget()),"Desktop Tab restores Font size to FIX decimal places");
  w.precisionBox_->setFocus();QTest::keyClick(w.precisionBox_,Qt::Key_Backtab);check(w.languageBox_->hasFocus(),"Desktop Backtab restores Significant digits to Language");
  w.saveTimer_->stop();
  if(!directory_.isEmpty()){QFile file(QDir(directory_).filePath("settings-geometry.json"));check(file.open(QIODevice::WriteOnly),"geometry manifest writable");if(file.isOpen())file.write(QJsonDocument(QJsonObject{{"schema",1},{"checks",checks_},{"failures",failures_},{"product_version","1.0.0"},{"verification_revision",2},{"qt",qVersion()},{"style",qApp->style()->objectName()},{"platform",QGuiApplication::platformName()},{"provenance","Native application widgets rendered by the stated Qt platform; no mockup"},{"cases",cases_},{"calculator_cases",calculatorCases_}}).toJson());}
  std::cout<<checks_-failures_<<'/'<<checks_<<" desktop Settings geometry checks passed\n";return failures_?1:0;
 }
};
int main(int argc,char **argv){
 QTemporaryDir profile;if(!profile.isValid())return 2;QVector<QByteArray> args;for(int i=0;i<argc;++i)args<<argv[i];args<<"--config-root"<<profile.path().toUtf8();QVector<char*> pointers;for(auto &arg:args)pointers<<arg.data();int count=pointers.size();pointers<<nullptr;
 QApplication app(count,pointers.data());app.setStyle("Fusion");QCoreApplication::setApplicationName("EMCICalc");DesktopFont::initialize(app);
 QString output;const int at=args.indexOf("--screenshots");if(at>=0&&at+1<args.size())output=QString::fromUtf8(args[at+1]);return DesktopAcceptanceTests().run(output);
}
