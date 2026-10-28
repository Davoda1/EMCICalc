// SPDX-License-Identifier: MIT
#include "mainwindow.h"
#include "workspacehub.h"
#include "workspacemodel.h"
#include "workspacejournal.h"
#include "exportdocument.h"
#include "stepsengine.h"
#include "fontscale.h"
#include "inwindowdialog.h"
#include "desktopworkbench.h"
#include <QCryptographicHash>

namespace {
// Keep the confirm/cancel row visible even in a short landscape viewport.
QVBoxLayout *scrollableDialogBody(QDialog &dialog, QWidget *owner, QVBoxLayout *&outer) {
    dialog.setPalette(owner->palette());
    DesktopFont::apply(&dialog);
    outer=new QVBoxLayout(&dialog);auto *scroll=new QScrollArea(&dialog);scroll->setWidgetResizable(true);scroll->setFrameShape(QFrame::NoFrame);scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);auto *body=new QWidget(scroll);auto *layout=new QVBoxLayout(body);scroll->setWidget(body);outer->addWidget(scroll,1);
    const QSize bounds(qMax(240,owner->width()-24),qMax(180,owner->height()-24));dialog.setMaximumSize(bounds);dialog.resize(qMin(480,bounds.width()),qMin(620,bounds.height()));return layout;
}
bool snapshotFiles(const QString &directory,QJsonObject &backup,QString *error){return WorkspaceJournal::snapshot(directory,backup,error);}
bool restoreFiles(const QString &directory,const QJsonObject &backup,QString *error){return WorkspaceJournal::restore(directory,backup,error);}

}

