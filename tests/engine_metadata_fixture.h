// SPDX-License-Identifier: MIT
#pragma once
#include "engine.h"
#include "matheditor.h"
#include <QThread>
#include <QObject>
// Production templates are driven by the shipped engine's immutable snapshot.
// Keep tests on the same contract, with the sole engine confined to its worker.
inline void installEngineMetadataFixture(){
    QThread worker;QObject owner;owner.moveToThread(&worker);worker.start();
    QVector<FunctionMetadata> metadata;
    QMetaObject::invokeMethod(&owner,[&]{CalcEngine engine;metadata=engine.functionMetadata();},Qt::BlockingQueuedConnection);
    worker.quit();worker.wait();MathEditor::setDefaultFunctionMetadata(metadata);
}
