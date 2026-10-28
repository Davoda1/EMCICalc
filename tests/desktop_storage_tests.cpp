// SPDX-License-Identifier: MIT
#include "mainwindow.h"
#include "workspacehub.h"
#include "workspacemodel.h"
#include "workspacejournal.h"
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QProcess>
#include <iostream>
namespace {
bool put(const QString &path,const QByteArray &bytes){QSaveFile file(path);return file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size()&&file.commit();}
QByteArray get(const QString &path){QFile file(path);return file.open(QIODevice::ReadOnly)?file.readAll():QByteArray();}
}
class DesktopStorageTests {
 int failures_=0,checks_=0;
 void check(bool ok,const char *name){++checks_;if(!ok)++failures_;std::cout<<(ok?"PASS ":"FAIL ")<<name<<'\n'<<std::flush;}
 bool wait(std::function<bool()> done,int ms=10000){QElapsedTimer timer;timer.start();while(!done()&&timer.elapsed()<ms){QApplication::processEvents(QEventLoop::AllEvents,20);QThread::msleep(2);}return done();}
public:
 int run(){
  QTemporaryDir dir;check(dir.isValid(),"temporary journal profile");const auto path=[&](const QString &name){return QDir(dir.path()).filePath(name);};
  const QByteArray settings="{\"schema\":1,\"draft\":\"original\"}";const QByteArray history("binary\0history",14);
  check(put(path("settings.json"),settings)&&put(path("history.qz"),history),"seed exact binary backup data");
  QJsonObject journal;QString error;check(WorkspaceJournal::snapshot(dir.path(),journal,&error),"complete journal snapshot with digests");
  check(WorkspaceJournal::validate(journal,nullptr,&error),"valid journal schema and checksums");
  check(put(path("settings.json"),"changed settings")&&put(path("registers.json"),"new register")&&put(path("history.qz"),"changed history"),"simulate partial transaction");
  auto malformed=journal;auto files=malformed["files"].toObject();auto row=files["history.qz"].toObject();row["sha256"]="00";files["history.qz"]=row;malformed["files"]=files;
  check(!WorkspaceJournal::restore(dir.path(),malformed,&error)&&get(path("settings.json"))=="changed settings"&&get(path("registers.json"))=="new register"&&get(path("history.qz"))=="changed history","checksum rejection precedes every live write");
  malformed=journal;files=malformed["files"].toObject();files.remove("history.qz");malformed["files"]=files;
  check(!WorkspaceJournal::restore(dir.path(),malformed,&error)&&get(path("settings.json"))=="changed settings","missing file record is rejected transactionally");
  malformed=journal;files=malformed["files"].toObject();row=files["settings.json"].toObject();row["bytes"]="%%%bad";files["settings.json"]=row;malformed["files"]=files;
  check(!WorkspaceJournal::restore(dir.path(),malformed,&error)&&get(path("settings.json"))=="changed settings","malformed base64 rejected without writes");
  malformed=journal;malformed["schema"]=2;check(!WorkspaceJournal::restore(dir.path(),malformed,&error),"unsupported journal schema rejected");
  check(WorkspaceJournal::recover(dir.path(),&error)&&get(path("settings.json"))==settings&&get(path("history.qz"))==history&&!QFile::exists(path("registers.json"))&&!QFile::exists(path("workspace-import.pending")),"recovery restores exact bytes and absent-file state then removes journal");
  check(put(path("workspace-import.pending"),"{broken")&&!WorkspaceJournal::recover(dir.path(),&error)&&get(path("settings.json"))==settings&&QFile::exists(path("workspace-import.pending")),"truncated journal remains preserved and never touches live data");
  QFile::remove(path("workspace-import.pending"));

  QProcess child;child.start(QCoreApplication::applicationFilePath(),{"--hold-profile-lock",dir.path()});check(child.waitForStarted(5000)&&wait([&]{return QFile::exists(path("lock-ready"));}),"separate process acquires profile transaction lock");
  QLockFile contender(path("desktop-profile.lock"));contender.setStaleLockTime(0);check(!contender.tryLock(0),"second process excluded during transaction");child.kill();check(child.waitForFinished(5000),"interrupted writer exits");check(contender.tryLock(1000),"dead process lock is recoverable");contender.unlock();

  MainWindow window(true);window.saveTimer_->stop();check(wait([&]{return window.registersReady_&&window.catalogReady_;}),"actual desktop worker initializes");
  check(window.saveSettings(),"actual desktop settings save succeeds");const auto primary=get(window.configPath_);
  QJsonObject interrupted;const auto windowDirectory=QFileInfo(window.configPath_).absolutePath();check(WorkspaceJournal::snapshot(windowDirectory,interrupted,&error)&&put(window.configPath_,"interrupted transaction"),"simulate a crashed peer after the window already opened");
  {auto transaction=window.lockProfile();check(transaction&&get(window.configPath_)==primary&&!QFile::exists(QDir(windowDirectory).filePath("workspace-import.pending")),"existing window recovers pending crash journal before becoming the next writer");}
  auto external=QJsonDocument::fromJson(primary).object();external["language"]="hu";check(put(window.configPath_,QJsonDocument(external).toJson()),"simulate another process saving settings");
  window.input_->setText("123456789+987654321");window.saveTimer_->stop();check(window.saveSettings(),"conflicting window gets a preserved snapshot");
  const auto live=QJsonDocument::fromJson(get(window.configPath_)).object();const auto preserved=QJsonDocument::fromJson(get(window.settingsConflictPath_)).object();
  check(live.value("language").toString()=="hu"&&preserved.value("language").toString()=="en"&&preserved.value("inputDrafts").toObject().value("10").toString()=="123456789+987654321","settings conflict preserves external preferences and local draft independently");
  check(!window.settingsConflictPath_.isEmpty()&&QFile::exists(window.settingsConflictPath_),"conflict snapshot has an explicit recoverable path");
  const auto conflictPath=window.settingsConflictPath_;const auto primaryBeforeRepeat=get(window.configPath_);
  window.input_->setText("246813579+(");window.saveTimer_->stop();
  check(window.saveSettings()&&window.settingsConflictPath_==conflictPath,"later conflicting save updates the same recoverable snapshot");
  check(get(window.configPath_)==primaryBeforeRepeat&&window.input_->text()=="246813579+("&&QJsonDocument::fromJson(get(conflictPath)).object().value("inputDrafts").toObject().value("10").toString()=="246813579+(","repeated conflict save preserves peer bytes and the latest incomplete local draft");

  const auto state=window.workbenchHub_->saveState();CalcContextSnapshot context;context.session.answer={"3",false};context.originalExpression="1+2";context.frozenExpression="1+2";context.committedCanonical="3";
  CalcResult result;result.ok=true;result.input="1+2";result.serialized="3";result.result="3";result.context=std::make_shared<const CalcContextSnapshot>(context);result.id="storage-fixture";
  CalcDefinition definition;definition.name="myfun";definition.kind=CalcDefinitionKind::Function;definition.parameters={"x"};definition.body="x^2+1";
  QJsonObject sections{{"history",QJsonArray{QJsonObject{{"expression","1+2"},{"result","3"},{"time","2026-10-12T08:00:00Z"},{"snapshotV2",WorkspaceModel::resultToJson(result)}}}},{"registers",QJsonObject{{"R0",QJsonObject{{"canonical","3"},{"approximate",false}}}}},{"values",QJsonObject{{"answer",QJsonObject{{"canonical","3"},{"approximate",false}}},{"memory",QJsonObject{{"canonical",""},{"approximate",false}}},{"selected",WorkspaceModel::resultToJson(result)}}},{"definitions",QJsonArray{WorkspaceModel::definitionToJson(definition)}},{"matrices",state.value("matrix")},{"datasets",state.value("data")},{"graphs",state.value("graph")},{"layouts",QJsonObject{{"keypad",state.value("keypad")},{"workflows",state.value("workflows")},{"programmer",state.value("programmer")},{"appliedKeypad",QJsonObject{}}}}};
  const auto encoded=WorkspaceModel::encode(sections,&error);QJsonObject decoded;check(!encoded.isEmpty()&&WorkspaceModel::decode(encoded,decoded,&error)&&decoded==sections,"portable workspace all eight sections round-trip unchanged");
  CalcResult restored;check(WorkspaceModel::resultFromJson(decoded["values"].toObject()["selected"].toObject(),restored,&error)&&restored.context&&restored.context->committedCanonical=="3"&&restored.context->originalExpression=="1+2","typed result and frozen canonical context round-trip");
  auto corrupted=QJsonDocument::fromJson(encoded).object();corrupted["sha256"]="00";QJsonObject unchanged{{"keep",true}};check(!WorkspaceModel::decode(QJsonDocument(corrupted).toJson(),unchanged,&error)&&unchanged==QJsonObject{{"keep",true}},"portable workspace checksum failure leaves candidate untouched");
  window.saveTimer_->stop();std::cout<<checks_-failures_<<'/'<<checks_<<" desktop storage checks passed\n";return failures_?1:0;
 }
};
int main(int argc,char **argv){
 QTemporaryDir profile;QList<QByteArray> args;for(int i=0;i<argc;++i)args<<argv[i];args<<"--config-root"<<profile.path().toUtf8();QList<char*> ptrs;for(auto &arg:args)ptrs<<arg.data();int count=ptrs.size();QApplication app(count,ptrs.data());app.setStyle("Fusion");
 if(app.arguments().value(1)=="--hold-profile-lock"){
  const auto dir=app.arguments().value(2);QLockFile lock(QDir(dir).filePath("desktop-profile.lock"));lock.setStaleLockTime(0);if(!lock.tryLock(0)||!put(QDir(dir).filePath("lock-ready"),"ready"))return 4;while(true)QThread::msleep(20);
 }
 return DesktopStorageTests().run();
}