void MainWindow::inspectWorkspace(QString expression,std::shared_ptr<const CalcContextSnapshot>context,std::function<void(CalcResult)>done,QString group){
 const auto selected=options();auto allowed=std::make_shared<std::atomic_bool>(true);const QString id=group.isEmpty()?QUuid::createUuid().toString():group;if(workspaceJobs_.contains(id))workspaceJobs_[id]->store(false);workspaceJobs_.insert(id,allowed);QPointer<MainWindow>guard(this);
 dispatch([this,guard,expression,context,selected,allowed,id,done](CalcEngine*e){CalcContextSnapshot snapshot=context?*context:e->captureContext(selected);if(context&&context->originalExpression.isEmpty()&&context->session.answer.canonical.isEmpty()){snapshot=e->captureContext(selected);snapshot.options.complexEnabled=context->options.complexEnabled; snapshot.assumptions=context->assumptions;}auto result=e->inspect(expression,snapshot,allowed);QMetaObject::invokeMethod(this,[guard,id,allowed,result,done]{if(!guard)return;if(guard->workspaceJobs_.value(id)==allowed)guard->workspaceJobs_.remove(id);if(!guard->closing_&&allowed->load())done(result);},Qt::QueuedConnection);});
}
void MainWindow::showWorkspace(QString id){if(!workbenchHub_){showPage(6);return;}showPage(10);workbenchHub_->open(id);if(id=="home"){auto *first=workbenchHub_->findChild<QPushButton*>(workbenchHub_->menuLayout()=="large"?"category_calculate":"quick_calculate");if(first)first->setFocus();}}
void MainWindow::setupWorkspace(){
 workspaceHost_=new QWidget;workspaceHost_->setObjectName("fullWorkspaceHost");workspaceHostLayout_=new QVBoxLayout(workspaceHost_);workspaceHostLayout_->setContentsMargins(0,0,0,0);pages_->addWidget(workspaceHost_);workbenchHub_=new WorkspaceHub;workspaceHostLayout_->addWidget(workbenchHub_);graph_->setParent(this);graph_->hide();decimalPlaces_=qBound(0,loadedSettings_.value("decimalPlacesV2").toInt(6),200);percentMode_=qBound(0,loadedSettings_.value("percentModeV2").toInt(),1);implicitMode_=qBound(0,loadedSettings_.value("implicitModeV2").toInt(),2);
 workbenchHub_->onChanged=[this]{if(saveTimer_)saveTimer_->start();};
 workbenchHub_->onPageChanged=[this]{updateAuxiliaryLayouts();};
 QString catalogRestoreError;if(!catalogTools_->restoreBrowserState(loadedSettings_.value("catalogBrowserV2").toObject(),&catalogRestoreError)&&!catalogRestoreError.isEmpty())restoreNotice_=catalogRestoreError;catalogTools_->onBrowserStateChanged=[this]{if(saveTimer_)saveTimer_->start();};
 workbenchHub_->onNavigate=[this](QString id){if(id=="calculate")showPage(0);else if(id=="finance"){showPage(2);if(toolboxTabs_)toolboxTabs_->setCurrentIndex(0);catalogTools_->setView(CalculatorTools::View::Statistics);catalogTools_->setFinanceVisible(true);showPage(7);}else if(id=="history")showPage(1);else if(id=="settings")showPage(3);else if(id=="guide")showPage(4);else if(id=="toolbox")showPage(2);};
 auto insert=[this](QString value){showPage(0);input_->insert(value);};
 InspectCallback inspect=[this](QString expression,std::shared_ptr<const CalcContextSnapshot> context,ResultCallback done){inspectWorkspace(expression,context,done);};
 workbenchHub_->details->onInspect=[this](QString expression,std::shared_ptr<const CalcContextSnapshot> context,ResultCallback done){inspectWorkspace(expression,context,done,"details-inspect");};
 workbenchHub_->guided->onInspect=[this](QString expression,std::shared_ptr<const CalcContextSnapshot> context,ResultCallback done){inspectWorkspace(expression,context,done,"guided-inspect");};
 workbenchHub_->definitions->onInspect=[this](QString expression,std::shared_ptr<const CalcContextSnapshot> context,ResultCallback done){inspectWorkspace(expression,context,done,"definitions-inspect");};
 workbenchHub_->details->onFormat=[this](CalcStoredValue value,CalcOptions selected,std::shared_ptr<const CalcContextSnapshot> context,ResultCallback done){
  auto allowed=std::make_shared<std::atomic_bool>(true);const QString id="details-inspect";if(workspaceJobs_.contains(id))workspaceJobs_[id]->store(false);workspaceJobs_[id]=allowed;QPointer<MainWindow> guard(this);
  dispatch([this,guard,value,selected,context,done,allowed,id](CalcEngine *engine){CalcResult result;
   if(context){auto snapshot=*context;snapshot.options=selected;snapshot.committedCanonical=value.canonical;snapshot.committedApproximate=value.approximate;result=engine->inspect(snapshot.originalExpression,snapshot,allowed);}
   else if(allowed->load())result=engine->formatValue(value.canonical,selected,value.approximate);
   QMetaObject::invokeMethod(this,[guard,result,done,allowed,id]{if(!guard)return;if(guard->workspaceJobs_.value(id)==allowed)guard->workspaceJobs_.remove(id);if(!guard->closing_&&allowed->load())done(result);},Qt::QueuedConnection);
  });
 };
 workbenchHub_->details->onInsert=insert;workbenchHub_->guided->onInsert=insert;workbenchHub_->definitions->onInsert=[this](QString value){showPage(0);if(QRegularExpression("^[A-Za-z_][A-Za-z0-9_]*\\($").match(value).hasMatch())input_->insertTemplate(value);else input_->insert(value);input_->setFocus();};
 workbenchHub_->details->onSteps=[this](QString e){workbenchHub_->steps->setExpression(selectedResult_.context&&!selectedResult_.context->frozenExpression.isEmpty()?selectedResult_.context->frozenExpression:e);workbenchHub_->open("steps");};
 workbenchHub_->details->onGraph=[this](QString expression,std::shared_ptr<const CalcContextSnapshot> context){workbenchHub_->graphs->addExpression(expression,context);workbenchHub_->open("graphs");};
 workbenchHub_->guided->onGraph=[this](QString expression){workbenchHub_->graphs->addExpression(expression);workbenchHub_->open("graphs");};
 workbenchHub_->guided->onSteps=[this](QString expression){workbenchHub_->steps->setExpression(expression);workbenchHub_->open("steps");};
 workbenchHub_->guided->onCancel=[this]{if(workspaceJobs_.contains("guided-inspect"))workspaceJobs_["guided-inspect"]->store(false);};
 workbenchHub_->details->onCancel=[this]{for(const auto &id:{"details-inspect","details-recalculate"})if(workspaceJobs_.contains(id))workspaceJobs_[id]->store(false);};
 workbenchHub_->details->onConvertQuantity=[this](CalcStoredValue value,QString target,std::shared_ptr<const CalcContextSnapshot> context,ResultCallback done){
  const QString id="details-inspect";if(workspaceJobs_.contains(id))workspaceJobs_[id]->store(false);auto allowed=std::make_shared<std::atomic_bool>(true);workspaceJobs_[id]=allowed;const auto selected=options();
  dispatch([this,value,target,context,done,allowed,id,selected](CalcEngine *engine){const auto current=engine->captureContext(selected);CalcResult result;QString error;
   if(allowed->load()&&(!context||engine->restoreContext(*context,&error))){const auto settings=context?context->options:selected;result=engine->formatUnitConversion(value.canonical,target,settings,value.approximate);}
   else result.error=error;
   if(!engine->restoreContext(current,&error)){result=CalcResult{};result.error=error;}
   QMetaObject::invokeMethod(this,[this,result,done,allowed,id]{if(workspaceJobs_.value(id)==allowed)workspaceJobs_.remove(id);if(!closing_&&allowed->load())done(result);},Qt::QueuedConnection);
  });
 };
 workbenchHub_->details->onRecalculate=[this](std::shared_ptr<const CalcContextSnapshot> context,QString expression,bool historical,ResultCallback done){
  if(busy_){CalcResult result;result.error=trc("Wait for the current calculation.");done(result);return;}
  const QString id="details-recalculate";if(workspaceJobs_.contains(id))workspaceJobs_[id]->store(false);auto allowed=std::make_shared<std::atomic_bool>(true);workspaceJobs_[id]=allowed;const auto selected=options();setBusy(true);
  dispatch([this,context,expression,historical,done,id,allowed,selected](CalcEngine*engine){
   const auto before=engine->captureContext(selected);auto candidate=before.session;CalcResult result;
   if(historical){if(context){auto frozen=*context;frozen.committedCanonical.clear();result=engine->inspect(frozen.originalExpression,frozen,allowed);if(result.ok){auto snapshot=std::make_shared<CalcContextSnapshot>(*result.context);snapshot->committedCanonical=result.serialized;snapshot->committedApproximate=result.valueApproximate;result.context=snapshot;result.id=QUuid::createUuid().toString(QUuid::WithoutBraces);candidate.answer={result.serialized,result.valueApproximate};}}else result.error=QStringLiteral("This legacy result has no saved context.");}
   else {result=engine->evaluate(expression,selected,allowed);candidate=engine->sessionState();}
   QString error;if(!engine->restoreContext(before,&error)){result=CalcResult{};result.error=error;}
   QMetaObject::invokeMethod(this,[this,result,candidate,done,id,allowed]{
    if(closing_)return;
    if(!allowed->load()){setBusy(false);return;}
    setBusy(false);
    if(!result.ok){done(result);return;}
    if(!commitWorkbench(result,candidate)){CalcResult failed;failed.error=trc("Could not save the calculation.");done(failed);return;}
    setBusy(true);dispatch([this,candidate,done,result](CalcEngine*engine){QString error;const bool ok=engine->restoreSessionState(candidate,&error);QMetaObject::invokeMethod(this,[this,ok,error,done,result]{if(closing_)return;setBusy(false);if(ok)done(result);else{configSaveBlocked_=true;CalcResult failed;failed.error=error;done(failed);}},Qt::QueuedConnection);});
   },Qt::QueuedConnection);
  });
 };

 workbenchHub_->guided->onAcceptTransform=[this](QString before,QString after){if(!input_->hasSelectedText()||input_->selectedText()!=before){setStatus(trc("The selection changed; select it again."));return;}input_->insert(after);showPage(0);};
 workbenchHub_->definitions->onReplace=[this](QVector<CalcDefinition>defs,std::function<void(bool,QString)>done){
  if(busy_){done(false,trc("Wait for the current calculation."));return;}auto transaction=lockProfile();if(!transaction){done(false,trc("Another window is saving this profile. Try again shortly."));return;}
  const auto oldDefs=definitions_;setBusy(true);
  dispatch([this,defs,oldDefs,done,transaction](CalcEngine*engine){QString error;const bool ok=engine->replaceDefinitions(defs,&error);const auto metadata=engine->functionMetadata();const auto entries=engine->catalog();
   QMetaObject::invokeMethod(this,[this,defs,oldDefs,done,ok,error,metadata,entries,transaction]{if(closing_)return;
    if(!ok){setBusy(false);done(false,error);return;}
    definitions_=defs;if(saveSettings()){applyFunctionMetadata(metadata,entries);setBusy(false);done(true,{});return;}
    definitions_=oldDefs;dispatch([this,oldDefs,done,transaction](CalcEngine*engine){QString error;const bool restored=engine->replaceDefinitions(oldDefs,&error);QMetaObject::invokeMethod(this,[this,restored,error,done,transaction]{if(closing_)return;setBusy(false);if(!restored)configSaveBlocked_=true;done(false,error.isEmpty()?trc("Could not save the calculation."):error);},Qt::QueuedConnection);});
   },Qt::QueuedConnection);
  });
 };
 QString restoreError;for(auto item:loadedSettings_.value("definitionsV2").toArray()){CalcDefinition d;if(!WorkspaceModel::definitionFromJson(item.toObject(),d,&restoreError)){definitions_.clear();break;}definitions_.append(d);}const auto definitions=definitions_;dispatch([this,definitions](CalcEngine*e){QString err;const bool ok=e->replaceDefinitions(definitions,&err);const auto metadata=e->functionMetadata();const auto entries=e->catalog();QMetaObject::invokeMethod(this,[this,ok,err,definitions,metadata,entries]{if(closing_)return;if(ok){workbenchHub_->definitions->setDefinitions(definitions);applyFunctionMetadata(metadata,entries);}else{definitions_.clear();setStatus(err);}},Qt::QueuedConnection);});
 if(!workbenchHub_->restoreState(loadedSettings_.value("workspaceV2").toObject(),&restoreError)&&!restoreError.isEmpty())restoreNotice_=restoreError;
 if(!loadedSettings_.contains("workspaceV2")){
  auto legacy=loadedSettings_.value("graphState").toObject();if(legacy.isEmpty()&&loadedSettings_.value("graphs").isArray())legacy={{"functions",loadedSettings_.value("graphs")},{"xMin",-10},{"xMax",10},{"yMin",-6},{"yMax",6}};
  const auto functions=legacy.value("functions").toArray();EmciGraph::Request migrated;
  migrated.bounds={legacy.value("xMin").toDouble(-10),legacy.value("xMax").toDouble(10),legacy.value("yMin").toDouble(-6),legacy.value("yMax").toDouble(6)};
  if(!migrated.bounds.valid())migrated.bounds=EmciGraph::Bounds{};
  bool valid=legacy.value("functions").isArray()&&functions.size()<=3;int index=0;
  for(const auto &value:functions){if(!value.isString()||value.toString().size()>4096){valid=false;break;}if(value.toString().trimmed().isEmpty())continue;EmciGraph::Series curve;curve.id=QString("migrated-%1").arg(index);curve.name=QString("y%1").arg(index+1);curve.expression=value.toString();curve.angle=angle_;curve.color=QColor(QStringList{"#3388dd","#df7734","#52a276"}.value(index));migrated.series.append(curve);++index;}
  if(valid&&!migrated.series.isEmpty()){migrated.tableSeries=migrated.series.first().id;migrated.tableStart=migrated.bounds.xmin;migrated.tableEnd=migrated.bounds.xmax;workbenchHub_->graphs->restoreState(EmciGraph::saveRequest(migrated),&restoreError);}
 }
 const auto applied=loadedSettings_.value("customKeypadAppliedV2").toObject();
 if(!applied.isEmpty()){CustomKeypad::Layout layout;if(CustomKeypad::fromJson(applied,&layout,&restoreError)&&calculator_->setCustomLayout(layout,&restoreError))keys_=calculator_->keyButtons();else restoreNotice_=restoreError;}

 WorkspaceModel::resultFromJson(loadedSettings_.value("selectedResultV2").toObject(),selectedResult_,&restoreError);workbenchHub_->setResult(selectedResult_);if(selectedResult_.ok){lastSerialized_=selectedResult_.serialized;lastValueApproximate_=selectedResult_.valueApproximate;commitDisplay(selectedResult_);}
 workbenchHub_->graphs->onChanged=workbenchHub_->onChanged;
 workbenchHub_->graphs->onSampleRequested=[this](EmciGraph::Request request,std::shared_ptr<std::atomic_bool>cancel){
  const auto selected=options();auto allowed=std::make_shared<std::atomic_bool>(true);if(workspaceJobs_.contains("graph"))workspaceJobs_["graph"]->store(false);workspaceJobs_["graph"]=allowed;
  dispatch([this,request,cancel,selected,allowed](CalcEngine*e){
   const auto original=e->captureContext(selected);auto activeOptions=selected;
   auto activate=[e,original,&activeOptions](const std::shared_ptr<const CalcContextSnapshot>&context,QString *error){const auto &snapshot=context?*context:original;activeOptions=snapshot.options;return e->restoreContext(snapshot,error);};
   auto evaluator=[e,cancel,&activeOptions,allowed](const QString&s,const QMap<QString,double>&bindings,int angle){if(cancel->load()||!allowed->load())return EmciGraph::ScalarResult{};auto opt=activeOptions;opt.angle=angle;auto r=e->inspectScalar(s,bindings,opt,allowed);return EmciGraph::ScalarResult{r.ok,r.value,r.error};};
   auto result=EmciGraph::sample(request,evaluator,cancel,activate);QString rollback;
   if(!e->restoreContext(original,&rollback)){result.status=EmciGraph::Status::Invalid;result.message=rollback;result.series.clear();result.table.clear();}
   QMetaObject::invokeMethod(this,[this,result,allowed]{if(!closing_&&allowed->load())workbenchHub_->graphs->acceptResult(result);},Qt::QueuedConnection);
  });
 };
 workbenchHub_->steps->onDeriveRequested=[this](StepsRequest request){if(workspaceJobs_.contains("steps"))workspaceJobs_["steps"]->store(false);if(workspaceCancelFlags_.contains("steps"))workspaceCancelFlags_["steps"]->store(true);auto cancel=std::make_shared<std::atomic_bool>(false);workspaceCancelFlags_["steps"]=cancel;auto allowed=std::make_shared<std::atomic_bool>(true);workspaceJobs_["steps"]=allowed;dispatch([this,request,allowed,cancel](CalcEngine*){auto r=StepsEngine::derive(request,cancel.get());QMetaObject::invokeMethod(this,[this,r,allowed]{if(!closing_&&allowed->load())workbenchHub_->steps->setResult(r);},Qt::QueuedConnection);});};workbenchHub_->steps->onCancelRequested=[this]{if(workspaceJobs_.contains("steps"))workspaceJobs_["steps"]->store(false);if(workspaceCancelFlags_.contains("steps"))workspaceCancelFlags_["steps"]->store(true);};
 workbenchHub_->steps->onExportRequested=[this](QString format,QByteArray bytes){QString error;auto r=EmciExport::save(this,trc("Export"),"EMCICalc-Steps."+format,format.toUpper()+" (*."+format+")",bytes,&error);if(r==EmciExport::Result::Failed)setStatus(error);};
 workbenchHub_->matrix->onCompute=[this](WorkbenchData::MatrixRequest request){if(workspaceJobs_.contains("matrix"))workspaceJobs_["matrix"]->store(false);if(workspaceCancelFlags_.contains("matrix"))workspaceCancelFlags_["matrix"]->store(true);auto cancel=std::make_shared<std::atomic_bool>(false);workspaceCancelFlags_["matrix"]=cancel;auto allowed=std::make_shared<std::atomic_bool>(true);workspaceJobs_["matrix"]=allowed;dispatch([this,request,allowed,cancel](CalcEngine*e){auto r=WorkbenchData::matrix(*e,request,cancel);QMetaObject::invokeMethod(this,[this,r,allowed]{if(!closing_&&allowed->load())workbenchHub_->matrix->setResult(r);},Qt::QueuedConnection);});};
 workbenchHub_->data->onCompute=[this](WorkbenchData::DataRequest request){if(workspaceJobs_.contains("data"))workspaceJobs_["data"]->store(false);if(workspaceCancelFlags_.contains("data"))workspaceCancelFlags_["data"]->store(true);auto cancel=std::make_shared<std::atomic_bool>(false);workspaceCancelFlags_["data"]=cancel;auto allowed=std::make_shared<std::atomic_bool>(true);workspaceJobs_["data"]=allowed;dispatch([this,request,allowed,cancel](CalcEngine*e){auto r=WorkbenchData::statistics(*e,request,cancel);QMetaObject::invokeMethod(this,[this,r,allowed]{if(!closing_&&allowed->load())workbenchHub_->data->setResult(r);},Qt::QueuedConnection);});};
 workbenchHub_->matrix->onCancel=[this](quint64){if(workspaceJobs_.contains("matrix"))workspaceJobs_["matrix"]->store(false);if(workspaceCancelFlags_.contains("matrix"))workspaceCancelFlags_["matrix"]->store(true);};workbenchHub_->data->onCancel=[this](quint64){if(workspaceJobs_.contains("data"))workspaceJobs_["data"]->store(false);if(workspaceCancelFlags_.contains("data"))workspaceCancelFlags_["data"]->store(true);};workbenchHub_->matrix->onInsert=insert;workbenchHub_->data->onInsert=insert;workbenchHub_->matrix->onChanged=workbenchHub_->onChanged;workbenchHub_->data->onChanged=workbenchHub_->onChanged;
 auto exportCsv=[this](QString csv){QString error;if(EmciExport::save(this,trc("Export"),"EMCICalc.csv","CSV (*.csv)",csv.toUtf8(),&error)==EmciExport::Result::Failed)setStatus(error);};workbenchHub_->matrix->onExportCsv=exportCsv;workbenchHub_->data->onExportCsv=exportCsv;
 auto readCsv=[this](bool matrix){auto name=QFileDialog::getOpenFileName(this,trc("Import CSV"),{},trc("CSV (*.csv)"));if(name.isEmpty())return;QFile f(name);if(!f.open(QIODevice::ReadOnly)||f.size()>8*1024*1024){setStatus(trc("Could not read CSV"));return;}QString error;auto text=QString::fromUtf8(f.readAll());bool ok=matrix?workbenchHub_->matrix->importCsv(text,&error):workbenchHub_->data->importCsv(text,&error);if(!ok)setStatus(error);};workbenchHub_->matrix->onImportCsv=[readCsv]{readCsv(true);};workbenchHub_->data->onImportCsv=[readCsv]{readCsv(false);};
 workbenchHub_->workflows->onEvaluate=[this](WorkflowRequest request){
  if(busy_){workbenchHub_->workflows->workflows().complete(request.id,{},trc("Wait for the current calculation."));return;}
  if(workspaceJobs_.contains("workflow"))workspaceJobs_["workflow"]->store(false);
  auto allowed=std::make_shared<std::atomic_bool>(true);workspaceJobs_["workflow"]=allowed;
  const auto workflowBefore=workbenchHub_->workflows->saveState();auto selected=options();selected.workflow=int(workbenchHub_->workflows->workflows().mode());
  setBusy(true);
  dispatch([this,request,allowed,selected,workflowBefore](CalcEngine*e){
   const auto before=e->sessionState();
   auto result=request.commit?e->evaluate(request.expression,selected,allowed):e->inspect(request.expression,selected,allowed);
   const auto candidate=e->sessionState();
   if(request.commit){QString error;if(!e->restoreSessionState(before,&error)){result=CalcResult{};result.error=error;}}
   QMetaObject::invokeMethod(this,[this,request,allowed,result,candidate,workflowBefore]{
    if(closing_)return;
    if(!allowed->load()){setBusy(false);return;}
    if(!request.commit)setBusy(false); // completion may synchronously request a final Classic operation
    const bool accepted=workbenchHub_->workflows->workflows().complete(request.id,result.serialized,result.ok?QString():result.error);
    if(workspaceJobs_.value("workflow")!=allowed)return;
    if(accepted&&result.ok&&request.commit){
     setBusy(false);
     if(commitWorkbench(result,candidate)){
      setBusy(true);
      dispatch([this,candidate](CalcEngine*engine){QString error;const bool ok=engine->restoreSessionState(candidate,&error);QMetaObject::invokeMethod(this,[this,ok,error]{if(closing_)return;setBusy(false);if(!ok){configSaveBlocked_=true;setStatus(error);}},Qt::QueuedConnection);});
      return;
     }
     workbenchHub_->workflows->restoreState(workflowBefore);
     return;
    }
    setBusy(false);
   },Qt::QueuedConnection);
  });
 };
 workbenchHub_->workflows->onCancel=[this](quint64){if(workspaceJobs_.contains("workflow"))workspaceJobs_["workflow"]->store(false);};workbenchHub_->workflows->onInsert=insert;workbenchHub_->workflows->onExpressionMode=[this]{showPage(0);};workbenchHub_->workflows->onStateChanged=workbenchHub_->onChanged;workbenchHub_->programmer->onInsert=insert;workbenchHub_->programmer->onStateChanged=workbenchHub_->onChanged;
 workbenchHub_->keypad->onApply=[this](const CustomKeypad::Layout &layout){QString error;if(!calculator_->setCustomLayout(layout,&error)){setStatus(error);return;}keys_=calculator_->keyButtons();showPage(0);saveTimer_->start();};workbenchHub_->keypad->onReset=[this]{calculator_->clearCustomLayout();keys_=calculator_->keyButtons();saveTimer_->start();};workbenchHub_->keypad->onStateChanged=workbenchHub_->onChanged;
 workbenchHub_->onExport=[this]{exportWorkspace();};workbenchHub_->onImport=[this]{importWorkspace();};workbenchHub_->retranslate([this](const char*s){return trc(s);});
 // A focusable desktop context action transforms an explicit structured selection.
 auto*selectionAction=new QAction(trc("Transform selection"),input_);selectionAction->setProperty("workspaceLabel","Transform selection");selectionAction->setShortcut(QKeySequence("Ctrl+Shift+T"));selectionAction->setShortcutContext(Qt::WidgetShortcut);input_->addAction(selectionAction);connect(selectionAction,&QAction::triggered,this,[this]{workbenchHub_->guided->setSelection(input_->text(),input_->selectedText());showWorkspace("guided");});

 workbenchHub_->setMenuLayout(loadedSettings_.value("menuLayoutV2").toString("large"));if(!loadedSettings_.contains("workspaceV2"))workbenchHub_->open("graphs");

}
bool MainWindow::commitWorkbench(const CalcResult &result,const CalcSessionState &session){
 auto transaction=lockProfile();if(!transaction)return false;
 if(!result.ok)return false;
 const bool wasEnabled=workbenchHub_->isEnabled();workbenchHub_->setEnabled(false);
 const QString directory=QFileInfo(configPath_).absolutePath();QString problem;QJsonObject backup;
 if(!snapshotFiles(directory,backup,&problem)){workbenchHub_->setEnabled(wasEnabled);setStatus(problem);return false;}
 const auto oldHistory=history_;const auto oldArchive=historyArchive_;const auto oldRegisterArchive=registerArchive_;
 const auto oldBank=registerValues_;const auto oldAnswer=answerState_;const auto oldSelected=selectedResult_;
 const auto oldMemory=memoryValue_;const bool oldApprox=memoryValueApproximate_;
 const QJsonObject row{{"expression",result.input},{"result",result.result},{"approximate",result.approximate},{"resultApproximate",result.resultApproximate},{"approximateApproximate",result.approximateApproximate},{"angle",result.context?result.context->options.angle:angle_},{"base",result.context?result.context->options.numberBase:numberBase_},{"notation",result.context?result.context->options.notation:notation_},{"exact",result.context?result.context->options.exact:exact_},{"complexEnabled",result.context?result.context->options.complexEnabled:complexEnabled_},{"resultRepeatStart",result.resultRepeatStart},{"resultRepeatLength",result.resultRepeatLength},{"resultCopyValue",result.resultCopyValue},{"approximateRepeatStart",result.approximateRepeatStart},{"approximateRepeatLength",result.approximateRepeatLength},{"approximateCopyValue",result.approximateCopyValue},
  {"serialized",result.serialized},{"valueApproximate",result.valueApproximate},
  {"time",QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},{"snapshotV2",WorkspaceModel::resultToJson(result)}};
 const bool historyWritten=historyArchive_.append(row)&&historyArchive_.save();
 bool saved=historyWritten&&registerArchive_.save(session.registers);
 if(saved){selectedResult_=result;answerState_=session.answer;memoryValue_=session.memorySet?session.memory.canonical:QString();memoryValueApproximate_=session.memory.approximate;registerValues_=session.registers;history_=historyArchive_.entries();saved=saveSettings();}
 if(!saved){
  problem=historyArchive_.error();if(problem.isEmpty())problem=registerArchive_.error();if(problem.isEmpty())problem=trc("Could not save the calculation.");
  // A refused archive lock means no live file changed. Never rewrite a file
  // owned by the competing writer merely to restore identical backup bytes.
  const bool restored=!historyWritten||restoreFiles(directory,backup,&problem);
  history_=oldHistory;historyArchive_=oldArchive;registerArchive_=oldRegisterArchive;registerValues_=oldBank;answerState_=oldAnswer;selectedResult_=oldSelected;memoryValue_=oldMemory;memoryValueApproximate_=oldApprox;
  if(restored){QFile::remove(QDir(directory).filePath("workspace-import.pending"));QMap<QString,CalcStoredValue> fresh;RegisterArchive archive;if(archive.load(directory,fresh)){registerArchive_=archive;registerValues_=fresh;dispatch([fresh](CalcEngine*engine){engine->restoreRegisters(fresh);});refreshRegisters();}}else configSaveBlocked_=true;
  workbenchHub_->setEnabled(wasEnabled);setStatus(problem);return false;
 }
 QFile::remove(QDir(directory).filePath("workspace-import.pending"));workbenchHub_->setResult(result);refreshRegisters();fillHistory();workbenchHub_->setEnabled(wasEnabled);return true;
}

