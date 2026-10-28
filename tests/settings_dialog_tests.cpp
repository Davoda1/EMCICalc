// SPDX-License-Identifier: MIT
// Native transaction, persistence, navigation and responsiveness regression tests.
#include "mainwindow.h"
#include <iostream>

class SettingsDialogTests {
 MainWindow &w;
 int checks_=0,failures_=0;
 QString screenshots_;
 void check(bool pass,const QString &message){++checks_;if(!pass)++failures_;std::cout<<(pass?"PASS ":"FAIL ")<<message.toStdString()<<'\n'<<std::flush;}
 bool wait(const std::function<bool()> &done,int timeout=30000){QElapsedTimer timer;timer.start();do{QApplication::processEvents(QEventLoop::AllEvents,20);if(done())return true;QThread::msleep(2);}while(timer.elapsed()<timeout);return done();}
 void settle(){for(int i=0;i<5;++i){QApplication::processEvents(QEventLoop::AllEvents,20);QCoreApplication::sendPostedEvents(nullptr,QEvent::LayoutRequest);}}
 bool idle(){return wait([&]{return w.registersReady_&&!w.busy_&&!w.formatPending_&&!w.previewRunning_&&!w.previewPending_&&!w.previewTimer_->isActive();});}
 void select(QComboBox *box,const QString &value){int i=box->findData(value);check(i>=0,"Setting exists: "+value);if(i>=0)box->setCurrentIndex(i);}
 QJsonObject saved(){QFile file(w.configPath_);if(!file.open(QIODevice::ReadOnly))return {};return QJsonDocument::fromJson(file.readAll()).object();}
 void shot(const QString &name){if(screenshots_.isEmpty())return;settle();check(w.grab().save(QDir(screenshots_).filePath(name+".png")),"Native screenshot: "+name);}
public:
 SettingsDialogTests(MainWindow &window,QString screenshots):w(window),screenshots_(std::move(screenshots)){}
 int run(){
  check(idle(),"Native calculator is ready");
  if(QCoreApplication::arguments().contains("--reload-check")){
   check(w.theme_=="Aurora"&&w.appearance_=="Dark"&&w.desktopEffects_==3&&w.precision_==47&&!w.keyTooltipsEnabled_&&!w.screenTooltipsEnabled_&&w.language_=="en"&&w.design_=="Desktop","Fresh process restores the committed Desktop preferences");
   return failures_?1:0;
  }
  check(w.design_=="Desktop"&&!w.handheld_,"Fresh profile starts in Desktop");
  w.input_->setText("17/29");w.evaluate();check(idle(),"Existing calculation completes");
  const auto expression=w.input_->text(),result=w.result_->text();const auto rows=w.history_.size();
  w.showPage(1);settle();auto historyCell=QPointer<QWidget>(w.historyTable_->cellWidget(0,0));
  w.showPage(3);settle();
  check(w.settingsDraftActive_&&w.settingsReturnPage_==1&&!w.settingsApply_->isEnabled()&&w.navigation_->isTabVisible(3)&&w.navigation_->currentIndex()==3,"Desktop Settings opens an unchanged draft and remembers History");
  check(w.settingsBack_->isVisible()&&!w.settingsBack_->icon().isNull()&&w.settingsCancel_->isVisible()&&w.settingsApply_->isVisible(),"Desktop has Back icon, Cancel and Apply controls");
  const int styleInstalls=w.property("themeStyleInstalls").toInt();const auto theme=w.theme_;const auto geometry=w.size();const auto generation=w.evaluationGeneration_;
  select(w.themeBox_,"Aurora");select(w.appearanceBox_,"Dark");w.desktopEffectsBox_->setCurrentIndex(3);w.precisionBox_->setValue(47);w.keyTooltipsBox_->setCurrentIndex(1);w.screenTooltipsBox_->setCurrentIndex(1);select(w.languageBox_,"hu");select(w.designBox_,"Calculator");
  check(w.theme_==theme&&w.precision_==30&&w.keyTooltipsEnabled_&&w.screenTooltipsEnabled_&&w.language_=="en"&&!w.handheld_&&w.size()==geometry,"All edited controls including language and mode remain draft-only");
  check(w.settingsForm_->isRowVisible(w.calculatorThemeBox_)&&!w.settingsForm_->isRowVisible(w.themeBox_),"Draft mode reveals its own options without switching the live app");
  check(w.property("themeStyleInstalls").toInt()==styleInstalls&&w.evaluationGeneration_==generation&&w.history_.size()==rows&&w.input_->text()==expression&&w.result_->text()==result,"Draft edits do not repolish, evaluate, append history or alter the current calculation");
  check(w.saveSettings()&&saved()["theme"]==theme&&saved()["precision"].toInt()==30&&saved()["mode"]=="Desktop"&&saved()["language"]=="en","Autosave during an open draft writes committed preferences only");
  w.settingsCancel_->click();settle();
  check(w.pages_->currentIndex()==1&&!w.settingsDraftActive_&&!w.navigation_->isTabVisible(3)&&w.navigation_->currentIndex()==1&&w.designBox_->currentData()=="Desktop"&&w.precisionBox_->value()==30&&w.themeBox_->currentData()==theme,"Cancel discards draft controls and returns to the opening page");
  w.showPage(2);w.showPage(3);select(w.themeBox_,"Aurora");w.settingsBack_->click();
  check(w.pages_->currentIndex()==2&&w.theme_==theme&&w.themeBox_->currentData()==theme,"Back discards pending changes and returns to Toolbox");
  w.showPage(3);w.precisionBox_->setValue(66);w.showPage(4);
  check(!w.settingsDraftActive_&&w.precision_==30&&w.precisionBox_->value()==30,"Other navigation also discards the Desktop draft");
  w.showPage(3);w.resize(680,540);settle();auto scroll=qobject_cast<QScrollArea*>(w.pages_->widget(3));
  check(scroll&&scroll->horizontalScrollBar()->maximum()==0,"Desktop settings fits the 680 by 540 minimum without horizontal scrolling");
  if(scroll){scroll->verticalScrollBar()->setValue(scroll->verticalScrollBar()->maximum());settle();check(w.settingsApply_->isVisible()&&w.settingsActions_->geometry().bottom()<=scroll->height()&&w.settingsActions_->geometry().top()>=scroll->viewport()->geometry().bottom(),"Back, Cancel and Apply stay outside the scrolling form");}
  shot("settings-dialog-desktop-settings-minimum");
  if(scroll)scroll->verticalScrollBar()->setValue(0);
  select(w.themeBox_,"Aurora");select(w.appearanceBox_,"Dark");w.desktopEffectsBox_->setCurrentIndex(3);w.precisionBox_->setValue(47);w.keyTooltipsBox_->setCurrentIndex(1);w.screenTooltipsBox_->setCurrentIndex(1);
  historyCell=w.historyTable_->cellWidget(0,0);const int beforeApply=w.property("themeStyleInstalls").toInt();
  QElapsedTimer applying;applying.start();w.settingsApply_->click();const auto applyMs=applying.elapsed();settle();check(idle(),"Applied preferences finish formatting");
  check(w.theme_=="Aurora"&&w.appearance_=="Dark"&&w.desktopEffects_==3&&w.precision_==47&&!w.keyTooltipsEnabled_&&!w.screenTooltipsEnabled_,"Apply commits the complete Desktop settings transaction");
  check(w.pages_->currentIndex()==3&&w.settingsDraftActive_&&!w.settingsApply_->isEnabled(),"Apply remains on Settings with a fresh unchanged draft");
  check(w.property("themeStyleInstalls").toInt()==beforeApply+1,"Combined theme, appearance and effects changes install one final stylesheet");
  check(historyCell&&!historyCell.isNull(),"Apply does not rebuild the hidden mathematical history widgets");
  check(saved()["theme"]=="Aurora"&&saved()["precision"].toInt()==47&&saved()["version"]=="1.0.0"&&saved()["profileFormat"].toInt()==1,"Apply immediately saves Desktop 1.0.0 preferences and profile format");
  std::cout<<"INFO Desktop multi-setting Apply took "<<applyMs<<" ms\n";
  const int appliedStyles=w.property("themeStyleInstalls").toInt();w.applyTheme();check(w.property("themeStyleInstalls").toInt()==appliedStyles,"Applying an unchanged theme does not repolish the whole app");
  check(wait([&]{return w.desktopSurface_->property("desktopBackdropPixels").toInt()>0;},3000),"Deferred decorative backdrop renders after controls repaint");
  check(w.desktopSurface_->property("desktopBackdropPixels").toInt()<=640*450,"Desktop blur cache is bounded to 288,000 decorative pixels");
  QProcess child;child.start(QCoreApplication::applicationFilePath(),{"--reload-check","--config-root",QFileInfo(w.configPath_).absolutePath()});check(child.waitForFinished(30000)&&child.exitCode()==0,"Fresh native process reloads only committed settings");
  if(child.exitCode()!=0)std::cerr<<child.readAllStandardOutput().toStdString()<<child.readAllStandardError().toStdString();
  w.resize(1100,820);shot("settings-dialog-desktop-settings");
  select(w.designBox_,"Calculator");select(w.calculatorThemeBox_,"Deep Ocean");check(!w.handheld_&&w.settingsActions_->isVisible(),"Desktop to Calculator stays pending until Apply");w.settingsApply_->click();settle();
  check(w.handheld_&&w.design_=="Calculator"&&w.calculatorTheme_=="Deep Ocean"&&!w.settingsDraftActive_&&w.settingsActions_->isHidden(),"Apply switches to Calculator and restores its immediate settings flow");
  const int precision=w.precision_;w.precisionBox_->setValue(precision+1);w.screenTooltipsBox_->setCurrentIndex(0);select(w.calculatorThemeBox_,"Classic");
  check(w.precision_==precision+1&&w.screenTooltipsEnabled_&&w.calculatorTheme_=="Classic","Calculator settings still apply immediately");
  w.resize(480,900);shot("settings-dialog-calculator-settings-unchanged");
  select(w.designBox_,"Desktop");check(!w.handheld_&&w.settingsDraftActive_&&w.settingsActions_->isVisible(),"Calculator to Desktop starts a new Desktop draft after the immediate mode switch");
  w.precisionBox_->setValue(85);w.settingsCancel_->click();check(w.precision_==precision+1&&w.pages_->currentIndex()==0,"New Desktop draft cancels without reverting the applied mode switch");
  w.showPage(3);w.precisionBox_->setValue(99);w.close();check(saved()["precision"].toInt()==precision+1,"Closing with pending settings never saves the draft");
  std::cout<<checks_-failures_<<'/'<<checks_<<" settings dialog settings checks passed\n";
  return failures_?1:0;
 }
};
int main(int argc,char **argv){
 QTemporaryDir config;if(!config.isValid())return 2;QList<QByteArray> args;for(int i=0;i<argc;++i)args<<argv[i];if(!args.contains("--config-root"))args<<"--config-root"<<config.path().toUtf8();QList<char*> pointers;for(auto &arg:args)pointers<<arg.data();int count=pointers.size();
 QApplication app(count,pointers.data());app.setStyle("Fusion");QString screenshots;const int at=args.indexOf("--screenshots");if(at>=0&&at+1<args.size())screenshots=QString::fromUtf8(args[at+1]);if(!screenshots.isEmpty())QDir().mkpath(screenshots);
 MainWindow window(true);window.show();return SettingsDialogTests(window,screenshots).run();
}
