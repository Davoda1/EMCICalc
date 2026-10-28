// SPDX-License-Identifier: MIT
// Real desktop settings transactions, including Windows delete-sharing rules.
#include "mainwindow.h"
#include <QCryptographicHash>
#include <QProcess>
#include <QTemporaryDir>
#include <iostream>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {
const QString finalDraft=QStringLiteral("987654321+(");
QByteArray bytesAt(const QString &path){QFile file(path);return file.open(QIODevice::ReadOnly)?file.readAll():QByteArray();}
QJsonObject objectAt(const QString &path){return QJsonDocument::fromJson(bytesAt(path)).object();}
QString savedDraft(const QString &path){return objectAt(path).value("inputDrafts").toObject().value("10").toString();}
}

class DesktopSettingsPersistenceTests {
 MainWindow &window_;
 int checks_=0,failures_=0;
 void check(bool pass,const char *name){++checks_;if(!pass)++failures_;std::cout<<(pass?"PASS ":"FAIL ")<<name<<'\n'<<std::flush;}
 bool wait(const std::function<bool()> &done,bool stopAutosave=true,int timeout=30000){
  QElapsedTimer timer;timer.start();
  do{if(stopAutosave)window_.saveTimer_->stop();QApplication::processEvents(QEventLoop::AllEvents,20);if(done())return true;QThread::msleep(2);}while(timer.elapsed()<timeout);
  return done();
 }
 bool ready(){return wait([&]{return window_.registersReady_&&window_.catalogReady_&&!window_.busy_&&!window_.formatPending_;});}
public:
 explicit DesktopSettingsPersistenceTests(MainWindow &window):window_(window){}
 int run(bool reload){
  auto &w=window_;w.saveTimer_->stop();
  if(!reload)check(!QFile::exists(w.configPath_),"clean profile has no settings before its first save");
  w.show();check(ready(),"actual desktop worker and catalogs initialize");w.saveTimer_->stop();
  if(reload){
   check(w.input_->text()==finalDraft&&w.baseDrafts_.value(2)=="101+1"&&w.precision_==47,
         "separate process restores latest incomplete draft, alternate-base draft and precision");
   check(w.saveSettings(),"restored process replaces its existing settings successfully");
   check(w.close()&&!w.isVisible()&&w.closing_,"restored process closes after a successful atomic save");
  }else{
   w.precision_=47;w.baseDrafts_[2]="101+1";w.input_->setText("17/29+(");w.saveTimer_->stop();
   check(w.saveSettings(),"first settings save creates the clean profile");
   check(QJsonDocument::fromJson(bytesAt(w.configPath_)).object().value("profileFormat").toInt()==1,"new profile records format 1");
   check(savedDraft(w.configPath_)=="17/29+("&&objectAt(w.configPath_).value("version")=="1.0.0",
         "initial complete JSON preserves an incomplete expression without changing product version");
   for(int i=0;i<3;++i){
    const QString draft=QString::number(1000+i)+"+(";w.input_->setText(draft);w.saveTimer_->stop();
    check(w.saveSettings()&&savedDraft(w.configPath_)==draft,"same-process repeated atomic replacement stores the latest draft");
   }
   w.input_->setText("314159265+(");w.saveTimer_->start(25);
   check(wait([&]{return savedDraft(w.configPath_)=="314159265+(";},false,10000),
         "real autosave timer can replace an existing settings file");
   w.saveTimer_->stop();
   check(w.settingsConflictPath_.isEmpty()&&QDir(QFileInfo(w.configPath_).absolutePath()).entryList({"settings-conflict-*.json"},QDir::Files).isEmpty(),
         "ordinary repeated saves do not generate false conflict snapshots");

   // A real transaction conflict must still protect both disk and the live draft.
   const auto committed=bytesAt(w.configPath_);const auto fingerprint=w.settingsFingerprint_;
   QLockFile peer(QDir(QFileInfo(w.configPath_).absolutePath()).filePath("desktop-profile.lock"));peer.setStaleLockTime(0);
   const bool locked=peer.tryLock(0);check(locked,"independent profile lock can be acquired between transactions");
   if(locked){
    w.input_->setText(finalDraft);w.saveTimer_->stop();
    check(!w.saveSettings()&&bytesAt(w.configPath_)==committed&&w.settingsFingerprint_==fingerprint,
          "busy profile leaves committed bytes and compare-and-swap fingerprint unchanged");
    check(!w.close()&&w.isVisible()&&!w.closing_&&w.input_->text()==finalDraft,
          "failed save refuses close and retains the unsaved draft");
    w.saveTimer_->stop();peer.unlock();
   }
   w.input_->setText(finalDraft);w.saveTimer_->stop();

#ifdef Q_OS_WIN
   // This is the actual Windows failure mode, not a geometry assertion or a
   // portable mock: readers without FILE_SHARE_DELETE deny atomic replacement.
   const auto widePath=QDir::toNativeSeparators(w.configPath_).toStdWString();
   HANDLE held=CreateFileW(widePath.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,
                         nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
   check(held!=INVALID_HANDLE_VALUE,"Windows opens a deliberate read handle without delete sharing");
   if(held!=INVALID_HANDLE_VALUE){
    const auto before=bytesAt(w.configPath_);const auto beforeFingerprint=w.settingsFingerprint_;
    check(!w.saveSettings()&&bytesAt(w.configPath_)==before&&w.settingsFingerprint_==beforeFingerprint,
          "Windows sharing violation preserves committed bytes and fingerprint");
    check(w.status_->toolTip().startsWith("Could not save settings: ")&&w.status_->toolTip().size()>25,
          "Windows save failure includes the underlying file error");
    check(!w.close()&&w.isVisible()&&!w.closing_&&w.input_->text()==finalDraft,
          "Windows sharing violation refuses close without losing the draft");
    w.saveTimer_->stop();CloseHandle(held);
   }
#else
   std::cout<<"SKIP Windows delete-sharing failure injection: requires native Windows execution\n";
#endif
   check(w.saveSettings()&&savedDraft(w.configPath_)==finalDraft,
         "releasing the blocker permits a successful retry with the retained draft");
   check(w.close()&&!w.isVisible()&&w.closing_,"window closes normally after successful repeated saves");
   w.saveTimer_->stop();
   QProcess child;child.setProcessChannelMode(QProcess::MergedChannels);
   child.start(QCoreApplication::applicationFilePath(),{"--reload-check","--config-root",QFileInfo(w.configPath_).absolutePath()});
   const bool childOk=child.waitForStarted(10000)&&child.waitForFinished(45000)
       &&child.exitStatus()==QProcess::NormalExit&&child.exitCode()==0;
   const auto childOutput=child.readAll();std::cout<<childOutput.constData()<<std::flush;
   check(childOk,"separate native process restores, saves and closes the profile");
   if(child.state()!=QProcess::NotRunning){child.kill();child.waitForFinished(5000);}
  }
  w.saveTimer_->stop();std::cout<<checks_-failures_<<'/'<<checks_<<" Settings persistence checks passed\n";
  return failures_?1:0;
 }
};

int main(int argc,char **argv){
 QTemporaryDir profile(QDir::tempPath()+"/EMCICalc-settings-persistence-XXXXXX");if(!profile.isValid())return 2;
 QList<QByteArray> args;for(int i=0;i<argc;++i)args<<argv[i];
 if(!args.contains("--config-root"))args<<"--config-root"<<QDir(profile.path()).filePath(QStringLiteral("Profile with spaces \u00e9")).toUtf8();
 QList<char*> pointers;for(auto &arg:args)pointers<<arg.data();int count=pointers.size();
 QApplication app(count,pointers.data());app.setApplicationName("EMCICalc");app.setApplicationVersion("1.0.0");app.setOrganizationName("davoda1");app.setStyle("Fusion");
 MainWindow window(true);return DesktopSettingsPersistenceTests(window).run(args.contains("--reload-check"));
}