void MainWindow::exportWorkspace(){
 QJsonObject sections;sections["history"]=history_;QJsonObject regs;for(auto i=registerValues_.begin();i!=registerValues_.end();++i)regs[i.key()]=QJsonObject{{"canonical",i->canonical},{"approximate",i->approximate}};sections["registers"]=regs;QJsonArray defs;for(const auto&d:definitions_)defs.append(WorkspaceModel::definitionToJson(d));sections["definitions"]=defs;
 sections["values"]=QJsonObject{{"answer",QJsonObject{{"canonical",answerState_.canonical},{"approximate",answerState_.approximate}}},{"memory",QJsonObject{{"canonical",memoryValue_},{"approximate",memoryValueApproximate_}}},{"selected",WorkspaceModel::resultToJson(selectedResult_)}};
 const auto state=workbenchHub_->saveState();sections["matrices"]=state.value("matrix");sections["datasets"]=state.value("data");sections["graphs"]=state.value("graph");sections["layouts"]=QJsonObject{{"keypad",workbenchHub_->keypad->saveState()},{"workflows",workbenchHub_->workflows->saveState()},{"programmer",workbenchHub_->programmer->saveState()},{"appliedKeypad",calculator_->hasCustomLayout()?CustomKeypad::toJson(calculator_->customLayout()):QJsonObject{}}};
 InWindowDialog choices(this);choices.setObjectName("workspaceExportSections");choices.setWindowTitle(trc("Export workspace"));QVBoxLayout *outer=nullptr;auto*l=scrollableDialogBody(choices,this,outer);QMap<QString,QCheckBox*>checks;for(const auto&k:WorkspaceModel::sectionNames()){auto*c=new QCheckBox(trc(k.toUtf8().constData()));c->setChecked(true);c->setMinimumHeight(DesktopWorkbench::controlHeight(c));checks[k]=c;l->addWidget(c);}auto*b=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel);outer->addWidget(b);connect(b,&QDialogButtonBox::accepted,&choices,&QDialog::accept);connect(b,&QDialogButtonBox::rejected,&choices,&QDialog::reject);if(choices.exec()!=QDialog::Accepted)return;for(auto i=checks.begin();i!=checks.end();++i)if(!i.value()->isChecked())sections.remove(i.key());QString error;const auto bytes=WorkspaceModel::encode(sections,&error);if(bytes.isEmpty()){setStatus(error);return;}if(EmciExport::save(this,trc("Export workspace"),"EMCICalc-workspace.emci","EMCICalc workspace (*.emci)",bytes,&error)==EmciExport::Result::Failed)setStatus(error);
}
void MainWindow::importWorkspace(){
 if(busy_){setStatus(trc("Wait for the current calculation."));return;}
 const auto filename=QFileDialog::getOpenFileName(this,trc("Import workspace"),{},"EMCICalc workspace (*.emci)");if(filename.isEmpty())return;QFile file(filename);if(!file.open(QIODevice::ReadOnly)||file.size()>32*1024*1024){setStatus(trc("Workspace could not be read or exceeds 32 MiB."));return;}QString error;QJsonObject sections;if(!WorkspaceModel::decode(file.readAll(),sections,&error)){setStatus(error);return;}
 InWindowDialog preview(this);preview.setObjectName("workspaceImportSections");preview.setWindowTitle(trc("Import workspace"));QVBoxLayout *outer=nullptr;auto*l=scrollableDialogBody(preview,this,outer);auto*note=new QLabel(trc("Choose sections to import. Saved formulas are validated without execution."));note->setWordWrap(true);l->addWidget(note);QMap<QString,QCheckBox*>checks;for(auto it=sections.begin();it!=sections.end();++it){auto*c=new QCheckBox(trc(it.key().toUtf8().constData()));c->setChecked(true);c->setMinimumHeight(DesktopWorkbench::controlHeight(c));checks[it.key()]=c;l->addWidget(c);}auto*conflict=new QComboBox;conflict->addItems({trc("Merge; keep existing names"),trc("Replace selected sections"),trc("Merge; rename incoming names")});conflict->setMinimumHeight(DesktopWorkbench::controlHeight(conflict));l->addWidget(conflict);auto*buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);outer->addWidget(buttons);connect(buttons,&QDialogButtonBox::accepted,&preview,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&preview,&QDialog::reject);if(preview.exec()!=QDialog::Accepted)return;for(auto it=checks.begin();it!=checks.end();++it)if(!it.value()->isChecked())sections.remove(it.key());const int policy=conflict->currentIndex();
 auto defs=policy==1&&sections.contains("definitions")?QVector<CalcDefinition>{}:definitions_;
 if(sections.contains("definitions")){
  if(!sections.value("definitions").isArray()||sections.value("definitions").toArray().size()>256){setStatus(trc("Invalid definitions."));return;}
  QVector<CalcDefinition> incoming;QSet<QString> incomingNames,reserved;
  for(const auto &d:defs)reserved.insert(d.name.toCaseFolded());
  for(const auto &value:sections.value("definitions").toArray()){
   CalcDefinition d;if(!value.isObject()||!WorkspaceModel::definitionFromJson(value.toObject(),d,&error)||incomingNames.contains(d.name.toCaseFolded())){setStatus(trc("Invalid definitions."));return;}
   incomingNames.insert(d.name.toCaseFolded());incoming.append(d);
  }
  QMap<QString,QString>renames;QVector<CalcDefinition>accepted;
  for(auto d:incoming){
   const bool collision=reserved.contains(d.name.toCaseFolded());if(collision&&policy==0)continue;
   if(collision&&policy==2){const auto original=d.name;int n=2;do{d.name=original+QString::number(n++);}while(reserved.contains(d.name.toCaseFolded())||incomingNames.contains(d.name.toCaseFolded()));renames.insert(original,d.name);}
   reserved.insert(d.name.toCaseFolded());accepted.append(d);
  }
  for(auto &d:accepted){
   // A single lexical pass avoids cascading a->a2->a22 replacements.
   QString body;int offset=0;auto matches=QRegularExpression("[A-Za-z_][A-Za-z0-9_]*").globalMatch(d.body);
   while(matches.hasNext()){const auto match=matches.next();body+=d.body.mid(offset,match.capturedStart()-offset);const auto name=match.captured();body+=d.parameters.contains(name)?name:renames.value(name,name);offset=match.capturedEnd();}
   d.body=body+d.body.mid(offset);defs.append(d);
  }
 }
 QMap<QString,CalcStoredValue>bank=policy==1&&sections.contains("registers")?QMap<QString,CalcStoredValue>{}:registerValues_;
 if(sections.contains("registers")){if(!sections.value("registers").isObject()||sections.value("registers").toObject().size()>10){setStatus(trc("Invalid registers."));return;}auto source=sections.value("registers").toObject();for(auto i=source.begin();i!=source.end();++i){auto o=i.value().toObject();if(!QRegularExpression("^R[0-9]$").match(i.key()).hasMatch()||!o.value("canonical").isString()||!o.value("approximate").isBool()||o.value("canonical").toString().size()>8*1024*1024){setStatus(trc("Invalid registers."));return;}if(policy==1||!bank.contains(i.key()))bank[i.key()]={o.value("canonical").toString(),o.value("approximate").toBool()};}}
 QJsonArray importedHistory;if(sections.contains("history")){if(!sections.value("history").isArray()||sections.value("history").toArray().size()>HistoryArchive::Limit){setStatus(trc("Invalid history."));return;}importedHistory=sections.value("history").toArray();for(auto v:importedHistory){auto o=v.toObject();if(!v.isObject()||!o.value("expression").isString()||!o.value("result").isString()||o.value("expression").toString().size()>8192){setStatus(trc("Invalid history."));return;}if(o.contains("snapshotV2")){CalcResult r;if(!WorkspaceModel::resultFromJson(o.value("snapshotV2").toObject(),r,&error)){setStatus(error);return;}}}}
 const auto oldState=workbenchHub_->saveState();auto nextState=oldState;
 const auto oldApplied=calculator_->hasCustomLayout()?CustomKeypad::toJson(calculator_->customLayout()):QJsonObject{};auto nextApplied=oldApplied;
 for(auto pair:QList<QPair<QString,QString>>{{"matrices","matrix"},{"datasets","data"},{"graphs","graph"}})if(sections.contains(pair.first)){
  if(!sections.value(pair.first).isObject()){setStatus(trc("Invalid workspace section."));return;}
  auto incoming=sections.value(pair.first).toObject();
  if(policy!=1){auto current=oldState.value(pair.second).toObject();
   if(pair.first=="graphs"){
    auto series=current.value("series").toArray();QSet<QString>names,ids;for(const auto &value:series){names.insert(value.toObject().value("name").toString());ids.insert(value.toObject().value("id").toString());}
    for(const auto &value:incoming.value("series").toArray()){auto curve=value.toObject();const auto original=curve.value("name").toString();bool collision=names.contains(original)||ids.contains(curve.value("id").toString());if(collision&&policy==0)continue;
     if(collision){QString name=original;int n=2;while(names.contains(name))name=original+QString::number(n++);curve["name"]=name;curve["id"]=QUuid::createUuid().toString(QUuid::WithoutBraces);}
     names.insert(curve.value("name").toString());ids.insert(curve.value("id").toString());series.append(curve);
    }current["series"]=series;
   }else{
    auto named=current.value("named").toObject();auto imported=incoming.value("named").toObject();
    for(auto it=imported.begin();it!=imported.end();++it){QString name=it.key();if(named.contains(name)&&policy==0)continue;if(named.contains(name)){int n=2;while(named.contains(name))name=it.key()+QString::number(n++);}named[name]=it.value();}
    // Preserve an incoming unsaved grid as an explicitly named imported item.
    const QString stem=incoming.value("name").toString().isEmpty()?QStringLiteral("Imported"):incoming.value("name").toString();QString name=stem;int n=2;while(named.contains(name))name=stem+QString::number(n++);
    if(pair.first=="matrices")named[name]=incoming.value("a");else{incoming.remove("named");named[name]=incoming;}
    current["named"]=named;
   }
   incoming=current;
  }
  nextState[pair.second]=incoming;
 }
 if(sections.contains("layouts")){
  if(!sections.value("layouts").isObject()){setStatus(trc("Invalid workspace layouts."));return;}
  const auto layouts=sections.value("layouts").toObject();
  if(layouts.contains("appliedKeypad")){if(!layouts.value("appliedKeypad").isObject()){setStatus(trc("Invalid workspace layouts."));return;}nextApplied=layouts.value("appliedKeypad").toObject();if(!nextApplied.isEmpty()){CustomKeypad::Layout layout;if(!CustomKeypad::fromJson(nextApplied,&layout,&error)||!CustomKeypad::validate(layout).isEmpty()||layout.stock){setStatus(trc("Invalid workspace layouts."));return;}}}
  for(const auto &key:{"keypad","workflows","programmer"}){if(!layouts.value(key).isObject()){setStatus(trc("Invalid workspace layouts."));return;}nextState[key]=layouts.value(key);}
 }
 // Validate page schemas on a separate native object without registering any
 // expression or applying any live user state.
 WorkspaceHub candidate;if(!candidate.restoreState(nextState,&error)){setStatus(error);return;}
 const auto oldDefs=definitions_;const auto oldBank=registerValues_;const auto oldHistory=history_;const auto oldArchive=historyArchive_;const auto oldRegisterArchive=registerArchive_;const auto oldSelected=selectedResult_;const auto oldAnswer=answerState_;const QString oldMemory=memoryValue_;const bool oldMemoryApprox=memoryValueApproximate_;
 CalcSessionState newSession;newSession.answer=answerState_;newSession.memory={memoryValue_,memoryValueApproximate_};newSession.memorySet=!memoryValue_.isEmpty();newSession.registers=bank;
 CalcResult importedSelected=selectedResult_;if(sections.contains("values")){auto value=sections.value("values").toObject();auto a=value.value("answer").toObject(),m=value.value("memory").toObject();if(!sections.value("values").isObject()||!value.value("answer").isObject()||!value.value("memory").isObject()||!a.value("canonical").isString()||!a.value("approximate").isBool()||!m.value("canonical").isString()||!m.value("approximate").isBool()||a.value("canonical").toString().size()>8*1024*1024||m.value("canonical").toString().size()>8*1024*1024){setStatus(trc("Invalid workspace section."));return;}newSession.answer={a.value("canonical").toString(),a.value("approximate").toBool()};newSession.memory={m.value("canonical").toString(),m.value("approximate").toBool()};newSession.memorySet=!newSession.memory.canonical.isEmpty();if(value.contains("selected")&&!WorkspaceModel::resultFromJson(value.value("selected").toObject(),importedSelected,&error)){setStatus(error);return;}}
 auto transaction=lockProfile();if(!transaction)return;
 // A single atomic journal protects the multi-file transaction. Startup rolls
 // it back if a provider failure or process death interrupts the commit.
 const QString directory=QFileInfo(configPath_).absolutePath(),journalPath=QDir(directory).filePath("workspace-import.pending");QJsonObject backup;
 if(!saveSettings()){setStatus(trc("Save the current workspace before importing."));return;}
 if(!snapshotFiles(directory,backup,&error)){setStatus(error);return;}
 workbenchHub_->setEnabled(false);setBusy(true);dispatch([this,defs,newSession,sections,policy,nextState,importedHistory,importedSelected,oldDefs,oldBank,oldHistory,oldArchive,oldRegisterArchive,oldState,oldApplied,nextApplied,oldSelected,oldAnswer,oldMemory,oldMemoryApprox,journalPath,directory,backup,transaction](CalcEngine*e){const auto oldSession=e->sessionState();QString error;bool ok=e->replaceDefinitions(defs,&error)&&e->restoreSessionState(newSession,&error);
 if(ok&&importedSelected.ok)ok=e->validateStoredValue(importedSelected.serialized,&error);
 if(ok)for(const auto &entry:importedHistory){const auto object=entry.toObject();const auto canonical=object.value("serialized").toString();if(!canonical.isEmpty()&&!e->validateStoredValue(canonical,&error)){ok=false;break;}}
if(!ok){e->replaceDefinitions(oldDefs);e->restoreSessionState(oldSession);}QMetaObject::invokeMethod(this,[this,ok,error,defs,newSession,sections,policy,nextState,importedHistory,importedSelected,oldDefs,oldBank,oldHistory,oldArchive,oldRegisterArchive,oldState,oldApplied,nextApplied,oldSelected,oldAnswer,oldMemory,oldMemoryApprox,oldSession,journalPath,directory,backup,transaction]()mutable{
  if(closing_)return;bool saved=ok;QString problem=error;
  if(ok){definitions_=defs;registerValues_=newSession.registers;answerState_=newSession.answer;memoryValue_=newSession.memorySet?newSession.memory.canonical:QString();memoryValueApproximate_=newSession.memory.approximate;selectedResult_=importedSelected;workbenchHub_->definitions->setDefinitions(defs);saved=workbenchHub_->restoreState(nextState,&problem);workbenchHub_->setResult(selectedResult_);
   if(saved&&sections.contains("layouts")){if(nextApplied.isEmpty())calculator_->clearCustomLayout();else{CustomKeypad::Layout layout;saved=CustomKeypad::fromJson(nextApplied,&layout,&problem)&&calculator_->setCustomLayout(layout,&problem);}keys_=calculator_->keyButtons();}

   if(saved&&sections.contains("history")){if(policy==1)historyArchive_.clear();for(int i=importedHistory.size()-1;i>=0&&saved;--i)saved=historyArchive_.append(importedHistory[i].toObject());}
   if(saved)saved=registerArchive_.save(registerValues_)&&historyArchive_.save()&&saveSettings();
  }
  bool recovered=true;
  if(!saved){recovered=restoreFiles(directory,backup,&problem);definitions_=oldDefs;registerValues_=oldBank;registerArchive_=oldRegisterArchive;history_=oldHistory;historyArchive_=oldArchive;selectedResult_=oldSelected;answerState_=oldAnswer;memoryValue_=oldMemory;memoryValueApproximate_=oldMemoryApprox;workbenchHub_->restoreState(oldState);if(oldApplied.isEmpty())calculator_->clearCustomLayout();else{CustomKeypad::Layout layout;if(CustomKeypad::fromJson(oldApplied,&layout))calculator_->setCustomLayout(layout);}keys_=calculator_->keyButtons();workbenchHub_->definitions->setDefinitions(oldDefs);workbenchHub_->setResult(oldSelected);dispatch([oldSession,oldDefs](CalcEngine*engine){engine->replaceDefinitions(oldDefs);engine->restoreSessionState(oldSession);});if(!recovered)configSaveBlocked_=true;}

  else history_=historyArchive_.entries();if(saved||recovered)QFile::remove(journalPath);refreshFunctionMetadata();refreshRegisters();fillHistory();workbenchHub_->setEnabled(true);setBusy(false);setStatus(saved?trc("Workspace imported"):trc("Import rolled back")+": "+problem);
 },Qt::QueuedConnection);});
}
