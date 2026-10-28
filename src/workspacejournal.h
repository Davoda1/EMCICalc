// SPDX-License-Identifier: MIT
#pragma once
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

// Caller holds desktop-profile.lock for the whole transaction or recovery.
// Validate every backup before touching any live file, including base64 and
// SHA-256, so a truncated/malformed journal cannot partially restore state.
namespace WorkspaceJournal {
inline QStringList names(){return {"settings.json","registers.json","history.qz"};}
inline bool fail(QString *error,const QString &message){if(error)*error=message;return false;}
inline bool validate(const QJsonObject &journal,QMap<QString,QByteArray> *decoded,QString *error){
 if(journal.size()!=2||journal.value("schema").toInt(-1)!=1||!journal.value("files").isObject())return fail(error,"Unsupported recovery journal schema.");
 const auto files=journal.value("files").toObject();if(files.size()!=3)return fail(error,"Incomplete recovery journal.");
 QMap<QString,QByteArray> values;qint64 total=0;
 for(const auto &name:names()){
  if(!files.value(name).isObject())return fail(error,"Incomplete recovery journal.");
  const auto entry=files.value(name).toObject();if(!entry.value("exists").isBool())return fail(error,"Invalid recovery journal entry.");
  if(!entry.value("exists").toBool()){if(entry.size()!=1)return fail(error,"Invalid absent-file journal entry.");continue;}
  if(entry.size()!=3||!entry.value("bytes").isString()||!entry.value("sha256").isString())return fail(error,"Invalid recovery payload.");
  const auto encoded=entry.value("bytes").toString().toLatin1();auto result=QByteArray::fromBase64Encoding(encoded,QByteArray::AbortOnBase64DecodingErrors);
  if(!result||result.decoded.toBase64()!=encoded)return fail(error,"Invalid recovery base64 data.");
  const auto digest=QCryptographicHash::hash(result.decoded,QCryptographicHash::Sha256).toHex();
  if(entry.value("sha256").toString().toLatin1()!=digest)return fail(error,"Recovery payload checksum mismatch.");
  total+=result.decoded.size();if(total>144LL*1024*1024)return fail(error,"Recovery payload exceeds its size limit.");
  values[name]=result.decoded;
 }
 if(decoded)*decoded=values;return true;
}
inline bool snapshot(const QString &directory,QJsonObject &journal,QString *error){
 QJsonObject files;
 for(const auto &name:names()){
  QFile file(QDir(directory).filePath(name));QJsonObject entry{{"exists",file.exists()}};
  if(file.exists()){
   if(file.size()>144LL*1024*1024||!file.open(QIODevice::ReadOnly))return fail(error,"Could not snapshot workspace file.");
   const auto bytes=file.readAll();entry["bytes"]=QString::fromLatin1(bytes.toBase64());entry["sha256"]=QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex());
  }
  files[name]=entry;
 }
 journal={{"schema",1},{"files",files}};if(!validate(journal,nullptr,error))return false;
 const auto bytes=QJsonDocument(journal).toJson(QJsonDocument::Compact);if(bytes.size()>192LL*1024*1024)return fail(error,"Recovery journal exceeds its size limit.");
 QSaveFile output(QDir(directory).filePath("workspace-import.pending"));output.setDirectWriteFallback(false);
 if(!output.open(QIODevice::WriteOnly)||output.write(bytes)!=bytes.size()||!output.commit())return fail(error,"Could not write the workspace recovery journal.");return true;
}
inline bool restore(const QString &directory,const QJsonObject &journal,QString *error){
 QMap<QString,QByteArray> decoded;if(!validate(journal,&decoded,error))return false;
 for(const auto &name:names()){
  const auto path=QDir(directory).filePath(name);
  if(!decoded.contains(name)){if(QFile::exists(path)&&!QFile::remove(path))return fail(error,"Could not restore absent workspace file.");continue;}
  const auto bytes=decoded.value(name);QSaveFile output(path);output.setDirectWriteFallback(false);
  if(!output.open(QIODevice::WriteOnly)||output.write(bytes)!=bytes.size()||!output.commit())return fail(error,"Could not restore workspace file.");
 }
 return true;
}
inline bool recover(const QString &directory,QString *error){
 QFile file(QDir(directory).filePath("workspace-import.pending"));if(!file.exists())return true;
 if(file.size()>192LL*1024*1024||!file.open(QIODevice::ReadOnly))return fail(error,"Recovery journal cannot be read.");
 QJsonParseError parse;auto journal=QJsonDocument::fromJson(file.readAll(),&parse);file.close();
 if(parse.error!=QJsonParseError::NoError||!journal.isObject())return fail(error,"Recovery journal is malformed.");
 if(!restore(directory,journal.object(),error))return false;
 if(!QFile::remove(file.fileName()))return fail(error,"Recovered journal could not be removed.");return true;
}
}
