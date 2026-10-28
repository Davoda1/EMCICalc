// SPDX-License-Identifier: MIT
// Native desktop archive conflict, lock, migration and snapshot retention gates.
#include "archivewritelock.h"
#include "historyarchive.h"
#include "registerarchive.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QProcess>
#include <QTemporaryDir>
#include <QTextStream>
#include <iostream>

int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    if (app.arguments().value(1)=="--hold-lock") {
        ArchiveWriteLock lock(app.arguments().value(2));
        if (!lock.tryLock(0)) return 3;
        QTextStream(stdout)<<"locked"<<Qt::endl;
        QTextStream input(stdin);input.readLine();return 0;
    }
    int checks=0,failures=0;
    auto check=[&](bool ok,const char *message){++checks;if(!ok){++failures;std::cerr<<"FAIL "<<message<<'\n';}};
    QTemporaryDir directory;check(directory.isValid(),"temporary profile available");
    const QString path=directory.path();
    QMap<QString,CalcStoredValue> firstValues,secondValues;
    RegisterArchive first,second;
    check(first.load(path,firstValues)&&second.load(path,secondValues),"both instances load initial register bank");
    firstValues["R0"]={"7/3",false};
    check(first.save(firstValues),"first instance atomically saves register");
    secondValues["R1"]={"11",false};
    check(!second.save(secondValues),"stale instance cannot overwrite another register transaction");
    check(second.load(path,secondValues)&&secondValues.value("R0").canonical=="7/3", "reload obtains preserved external register");
    secondValues["R1"]={"11",false};
    check(second.save(secondValues),"reloaded instance can deliberately extend bank");
    check(first.load(path,firstValues)&&firstValues.size()==2,"saved bank preserves both explicit values");
    QProcess writer;writer.start(app.applicationFilePath(),{"--hold-lock",path+"/registers.json.lock"});
    check(writer.waitForReadyRead(3000)&&writer.readAllStandardOutput().contains("locked"),"independent process acquires desktop lock");
    firstValues["R2"]={"17",false};
    check(!first.save(firstValues),"register save cannot bypass live process lock");
    writer.write("release\n");writer.closeWriteChannel();
    check(writer.waitForFinished(3000)&&writer.exitCode()==0,"independent lock holder releases normally");
    check(first.save(firstValues),"register save resumes after process lock release");
    HistoryArchive historyA,historyB;
    check(historyA.load(path)&&historyB.load(path),"both instances load empty history");
    auto row=[](QString value,int notation){return QJsonObject{{"expression",value},{"result",value},{"serialized",value}, {"time",QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},{"notation",notation},{"snapshotV2",QJsonObject{{"schema",2},{"proof","frozen extension retained"}}}};};
    check(historyA.append(row("12",3))&&historyA.save(),"FIX history record saves");
    check(historyB.append(row("13",4))&&historyB.save(),"SI history record merges another instance");
    HistoryArchive reopened;check(reopened.load(path)&&reopened.entries().size()==2,"concurrent history merge preserves both records");
    for(const auto &entry:reopened.entries())check(entry.toObject().value("snapshotV2").toObject().value("proof")=="frozen extension retained","historical snapshot extension survives compression and merge");
    QFile malformed(path+"/registers.json");check(malformed.open(QIODevice::WriteOnly),"malformed recovery fixture opens");malformed.write("not json");malformed.close();
    RegisterArchive recovered;QMap<QString,CalcStoredValue> empty;
    check(!recovered.load(path,empty)&&empty.isEmpty()&&recovered.error().contains("preserved"),"invalid legacy bank preserved before recovery");
    empty["R9"]={"-infinity",false};check(recovered.save(empty),"recovery can save valid extended-real register data");
    std::cout<<checks<<" checks, "<<failures<<" failures\n";return failures?1:0;
}
