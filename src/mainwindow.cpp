// SPDX-License-Identifier: MIT
#include "mainwindow.h"
#include "desktopfonts.h"
#include "i18n.h"
#include "uiicons.h"
#include "calculatorthemes.h"
#include "financepanel.h"
#include "workspacehub.h"
#include "workspacemodel.h"
#include "workspacejournal.h"
#include "fontscale.h"
#include "desktopworkbench.h"
#include "argumentguide.h"
#include <cmath>
#include <algorithm>
#include <QCryptographicHash>
namespace {
// Keep Desktop actions reachable while the settings form scrolls. Calculator
// uses the same ordinary scroll area with no extra margin or visible actions.
class SettingsScrollArea final : public QScrollArea {
public:
 explicit SettingsScrollArea(QWidget *parent=nullptr):QScrollArea(parent){viewport()->installEventFilter(this);}
 void setActions(QWidget *actions){actions_=actions;actions_->setParent(this);actions_->installEventFilter(this);}
 void setActionsVisible(bool visible){
  if(!actions_)return;actions_->setVisible(visible);placeActions();schedulePlacement();
 }
protected:
 void resizeEvent(QResizeEvent *event)override{QScrollArea::resizeEvent(event);placeActions();}
 bool event(QEvent *event)override{
  const bool handled=QScrollArea::event(event);
  if(event->type()==QEvent::StyleChange||event->type()==QEvent::FontChange||event->type()==QEvent::LayoutRequest)schedulePlacement();
  return handled;
 }
 bool eventFilter(QObject *watched,QEvent *event)override{
  if((watched==actions_&&(event->type()==QEvent::LayoutRequest||event->type()==QEvent::FontChange||event->type()==QEvent::StyleChange))
     ||(watched==viewport()&&event->type()==QEvent::Resize))schedulePlacement();
  return QScrollArea::eventFilter(watched,event);
 }
private:
 void schedulePlacement(){
  if(placementPending_)return;placementPending_=true;
  QTimer::singleShot(0,this,[this]{placementPending_=false;placeActions();});
 }
 void placeActions(){
  if(!actions_||placing_)return;placing_=true;
  const int actionHeight=actions_->isHidden()?0:qMax(actions_->sizeHint().height(),actions_->minimumSizeHint().height());
  if(viewportMargins().bottom()!=actionHeight)setViewportMargins(0,0,0,actionHeight);
  if(actionHeight){
   // QAbstractScrollArea keeps its vertical scrollbar beside the reserved
   // bottom margin too. Use the actual viewport span so it cannot cover Apply.
   const QRect view=viewport()->geometry();
   actions_->setGeometry(view.x(),view.bottom()+1,view.width(),actionHeight);
  }
  placing_=false;
 }
 QWidget *actions_=nullptr;
 bool placementPending_=false,placing_=false;
};
// A wheel gesture on the preferences page scrolls; it never edits a value.
// Forward explicitly so focused controls and spin-box line editors behave alike.
class SettingsWheelGuard final : public QObject {
public:
 explicit SettingsWheelGuard(QScrollArea *scroll):QObject(scroll),scroll_(scroll){scroll_->viewport()->installEventFilter(this);}
 void protect(QWidget *control){
  control->installEventFilter(this);
  for(auto child:control->findChildren<QLineEdit*>())child->installEventFilter(this);
  if(auto combo=qobject_cast<QComboBox*>(control)){
   auto view=combo->view();
   popupViews_.insert(view,view);popupViews_.insert(view->viewport(),view);
   view->installEventFilter(this);view->viewport()->installEventFilter(this);
  }
 }
protected:
 bool eventFilter(QObject *watched,QEvent *event) override {
  if(event->type()!=QEvent::Wheel)return false;
  auto wheel=static_cast<QWheelEvent*>(event);
  // Pixel-only trackpad gestures are ignored by QScrollBar on some Qt styles.
  // Apply their distance explicitly while keeping native angle-wheel behavior.
  if(!wheel->pixelDelta().isNull()){
   auto view=popupViews_.value(watched);
   auto bar=view?view->verticalScrollBar():scroll_->verticalScrollBar();
   qreal distance=wheel->pixelDelta().y();
   if(view&&view->verticalScrollMode()==QAbstractItemView::ScrollPerItem)
    distance/=qMax(1,view->sizeHintForRow(0));
   auto &remainder=pixelRemainder_[bar];
   if(remainder*distance<0)remainder=0;
   remainder+=distance;const int steps=qRound(remainder);remainder-=steps;
   bar->setValue(bar->value()-steps);event->accept();return true;
  }
  if(watched==scroll_->viewport())return false;
  QWidget *target=scroll_->viewport();
  if(auto view=popupViews_.value(watched))target=view->verticalScrollBar();
  QWheelEvent forwarded(target->mapFromGlobal(wheel->globalPosition().toPoint()),wheel->globalPosition(),wheel->pixelDelta(),wheel->angleDelta(),wheel->buttons(),wheel->modifiers(),wheel->phase(),wheel->inverted(),wheel->source());
  QApplication::sendEvent(target,&forwarded);
  event->accept();return true;
 }
private:
 QScrollArea *scroll_;
 QHash<QObject*,QAbstractItemView*> popupViews_;
 QHash<QObject*,qreal> pixelRemainder_;
};
QString csv(QString s){s.replace('"',"\"\"");return '"'+s+'"';}
void appendRecurringMetadata(QJsonObject& row,const CalcResult& result){
 row.insert("resultRepeatStart",result.resultRepeatStart);row.insert("resultRepeatLength",result.resultRepeatLength);row.insert("resultCopyValue",result.resultCopyValue);
 row.insert("approximateRepeatStart",result.approximateRepeatStart);row.insert("approximateRepeatLength",result.approximateRepeatLength);row.insert("approximateCopyValue",result.approximateCopyValue);
}
QString exportedValue(const QJsonObject& row,const QString& key){
 const QString stem=key=="result"?"result":key=="approximate"?"approximate":QString();
 if(!stem.isEmpty()&&!row[stem+"CopyValue"].toString().isEmpty())return row[stem+"CopyValue"].toString();
 return row[key].toVariant().toString();
}


}
QString MainWindow::trc(const char*s)const{return translateText(language_,s);}
QString MainWindow::calculationError(const QString&message)const{const auto match=QRegularExpression("^Register (R[0-9]) is empty\\.?$").match(message);return match.hasMatch()?trc("Register %1 is empty").arg(match.captured(1)):message;}
void MainWindow::addText(QWidget*w,const char*s){labels_.append({w,QString::fromUtf8(s)});}
MainWindow::MainWindow(bool test):testMode_(test){
 DesktopFonts::registerBundledFonts();
 setObjectName("EMCICalc");setWindowTitle("EMCICalc");setMinimumSize(680,540);resize(1180,760);loadSettings();
 fontSizeMode_=DesktopFont::normalizeSizeMode(loadedSettings_.value("fontSizeMode").toString());DesktopFont::setSizeMode("System");
 decimalPlaces_=qBound(0,loadedSettings_.value("decimalPlacesV2").toInt(6),200);percentMode_=qBound(0,loadedSettings_.value("percentModeV2").toInt(),1);implicitMode_=qBound(0,loadedSettings_.value("implicitModeV2").toInt(),2);
 auto savedAnswer=loadedSettings_.value("answerV2").toObject();answerState_={savedAnswer.value("canonical").toString("0"),savedAnswer.value("approximate").toBool()};
 if(!registerArchive_.load(QFileInfo(configPath_).absolutePath(),registerValues_))restoreNotice_=registerArchive_.error();
 historyArchive_.load(QFileInfo(configPath_).absolutePath(),history_);history_=historyArchive_.entries();if(!historyArchive_.error().isEmpty())restoreNotice_=historyArchive_.error();
 auto icon=QIcon(QCoreApplication::applicationDirPath()+"/../share/emcicalc/icon.png");
 for(const auto&p:{QString("/share/emcicalc/icon.png"),QString("/assets/icon.png"),QString("/icon.png")})if(icon.isNull())icon=QIcon(QCoreApplication::applicationDirPath()+p);
 if(icon.isNull())icon=QIcon("assets/icon.png");setWindowIcon(icon);
 auto root=new DesktopSurface;desktopSurface_=root;outer_=new QVBoxLayout(root);outer_->setSizeConstraint(QLayout::SetNoConstraint);outer_->setContentsMargins(10,8,10,6);outer_->setSpacing(6);setCentralWidget(root);
 desktopHeader_=new QWidget;desktopHeader_->setObjectName("desktopHeader");auto header=new QHBoxLayout(desktopHeader_);header->setContentsMargins(8,5,8,5);
 auto title=new QLabel("EMCI<span style='font-weight:400'>Calc</span>");QFont f=font();f.setPointSize(18);f.setBold(true);title->setFont(f);title->setObjectName("brand");header->addWidget(title);
 brandSub_=new QLabel;brandSub_->setObjectName("muted");header->addWidget(brandSub_);header->addStretch();
 gear_=new QPushButton;gear_->setObjectName("settingsGear");gear_->setFixedSize(36,32);gear_->setIconSize(QSize(22,22));header->addWidget(gear_);connect(gear_,&QPushButton::clicked,this,[this]{showPage(3);});
 full_=new QPushButton("F11");full_->setFixedWidth(60);header->addWidget(full_);connect(full_,&QPushButton::clicked,this,[this]{rememberWindowGeometry();isFullScreen()?showNormal():showFullScreen();});outer_->addWidget(desktopHeader_);
 navigation_=new QTabBar;navigation_->setExpanding(false);for(int i=0;i<5;++i)navigation_->addTab("");navigation_->setTabVisible(3,false);outer_->addWidget(navigation_);
 auxHeader_=new QWidget;auxHeader_->setObjectName("calculatorPageHeader");auto aux=new QHBoxLayout(auxHeader_);aux->setContentsMargins(6,4,6,4);aux->setSpacing(5);
 back_=new QPushButton;back_->setObjectName("calculatorBack");back_->setFixedSize(34,32);back_->setIconSize(QSize(20,20));aux->addWidget(back_);connect(back_,&QPushButton::clicked,this,[this]{goBack();});
 auxTitle_=new QLabel;auxTitle_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);QFont af=font();af.setBold(true);auxTitle_->setFont(af);aux->addWidget(auxTitle_,1);
 auxMenu_=new QPushButton("MENU");auxMenu_->setFixedSize(52,32);aux->addWidget(auxMenu_);connect(auxMenu_,&QPushButton::clicked,this,[this]{showWorkspace();});
 auxGear_=new QPushButton;auxGear_->setObjectName("settingsGear");auxGear_->setFixedSize(34,32);auxGear_->setIconSize(QSize(22,22));aux->addWidget(auxGear_);connect(auxGear_,&QPushButton::clicked,this,[this]{showPage(3);});outer_->addWidget(auxHeader_);auxHeader_->hide();
 pages_=new QStackedWidget;pages_->setObjectName("desktopPages");pages_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Ignored);outer_->addWidget(pages_,1);
 workspace_=new QWidget;workspace_->setObjectName("desktopWorkspace");auto workLayout=new QVBoxLayout(workspace_);workLayout->setContentsMargins(0,0,0,0);workLayout->setSpacing(0);workLayout->setSizeConstraint(QLayout::SetNoConstraint);
 split_=new QSplitter;split_->setChildrenCollapsible(false);compact_=new QTabWidget;workLayout->addWidget(split_);workLayout->addWidget(compact_);compact_->hide();
 handheldHost_=new QWidget;handheldLayout_=new QVBoxLayout(handheldHost_);handheldLayout_->setContentsMargins(0,0,0,0);handheldLayout_->setSpacing(0);workLayout->addWidget(handheldHost_);handheldHost_->hide();
 calc_=makeCalculator();graph_=new GraphPanel;graph_->setProperty("desktopCard",true);graph_->setExportDirectory(lastDir_);split_->addWidget(calc_);split_->addWidget(graph_);split_->setStretchFactor(0,0);split_->setStretchFactor(1,1);
 graphHost_=new QWidget;graphLayout_=new QVBoxLayout(graphHost_);graphLayout_->setContentsMargins(0,0,0,0);graphLayout_->setSpacing(0);
 graph_->onPlotRequested=[this](QStringList e,double a,double b,int n){queueGraph(e,a,b,n);};
 graph_->onStatusMessage=[this](QString s){if(status_&&!handheld_){status_->setText(fontMetrics().elidedText(s,Qt::ElideRight,qMax(200,width()-30)));status_->setToolTip(s);}};graph_->onUserDirectoryChanged=[this](QString d){lastDir_=d;saveTimer_->start();};
 pages_->addWidget(workspace_);pages_->addWidget(makeHistory());pages_->addWidget(makeToolbox());pages_->addWidget(makeSettings());pages_->addWidget(makeGuide());pages_->addWidget(graphHost_);pages_->addWidget(makeMenu());catalogTools_=new CalculatorTools;converterTools_=new CalculatorTools;converterTools_->setView(CalculatorTools::View::Converter);converterTools_->setConversionExact(converterExact_);pages_->addWidget(catalogTools_);pages_->addWidget(converterTools_);currencyPanel_=new CurrencyPanel;pages_->addWidget(currencyPanel_);for(int page:{1,2,3,4,6,7,8,9})pages_->widget(page)->setProperty("desktopCard",true);
 status_=new QLabel;status_->setObjectName("muted");status_->setMinimumHeight(17);status_->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);outer_->addWidget(status_);
 navigation_->addTab(trc("Workbenches"));
 connect(navigation_,&QTabBar::currentChanged,this,[this](int i){if(i==5)showWorkspace();else showPage(i);});
 saveTimer_=new QTimer(this);saveTimer_->setSingleShot(true);saveTimer_->setInterval(700);connect(saveTimer_,&QTimer::timeout,this,[this]{saveSettings();});
 previewTimer_=new QTimer(this);previewTimer_->setSingleShot(true);previewTimer_->setInterval(250);connect(previewTimer_,&QTimer::timeout,this,[this]{startPreview();});
 setupWorker();setupTools();setupWorkspace();DesktopFont::setSizeMode(fontSizeMode_);refreshFunctionMetadata();watchSystemTheme();updateTranslations();applyMathPreferences();applyTheme();layoutReady_=true;updateLayout();restoreGeometrySafe();updateLayout();fillHistory();
 for(const auto&pair:QList<QPair<QKeySequence,std::function<void()>>>{{QKeySequence(Qt::Key_F11),[this]{full_->click();}},{QKeySequence(Qt::Key_Escape),[this]{if(calculator_->registerLayerActive()){handleAction("@regExit");return;}if(isFullScreen())showNormal();else if((handheld_&&pages_->currentIndex()!=0)||settingsDraftActive_)goBack();else if(pages_->currentIndex()==0&&calc_->isVisible()){if(shift_||alpha_){shift_=alpha_=false;refreshKeys();}else{calculator_->flashToken("@clear");handleAction("@clear");}}}},{QKeySequence("Ctrl+L"),[this]{showPage(0);if(!handheld_&&narrow_)compact_->setCurrentWidget(calc_);input_->setFocus();input_->selectAll();}},{QKeySequence("Ctrl+G"),[this]{showGraph();}},{QKeySequence("Ctrl+H"),[this]{showPage(1);}},{QKeySequence("Ctrl+,"),[this]{showPage(3);}},{QKeySequence(Qt::Key_F1),[this]{showPage(4);}}}){auto a=new QShortcut(pair.first,this);connect(a,&QShortcut::activated,this,pair.second);}
 keyboard_=new CalculatorKeyboard(this,calculator_,[this]{return pages_->currentIndex()==0&&calc_->isVisible();});keyboard_->onAction=[this](QString token){handleAction(token);};keyboard_->onHistoryStep=[this](int direction){stepHistory(direction);};keyboard_->onCopyResult=[this]{copyResult();};keyboard_->onResetLayer=[this]{if(closing_)return;shift_=alpha_=false;refreshKeys();};
 QTimer::singleShot(50,this,[this]{if(!restoreNotice_.isEmpty())setStatus(trc(restoreNotice_.toUtf8().constData()));if(pages_->currentIndex()==0)input_->setFocus();schedulePreview();});
}
MainWindow::~MainWindow(){closing_=true;for(auto flag:workspaceJobs_)flag->store(false);for(auto flag:workspaceCancelFlags_)flag->store(true);if(workbenchHub_)workbenchHub_->cancelWork();if(evaluationCommitAllowed_)evaluationCommitAllowed_->store(false);if(exchangeRates_)exchangeRates_->cancel();if(systemThemeProcess_ && systemThemeProcess_->state()!=QProcess::NotRunning){systemThemeProcess_->terminate();if(!systemThemeProcess_->waitForFinished(100)){systemThemeProcess_->kill();systemThemeProcess_->waitForFinished(100);}}if(workerThread_.isRunning()){QMetaObject::invokeMethod(worker_,[this]{delete engine_;engine_=nullptr;},Qt::BlockingQueuedConnection);workerThread_.quit();workerThread_.wait();}}
void MainWindow::setupWorker(){worker_=new QObject;worker_->moveToThread(&workerThread_);connect(&workerThread_,&QThread::finished,worker_,&QObject::deleteLater);workerThread_.start();const auto registers=registerValues_;const auto answer=answerState_;const auto memory=CalcStoredValue{memoryValue_,memoryValueApproximate_};dispatch([this,registers,answer,memory](CalcEngine*){engine_=new CalcEngine;QString error;CalcSessionState session;session.registers=registers;session.answer=answer;session.memory=memory;session.memorySet=!memory.canonical.isEmpty();const bool restored=engine_->restoreSessionState(session,&error);QMetaObject::invokeMethod(this,[this,restored]{if(!restored){registerValues_.clear();registerArchive_.preserveInvalid();restoreNotice_=registerArchive_.error();setStatus(trc(restoreNotice_.toUtf8().constData()));}registersReady_=true;refreshRegisters();},Qt::QueuedConnection);});}
void MainWindow::persistRegisters(){
 auto transaction=lockProfile();if(!transaction)return;
 if(!registerArchive_.save(registerValues_))setStatus(trc(registerArchive_.error().toUtf8().constData()));
}
void MainWindow::refreshRegisters(){
 if(!registersPanel_||!registersReady_)return;
 const int generation=++registersGeneration_;
 const auto bank=registerValues_;
 // Discard stale formatted rows immediately, including while the panel is hidden.
 registersPanel_->setValues(bank,{});
 if(!pages_||pages_->currentIndex()!=2||toolboxTabs_->currentIndex()!=1)return;const auto selected=options();registersPanel_->setDisplayOptions(selected);
 dispatch([this,bank,selected,generation](CalcEngine*engine){QMap<QString,CalcResult> formatted;
  for(auto it=bank.begin();it!=bank.end();++it)formatted[it.key()]=engine->formatValue(it.value().canonical,selected,it.value().approximate);
  QMetaObject::invokeMethod(this,[this,bank,formatted,generation]{if(closing_||generation!=registersGeneration_)return;registersPanel_->setValues(bank,formatted);},Qt::QueuedConnection);
 });
}
void MainWindow::clearRegisters(const QStringList& names){
 auto transaction=lockProfile();if(!transaction)return;
 if(!registersReady_||busy_)return;
 ++registersGeneration_;const auto before=registerValues_;auto next=before;
 if(names.isEmpty())next.clear();else for(const auto&name:names)next.remove(name);
 if(!registerArchive_.save(next)){setStatus(trc(registerArchive_.error().toUtf8().constData()));return;}
 registerValues_=next;
 dispatch([this,names](CalcEngine*engine){engine->clearRegisters(names);QMetaObject::invokeMethod(this,[this]{refreshRegisters();if(!baseResultDetached_)schedulePreview();},Qt::QueuedConnection);});
 refreshRegisters();
}
void MainWindow::setupTools(){
 catalogTools_->onInsert=[this](QString token){showPage(0);if(!handheld_&&narrow_)compact_->setCurrentWidget(calc_);insert(token);};
 auto *finance=catalogTools_->financePanel();
 finance->onInsert=catalogTools_->onInsert;
 finance->onDraftChanged=[this]{++financeGeneration_;if(saveTimer_)saveTimer_->start();};
 finance->restoreState(loadedSettings_.value("financeDraftV2").toObject());
 finance->onCalculate=[this,finance](QString id,QJsonObject input){
  const int generation=++financeGeneration_;const auto selected=options();finance->setBusy(true);
  dispatch([this,id,input,selected,generation,finance](CalcEngine*engine){
   Finance::Result result;
   try{
    Finance::Tool definition;bool found=false;for(const auto&t:Finance::tools())if(t.id==id){definition=t;found=true;break;}
    Finance::require(found,"Unknown financial worksheet.","unsupported");
    const auto resolved=Finance::resolveInput(definition,input,[engine,selected](QString expression){return engine->financeNumber(expression,selected);});
    result=Finance::calculate(id,resolved);
   }catch(const Finance::Error&e){result.ok=false;result.status=e.status;result.message=QString::fromUtf8(e.what());}
   catch(const std::exception&e){result.ok=false;result.status="invalid";result.message=QString::fromUtf8(e.what());}
   QMetaObject::invokeMethod(this,[this,finance,result,generation]{if(closing_||generation!=financeGeneration_)return;finance->setBusy(false);finance->setResult(result);},Qt::QueuedConnection);
  });
 };
 finance->onUseCachedRate=[this,finance](QString from,QString to){
  const auto snapshot=activeRatesSnapshot_;from=from.trimmed().toUpper();to=to.trimmed().toUpper();
  if(!snapshot.isValid()||!snapshot.ratesPerEuro.contains(from)||!snapshot.ratesPerEuro.contains(to)){
   Finance::Result result;result.ok=false;result.status="unavailable";result.message="No cached rate is available for these currencies. Open Currency converter and refresh rates.";finance->setResult(result);return;
  }
  try{
   const auto rate=Finance::Real(snapshot.ratesPerEuro.value(to))/Finance::Real(snapshot.ratesPerEuro.value(from));
   const QString timestamp=snapshot.date.toString(Qt::ISODate)+"; fetched "+snapshot.fetchedAtUtc.toUTC().toString(Qt::ISODate);
   finance->setCachedRate(rate.str(),timestamp,snapshot.source);
  }catch(const Finance::Error&e){Finance::Result result;result.ok=false;result.message=QString::fromUtf8(e.what());finance->setResult(result);}
 };
 converterTools_->onInsert=[this](QString token){baseBox_->setCurrentIndex(baseBox_->findData(10));showPage(0);if(!handheld_&&narrow_)compact_->setCurrentWidget(calc_);insert(token);};
 catalogTools_->onInsertMatrix=[this](int rows,int columns){
  const bool resizing=!input_->matrixDimensions().isEmpty();
  if(resizing&&!input_->resizeMatrix(rows,columns)){setStatus(trc("Remove filled cells before reducing matrix dimensions."));return;}
  if(!resizing)input_->insertMatrix(rows,columns);
  showPage(0);if(!handheld_&&narrow_)compact_->setCurrentWidget(calc_);input_->setFocus();
 };
 converterTools_->onConvert=[this](QString value,QString from,QString to){convertUnits(value,from,to);};
 converterTools_->onDraftChanged=[this]{++conversionGeneration_;lastConversion_={};lastConversionTarget_.clear();converterTools_->setBusy(false);};
 converterTools_->onConversionExactChanged=[this](bool exact){converterExact_=exact;formatConversionResult();saveTimer_->start();};
 currencyPanel_->onInsert=converterTools_->onInsert;
 currencyPanel_->onConvert=[this](QString value,QString from,QString to){convertCurrency(value,from,to);};
 currencyPanel_->onDraftChanged=[this]{++currencyConversionGeneration_;currencyPanel_->setBusy(currencyRatesApplying_);};
 exchangeRates_=new ExchangeRateService(QFileInfo(configPath_).absolutePath()+"/exchange-rates.zip",this);
 exchangeRates_->onChanged=[this]{if(!closing_)syncCurrencyRates();};
 currencyPanel_->onRefresh=[this]{exchangeRates_->refresh(true);};
 const auto cached=exchangeRates_->snapshot();requestedRatesSnapshot_=cached;
 dispatch([this,cached](CalcEngine*engine){
  auto entries=engine->catalog();auto currencies=engine->currencyCatalog();QString rateError;
  const bool ratesReady=cached.isValid()&&engine->setCurrencyRates(cached.ratesPerEuro,&rateError);
  QMetaObject::invokeMethod(this,[this,entries,currencies,cached,ratesReady,rateError]{
   if(closing_)return;unitTokens_.clear();constantTokens_.clear();unitLabels_.clear();
   for(const auto&entry:entries)if(entry.unit){unitTokens_<<entry.token;if(!entry.displayToken.isEmpty())unitLabels_[entry.token]=entry.displayToken;}else constantTokens_<<entry.token;
   for(const auto&entry:currencies)unitTokens_<<entry.token;
   registersPanel_->setCatalog(entries);catalogTools_->setCatalog(entries);converterTools_->setCatalog(entries);currencyPanel_->setCatalog(currencies);
   for(auto widget:findChildren<QWidget*>())if(auto editor=dynamic_cast<MathEditor*>(widget)){editor->setUnitTokens(unitTokens_);editor->setUnitLabels(unitLabels_);editor->setConstantNames(constantTokens_);}
   if(ratesReady)activeRatesSnapshot_=cached;
   currencyPanel_->setRateState(activeRatesSnapshot_,exchangeRates_->refreshing(),rateError.isEmpty()?trc(exchangeRates_->lastError().toUtf8().constData()):trc(rateError.toUtf8().constData()));
   catalogReady_=true;fillHistory();
  },Qt::QueuedConnection);
 });
}
void MainWindow::convertUnits(QString value,QString from,QString to){
 const int generation=++conversionGeneration_;lastConversion_={};lastConversionTarget_.clear();auto selected=options();selected.numberBase=10;selected.exact=converterTools_->conversionExact();selected.notation=0;converterTools_->setBusy(true);
 dispatch([this,value,from,to,selected,generation](CalcEngine*engine){auto result=engine->convertUnits(value,from,to,selected);QMetaObject::invokeMethod(this,[this,result,to,selected,generation]{if(closing_||generation!=conversionGeneration_)return;converterTools_->setBusy(false);lastConversion_=result;lastConversionTarget_=result.ok?to:QString();converterTools_->setConversionResult(result);if(!result.ok)return;
 QJsonObject row{{"expression",result.input},{"result",result.result},{"approximate",result.approximate},{"resultApproximate",result.resultApproximate},{"approximateApproximate",result.approximateApproximate},{"serialized",result.serialized},{"valueApproximate",result.valueApproximate},{"angle",selected.angle},{"base",10},{"notation",selected.notation},{"exact",selected.exact},{"complexEnabled",selected.complexEnabled},{"time",QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}};appendRecurringMetadata(row,result);row.insert("snapshotV2",WorkspaceModel::resultToJson(result));
 if(!historyArchive_.append(row))setStatus(trc("Could not save history")+": "+historyArchive_.error());history_=historyArchive_.entries();historyPage_=0;fillHistory();saveTimer_->start();
 },Qt::QueuedConnection);});
}
void MainWindow::formatConversionResult(){
 const int generation=++conversionGeneration_;
 if(!lastConversion_.ok||lastConversion_.serialized.isEmpty()){converterTools_->setConversionResult({});return;}
 const auto stored=lastConversion_;const auto target=lastConversionTarget_;auto selected=options();selected.numberBase=10;selected.exact=converterTools_->conversionExact();selected.notation=0;
 converterTools_->setConversionResult({});converterTools_->setBusy(true);
 dispatch([this,stored,target,selected,generation](CalcEngine*engine){auto result=engine->formatUnitConversion(stored.serialized,target,selected,stored.valueApproximate);result.input=stored.input;
  QMetaObject::invokeMethod(this,[this,result,generation]{if(closing_||generation!=conversionGeneration_)return;converterTools_->setBusy(false);converterTools_->setConversionResult(result);},Qt::QueuedConnection);
 });
}
void MainWindow::syncCurrencyRates(){
 const auto snapshot=exchangeRates_->snapshot();
 if(snapshot.isValid()&&(snapshot.ratesPerEuro!=requestedRatesSnapshot_.ratesPerEuro||snapshot.date!=requestedRatesSnapshot_.date)){
  requestedRatesSnapshot_=snapshot;const int generation=++currencyRatesGeneration_;++currencyConversionGeneration_;currencyRatesApplying_=true;
  currencyPanel_->setConversionResult(CalcResult{});currencyPanel_->setBusy(true);
  dispatch([this,snapshot,generation](CalcEngine*engine){QString error;const bool ready=engine->setCurrencyRates(snapshot.ratesPerEuro,&error);
   QMetaObject::invokeMethod(this,[this,snapshot,generation,ready,error]{if(closing_||generation!=currencyRatesGeneration_)return;
    currencyRatesApplying_=false;if(ready)activeRatesSnapshot_=snapshot;
    currencyPanel_->setRateState(activeRatesSnapshot_,exchangeRates_->refreshing(),ready?trc(exchangeRates_->lastError().toUtf8().constData()):trc(error.toUtf8().constData()));currencyPanel_->setBusy(false);
   },Qt::QueuedConnection);
  });
 }else{
  if(!currencyRatesApplying_&&snapshot.isValid()&&snapshot.ratesPerEuro==activeRatesSnapshot_.ratesPerEuro&&snapshot.date==activeRatesSnapshot_.date)activeRatesSnapshot_=snapshot;
  currencyPanel_->setRateState(activeRatesSnapshot_,exchangeRates_->refreshing(),trc(exchangeRates_->lastError().toUtf8().constData()));
 }
}
void MainWindow::convertCurrency(QString value,QString from,QString to){
 if(currencyRatesApplying_)return;
 const int generation=++currencyConversionGeneration_,rateGeneration=currencyRatesGeneration_;const auto snapshot=activeRatesSnapshot_;
 auto selected=options();selected.numberBase=10;selected.exact=false;selected.mixedFractions=false;selected.notation=0;currencyPanel_->setBusy(true);
 dispatch([this,value,from,to,selected,generation,rateGeneration,snapshot](CalcEngine*engine){auto result=engine->convertCurrency(value,from,to,selected);
  QMetaObject::invokeMethod(this,[this,result,selected,generation,rateGeneration,snapshot]{if(closing_||generation!=currencyConversionGeneration_||rateGeneration!=currencyRatesGeneration_)return;
   currencyPanel_->setBusy(false);currencyPanel_->setConversionResult(result);if(!result.ok)return;
   QJsonObject row{{"expression",result.input},{"result",result.result},{"approximate",result.approximate},{"resultApproximate",result.resultApproximate},{"approximateApproximate",result.approximateApproximate},{"serialized",result.serialized},{"valueApproximate",result.valueApproximate},{"angle",selected.angle},{"base",10},{"notation",selected.notation},{"exact",selected.exact},{"complexEnabled",selected.complexEnabled},{"time",QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},{"conversionKind","currency"},{"rateDate",snapshot.date.toString(Qt::ISODate)},{"rateSource",snapshot.source},{"ratesFetchedAt",snapshot.fetchedAtUtc.toString(Qt::ISODate)}};appendRecurringMetadata(row,result);row.insert("snapshotV2",WorkspaceModel::resultToJson(result));
   if(!historyArchive_.append(row))setStatus(trc("Could not save history")+": "+historyArchive_.error());history_=historyArchive_.entries();historyPage_=0;fillHistory();saveTimer_->start();
  },Qt::QueuedConnection);
 });
}
void MainWindow::dispatch(std::function<void(CalcEngine*)>fn){QMetaObject::invokeMethod(worker_,[this,fn]{fn(engine_);},Qt::QueuedConnection);}
CalcOptions MainWindow::options()const{CalcOptions o;o.angle=angle_;o.exact=exact_;o.mixedFractions=mixedFractions_;o.precision=precision_;o.numberBase=numberBase_;o.notation=notation_;o.complexEnabled=complexEnabled_;o.decimalPlaces=decimalPlaces_;o.percentMode=percentMode_;o.implicitMode=implicitMode_;return o;}
QWidget* MainWindow::makeCalculator(){
 calculator_=new CalculatorPanel;calculator_->setKeyTooltipsEnabled(keyTooltipsEnabled_);calculator_->setScreenTooltipsEnabled(screenTooltipsEnabled_);
 input_=calculator_->editor();result_=calculator_->resultView();approx_=calculator_->approximateLabel();
 angleBox_=calculator_->angleBox();exactBox_=calculator_->exactBox();baseBox_=calculator_->baseBox();notationBox_=calculator_->notationBox();notationBox_->addItem("FIX",3);notationBox_->addItem("SI",4);memoryButtons_=calculator_->memoryButtons();copy_=calculator_->copyButton();
 angleBox_->setCurrentIndex(angle_);exactBox_->setCurrentIndex(exact_?0:1);baseBox_->setCurrentIndex(baseBox_->findData(numberBase_));notationBox_->setCurrentIndex(notation_);input_->setNumberBase(numberBase_);result_->setNumberBase(numberBase_);input_->setText(baseDrafts_.value(numberBase_));
 connect(angleBox_,&QComboBox::currentIndexChanged,this,[this](int n){angle_=n;invalidateResult();schedulePreview();if(saveTimer_)saveTimer_->start();if(graph_&&graph_->hasExpressions())graph_->requestPlot(false);});
 connect(exactBox_,&QComboBox::currentIndexChanged,this,[this](int n){exact_=n==0;schedulePreview();reformatResult();if(saveTimer_)saveTimer_->start();});
 input_->onTextChanged=[this]{calculator_->setLiteralInputBase(input_->explicitLiteralBase());if(restoringDraft_)return;const bool detached=baseResultDetached_;baseResultDetached_=false;if(detached)setStatus(trc("Ready"));if(busy_)invalidateResult();schedulePreview();if(!continuousEvaluation_){previewDraft_=true;clearResultDisplay();}};
 input_->onCursorChanged=[this]{calculator_->setLiteralInputBase(input_->explicitLiteralBase());};
 input_->onEdited=[this]{historyCursor_=-1;historyDraft_.clear();if(saveTimer_)saveTimer_->start();};
 connect(baseBox_,&QComboBox::currentIndexChanged,this,[this]{changeBase(baseBox_->currentData().toInt());});
 connect(notationBox_,&QComboBox::currentIndexChanged,this,[this](int n){notation_=n;schedulePreview();reformatResult();if(saveTimer_)saveTimer_->start();});
 for(int i=0;i<memoryButtons_.size();++i){const QString op=QStringList{"MS","MR","M+","MC"}[i];connect(memoryButtons_[i],&QPushButton::clicked,this,[this,op]{memory(op);});}
 connect(copy_,&QPushButton::clicked,this,[this]{copyResult();});
 calculator_->onAction=[this](QString token){handleAction(token);};
 applyDesign();calculator_->setMemoryActive(!memoryValue_.isEmpty());return calculator_;
}
void MainWindow::applyDesign(){
 const bool nextHandheld=design_!="Desktop", changed=nextHandheld!=handheld_;
 if(layoutReady_&&changed){calculator_->setRegisterLayer(false);shift_=alpha_=false;rememberWindowGeometry();pageBackStack_.clear();}handheld_=nextHandheld;
 if(registersPanel_)registersPanel_->setCalculatorMode(handheld_);
 calculator_->setDesign(design_);calculator_->setCalculatorTheme(calculatorTheme_);calculator_->setCalculatorLayout(calculatorLayout_);calculator_->setComplexEnabled(complexEnabled_);calculator_->setDesktopEffects(desktopEffects_);calculator_->setDecimalSeparator(decimalSeparator_);keys_=calculator_->keyButtons();calculator_->retranslate([this](const char*s){return trc(s);});refreshKeys();
 if(layoutReady_){updateLayout();if(changed&&!isFullScreen()&&!isMaximized()){auto geometry=handheld_?calculatorGeometry_:desktopGeometry_;if(!geometry.isEmpty())restoreGeometry(geometry);else resize(handheld_?QSize(380,720):QSize(1180,760));}applyTheme();updateAuxiliaryLayouts();}
}
void MainWindow::refreshKeys(){if(calculator_)calculator_->setShiftState(shift_,alpha_);}
void MainWindow::copyResult(){if(result_->text().isEmpty()||result_->resultRelation().isEmpty())return;QApplication::clipboard()->setText(displayCopyValue_.isEmpty()?result_->toPlainText():displayCopyValue_);setStatus(trc("Result copied"));}
void MainWindow::stepHistory(int direction){
 if(history_.isEmpty())return;
 if(historyCursor_<0){if(direction>0)return;historyDraft_=input_->text();historyDraftOptions_=options();historyCursor_=0;}
 else historyCursor_=qBound(-1,historyCursor_+(direction<0?1:-1),int(history_.size())-1);
 const int cursor=historyCursor_;const auto draft=historyDraft_;
 if(cursor<0)recallHistoryEntry({{"expression",draft},{"base",historyDraftOptions_.numberBase},{"notation",historyDraftOptions_.notation},{"angle",historyDraftOptions_.angle},{"exact",historyDraftOptions_.exact},{"complexEnabled",historyDraftOptions_.complexEnabled}});
 else recallHistoryEntry(history_[cursor].toObject());
 historyCursor_=cursor;historyDraft_=draft;
}
void MainWindow::handleAction(QString token){
 if(token=="@modifierExit"){shift_=alpha_=false;calculator_->setRegisterLayer(false);calculator_->setHeldModifiers(false,false);if(keyboard_)keyboard_->resetModifiers();refreshKeys();input_->setFocus();return;}
 if(token=="@reg"||token=="@regExit"){shift_=alpha_=false;calculator_->setRegisterLayer(token=="@reg");refreshKeys();input_->setFocus();return;}
 if(token.startsWith("@hex")||token=="@binaryPrefix"){insert(token);shift_=alpha_=false;refreshKeys();return;}
 if(token=="@shift"){shift_=!shift_;alpha_=false;refreshKeys();return;}
 if(token=="@alpha"){alpha_=!alpha_;shift_=false;refreshKeys();return;}
 bool focusEditor=true;
 if(token=="@left")input_->cursorBackward(false);
 else if(token=="@right")input_->cursorForward(false);
 else if(token=="@home")input_->home(false);
 else if(token=="@end")input_->end(false);
 else if(token=="@fraction"||token=="@mixedFraction"||token=="@recurring"||token=="@dms")input_->insertTemplate(token);
 else if(token=="@del")input_->backspace();
 else if(token=="@forwardDelete")input_->del();
 else if(token=="@undo")input_->undo();
 else if(token=="@clear"){selectedResult_={};if(workbenchHub_)workbenchHub_->setResult(selectedResult_);if(evaluationCommitAllowed_)evaluationCommitAllowed_->store(false);++evaluationGeneration_;++renderGeneration_;formatPending_=false;input_->clear();cancelPreview();commitDisplay({},{});lastSerialized_.clear();lastValueApproximate_=false;historyCursor_=-1;historyDraft_.clear();}
 else if(token=="@eval"||token=="@approx"){if(token=="@approx"){QSignalBlocker block(exactBox_);exactBox_->setCurrentIndex(1);exact_=false;schedulePreview();}evaluate();}
 else if(token=="@angle")angleBox_->setCurrentIndex((angleBox_->currentIndex()+1)%3);
 else if(token.startsWith("@base")){if(busy_)return;const int b=token.mid(5).toInt();baseBox_->setCurrentIndex(baseBox_->findData(b));}
 else if(token=="@notation"&&!busy_)notationBox_->setCurrentIndex((notationBox_->currentIndex()+1)%notationBox_->count());
 else if(token=="@complex"){complexEnabled_=!complexEnabled_;calculator_->setComplexEnabled(complexEnabled_);if(complexBox_){QSignalBlocker block(complexBox_);complexBox_->setCurrentIndex(complexEnabled_?1:0);}invalidateResult();schedulePreview();saveTimer_->start();}
 else if(token=="@constants"){catalogTools_->setView(CalculatorTools::View::Constants);showPage(7);focusEditor=false;}
 else if(token=="@units"){showPage(8);focusEditor=false;}
 else if(token=="@currency"){showPage(9);focusEditor=false;}
 else if(token=="@statistics"){catalogTools_->setView(CalculatorTools::View::Statistics);showPage(7);focusEditor=false;}
 else if(token=="@matrix"||(token=="["&&structuredInput_)){openMatrixTools();focusEditor=false;}
 else if(token=="@format"){if(!busy_){exactBox_->setCurrentIndex(1-exactBox_->currentIndex());}}
 else if(token=="@settings"){showPage(3);focusEditor=false;}
 else if(token=="@menu"){showWorkspace();focusEditor=false;}
 else if(token=="@toolbox"){showPage(2);focusEditor=false;}
 else if(token=="@history"){showPage(1);focusEditor=false;}
 else if(token=="@graph"){showGraph();focusEditor=false;}
 else if(token.startsWith("@memory"))memory(token.mid(7));
 else if(token=="@mminus")memory("M-");
 else if(token=="@ms")memory("MS");else if(token=="@mr")memory("MR");else if(token=="@mplus")memory("M+");else if(token=="@mc")memory("MC");
 else if(!token.startsWith('@'))insert(token);
 shift_=alpha_=false;refreshKeys();if(focusEditor)input_->setFocus();
}
void MainWindow::insert(QString s){input_->insertTemplate(s);input_->setFocus();}
void MainWindow::setBusy(bool busy){
 busy_=busy;calculator_->setBusy(busy);baseBox_->setEnabled(!busy);notationBox_->setEnabled(!busy);setStatus(busy?trc("Calculating…"):trc("Ready"));
 if(!busy&&previewPending_&&previewTimer_&&!previewTimer_->isActive())QTimer::singleShot(0,this,[this]{startPreview();});
 if(!busy&&formatPending_)QTimer::singleShot(0,this,[this]{if(formatPending_){formatPending_=false;reformatResult();}});
}
void MainWindow::evaluate(){
 if(busy_||!registersReady_||input_->text().trimmed().isEmpty())return;
 if(input_->text().size()>input_->maxLength()){invalidateResult();setStatus(trc("Input exceeds the %1-character limit.").arg(input_->maxLength()));input_->setFocus();return;}
 if(!input_->isComplete()){invalidateResult();setStatus(trc("Complete the input slots and close all brackets."));input_->setFocus();return;}
 cancelPreview();baseResultDetached_=false;
 if(evaluationCommitAllowed_)evaluationCommitAllowed_->store(false);evaluationCommitAllowed_=std::make_shared<std::atomic_bool>(true);const auto allowed=evaluationCommitAllowed_;
 const auto expression=input_->text().trimmed();const int generation=++evaluationGeneration_;setBusy(true);const auto selected=options();
 dispatch([this,expression,selected,generation,allowed](CalcEngine*engine){
  const auto before=engine->sessionState();auto result=engine->evaluate(expression,selected,allowed);const auto candidate=engine->sessionState();QString error;
  if(!engine->restoreSessionState(before,&error)){result=CalcResult{};result.error=error;}
  QMetaObject::invokeMethod(this,[this,result,selected,generation,candidate,allowed]{
   if(closing_)return;
   if(generation!=evaluationGeneration_||!allowed->load()){setBusy(false);return;}
   if(!result.ok){setBusy(false);invalidateResult();if(result.undefinedResult){undefinedDisplayError_=result.error;result_->setPlaceholderText("NaN");result_->setAccessibleName(trc("Result")+": NaN");QAccessibleEvent event(result_,QAccessible::NameChanged);QAccessible::updateAccessibility(&event);}setStatus(trc("Calculation error")+": "+calculationError(result.error));return;}
   if(!commitWorkbench(result,candidate)){setBusy(false);invalidateResult();return;}
   lastSerialized_=result.serialized;lastValueApproximate_=result.valueApproximate;answerState_=candidate.answer;
   if(input_->text().trimmed()==result.input.trimmed()&&selected.numberBase==numberBase_&&selected.angle==angle_&&selected.complexEnabled==complexEnabled_)previewDraft_=false;
   if(selected.numberBase==numberBase_&&selected.angle==angle_&&selected.complexEnabled==complexEnabled_&&selected.notation==notation_&&selected.exact==exact_&&selected.precision==precision_&&selected.mixedFractions==mixedFractions_)commitDisplay(result);
   else{clearResultDisplay();formatPending_=true;}
   calculator_->setMemoryActive(!memoryValue_.isEmpty());historyCursor_=-1;historyDraft_.clear();historyPage_=0;
   dispatch([this,candidate](CalcEngine*engine){QString error;const bool restored=engine->restoreSessionState(candidate,&error);QMetaObject::invokeMethod(this,[this,restored,error]{if(closing_)return;setBusy(false);if(!restored){configSaveBlocked_=true;invalidateResult();setStatus(error);}},Qt::QueuedConnection);});
  },Qt::QueuedConnection);
 });
}
void MainWindow::reformatResult(){
 refreshRegisters();
 if(lastSerialized_.isEmpty()){commitDisplay({},{});return;}
 const int generation=++renderGeneration_;if(busy_){formatPending_=true;return;}
 formatPending_=false;const QString value=lastSerialized_;const bool valueApproximate=lastValueApproximate_;const auto selected=options();setBusy(true);
 dispatch([this,value,valueApproximate,selected,generation](CalcEngine*engine){auto result=engine->formatValue(value,selected,valueApproximate);QMetaObject::invokeMethod(this,[this,result,value,generation]{
  if(closing_)return;setBusy(false);
  if(generation!=renderGeneration_||value!=lastSerialized_)return;
  if(!result.ok){committedDisplayData_={};committedResult_.clear();committedApproximation_.clear();clearResultDisplay();setStatus(trc("Calculation error")+": "+calculationError(result.error));return;}
  commitDisplay(result);if(baseResultDetached_)setStatus(trc("Last confirmed result · %1").arg(numberBase_==2?"BIN":numberBase_==8?"OCT":numberBase_==16?"HEX":"DEC"));
 },Qt::QueuedConnection);});
}
void MainWindow::clearResultDisplay(){
 undefinedDisplayError_.clear();result_->setPlaceholderText({});result_->setAccessibleName(trc("Result"));
 previewDisplayed_=false;displayCopyValue_.clear();copy_->setEnabled(false);result_->setResultRelation({});result_->setPlainText({});setApproximation({});
}
void MainWindow::invalidateResult(){
 selectedResult_={};if(workbenchHub_)workbenchHub_->setResult(selectedResult_);
 if(evaluationCommitAllowed_)evaluationCommitAllowed_->store(false);
 ++evaluationGeneration_;++renderGeneration_;formatPending_=false;cancelPreview();lastSerialized_.clear();lastValueApproximate_=false;
 committedDisplayData_={};committedResult_.clear();committedApproximation_.clear();clearResultDisplay();
}
void MainWindow::showCommittedResult(){
 if(committedResult_.isEmpty()){clearResultDisplay();return;}
 displayCopyValue_=committedDisplayData_.resultCopyValue;copy_->setEnabled(true);
 result_->setResultRelation(committedResult_=="—"?QString():committedResultApproximate_?QString("≈"):QString("="));result_->setPlainText(committedResult_);result_->setRecurringDigits(committedDisplayData_.resultRepeatStart,committedDisplayData_.resultRepeatLength,committedDisplayData_.resultCopyValue);setApproximation(committedApproximation_,committedApproximationApproximate_,committedDisplayData_.approximateRepeatStart,committedDisplayData_.approximateRepeatLength,committedDisplayData_.approximateCopyValue);
}
void MainWindow::commitDisplay(const CalcResult& value){
 undefinedDisplayError_.clear();result_->setPlaceholderText({});result_->setAccessibleName(trc("Result"));
 committedDisplayData_=value;committedResult_=value.result;committedApproximation_=value.approximate;committedResultApproximate_=value.resultApproximate;committedApproximationApproximate_=value.approximateApproximate;
 if(!previewDisplayed_&&!previewDraft_)showCommittedResult();
}
void MainWindow::commitDisplay(const QString& value,const QString& approximate){CalcResult display;display.result=value;display.approximate=approximate;commitDisplay(display);}
void MainWindow::cancelPreview(){
 ++previewGeneration_;previewPending_=false;
 if(previewTimer_)previewTimer_->stop();
 previewDisplayed_=false;previewDraft_=continuousEvaluation_;
 if(previewDraft_)clearResultDisplay();else showCommittedResult();
}
void MainWindow::schedulePreview(){
 if(restoringDraft_)return;
 if(baseResultDetached_){cancelPreview();previewDraft_=false;return;}
 cancelPreview();
 if(!previewTimer_||!continuousEvaluation_||!input_->isComplete()||input_->text().trimmed().isEmpty())return;
 previewPending_=true;previewTimer_->start();
}
void MainWindow::startPreview(){
 if(!previewPending_||!continuousEvaluation_||closing_||!input_->isComplete())return;
 if(busy_||previewRunning_)return;
 previewPending_=false;previewRunning_=true;
 const int generation=previewGeneration_;const QString expression=input_->text().trimmed();const auto selected=options();
 dispatch([this,expression,selected,generation](CalcEngine* engine){
  const auto preview=engine->preview(expression,selected);
  QMetaObject::invokeMethod(this,[this,preview,generation]{
   previewRunning_=false;if(closing_)return;
   if(continuousEvaluation_&&generation==previewGeneration_&&preview.ok){
    displayCopyValue_=preview.resultCopyValue;copy_->setEnabled(true);
    previewDisplayed_=true;result_->setResultRelation(preview.resultApproximate?"≈":"=");result_->setPlainText(preview.result);result_->setRecurringDigits(preview.resultRepeatStart,preview.resultRepeatLength,preview.resultCopyValue);setApproximation(preview.approximate,preview.approximateApproximate,preview.approximateRepeatStart,preview.approximateRepeatLength,preview.approximateCopyValue);
   }
   // Coalesce edits made while the worker was busy. Never queue each keystroke.
   if(previewPending_&&!previewTimer_->isActive())startPreview();
  },Qt::QueuedConnection);
 });
}
void MainWindow::memory(QString op){
 if(op=="MR"){if(!memoryValue_.isEmpty()){input_->insert("M");input_->setFocus();}return;}
 if(busy_||(op!="MC"&&lastSerialized_.isEmpty()))return;
 cancelPreview();
 if(op=="MC"){memoryValue_.clear();memoryValueApproximate_=false;calculator_->setMemoryActive(false);dispatch([](CalcEngine*e){e->setVariable("M","0");});schedulePreview();setStatus(trc("Memory cleared"));saveTimer_->start();return;}
 if(lastSerialized_.isEmpty())return;
 const QString value=(op=="M+"||op=="M-")?"("+(memoryValue_.isEmpty()?QString("0"):memoryValue_)+")"+(op=="M+"?"+":"-")+"("+lastSerialized_+")":lastSerialized_;
 const bool valueApproximate=lastValueApproximate_||((op=="M+"||op=="M-")&&memoryValueApproximate_);setBusy(true);
 dispatch([this,value,valueApproximate](CalcEngine*e){bool ok=e->setVariable("M",value,valueApproximate);QString canonical=ok?e->variableValue("M"):QString();QMetaObject::invokeMethod(this,[this,ok,canonical,valueApproximate]{setBusy(false);if(ok&&!canonical.isEmpty()){memoryValue_=canonical;memoryValueApproximate_=valueApproximate;calculator_->setMemoryActive(true);setStatus(trc("Result stored in memory"));schedulePreview();saveTimer_->start();}else setStatus(trc("Memory operation failed"));},Qt::QueuedConnection);});
}
void MainWindow::queueGraph(QStringList expressions,double a,double b,int n){if(!worker_)return;++graphGeneration_;if(std::all_of(expressions.cbegin(),expressions.cend(),[](const QString&s){return s.trimmed().isEmpty();})){graphPending_=false;if(saveTimer_)saveTimer_->start();return;}int gen=graphGeneration_;auto o=options();if(graphRunning_){graphPending_=true;return;}graphRunning_=true;dispatch([this,expressions,a,b,n,o,gen](CalcEngine*e){auto r=e->graph(expressions,a,b,n,o);QMetaObject::invokeMethod(this,[this,r,gen]{graphRunning_=false;if(gen==graphGeneration_)graph_->setGraphResult(r);if(graphPending_){graphPending_=false;graph_->requestPlot();}saveTimer_->start();},Qt::QueuedConnection);});}
QWidget* MainWindow::makeHistory(){
 auto w=new QWidget;auto v=new QVBoxLayout(w);historyControls_=new QGridLayout;historySearch_=new QLineEdit;historySearch_->setMinimumWidth(0);historyControls_->addWidget(historySearch_,0,0);
 historyExportButton_=new QPushButton;addText(historyExportButton_,"Export CSV");historyControls_->addWidget(historyExportButton_,0,1);connect(historyExportButton_,&QPushButton::clicked,this,&MainWindow::historyExport);
 historyClearButton_=new QPushButton;addText(historyClearButton_,"Clear history");historyControls_->addWidget(historyClearButton_,0,2);connect(historyClearButton_,&QPushButton::clicked,this,[this]{historyArchive_.clear();history_=historyArchive_.entries();historyCursor_=-1;historyPage_=0;fillHistory();saveTimer_->start();});v->addLayout(historyControls_);
 historyCount_=new QLabel;historyCount_->setWordWrap(true);v->addWidget(historyCount_);
 historyTable_=new QTableWidget;historyTable_->setColumnCount(3);historyTable_->horizontalHeader()->setMinimumSectionSize(42);historyTable_->setSelectionBehavior(QAbstractItemView::SelectRows);historyTable_->setSelectionMode(QAbstractItemView::SingleSelection);historyTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);historyTable_->setAlternatingRowColors(true);historyTable_->verticalHeader()->hide();historyTable_->verticalHeader()->setDefaultSectionSize(62);historyTable_->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);historyTable_->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);historyTable_->horizontalHeader()->setSectionResizeMode(2,QHeaderView::ResizeToContents);v->addWidget(historyTable_,1);
 connect(historyTable_->horizontalHeader(),&QHeaderView::sectionResized,this,[this]{QTimer::singleShot(0,this,[this]{sizeHistoryRows();});});
 connect(historyTable_,&QTableWidget::itemSelectionChanged,this,[this]{for(int row=0;row<historyTable_->rowCount();++row)for(int col=0;col<2;++col){auto widget=historyTable_->cellWidget(row,col);auto item=historyTable_->item(row,col);if(!widget||!item)continue;QPalette p=historyTable_->palette();if(item->isSelected()){p.setColor(QPalette::Base,p.color(QPalette::Highlight));p.setColor(QPalette::Text,p.color(QPalette::HighlightedText));}else if(row%2)p.setColor(QPalette::Base,p.color(QPalette::AlternateBase));widget->setPalette(p);}});
 auto pager=new QHBoxLayout;historyPrevious_=new QPushButton;addText(historyPrevious_,"Previous");historyNext_=new QPushButton;addText(historyNext_,"Next");pager->addWidget(historyPrevious_);pager->addWidget(historyNext_);v->addLayout(pager);
 connect(historyPrevious_,&QPushButton::clicked,this,[this]{--historyPage_;fillHistory();});connect(historyNext_,&QPushButton::clicked,this,[this]{++historyPage_;fillHistory();});
 auto reuse=new QPushButton;addText(reuse,"Use selected expression");v->addWidget(reuse);auto recall=[this]{auto row=historyTable_->currentRow();if(row<0||!historyTable_->item(row,0))return;const int index=historyTable_->item(row,0)->data(Qt::UserRole).toInt();if(index<0||index>=history_.size())return;recallHistoryEntry(history_[index].toObject());showPage(0);if(!handheld_&&narrow_)compact_->setCurrentWidget(calc_);input_->setFocus();};
 connect(reuse,&QPushButton::clicked,this,recall);connect(historyTable_,&QTableWidget::cellDoubleClicked,this,[recall](int,int){recall();});connect(historySearch_,&QLineEdit::textChanged,this,[this]{historyPage_=0;fillHistory();});return w;
}
void MainWindow::fillHistory(){
 if(!historyTable_)return;historyMatches_.clear();const auto filter=historySearch_->text();
 for(int index=0;index<history_.size();++index){const auto e=history_[index].toObject();if(filter.isEmpty()||e["expression"].toString().contains(filter,Qt::CaseInsensitive)||e["result"].toString().contains(filter,Qt::CaseInsensitive))historyMatches_<<index;}
 const int count=historyMatches_.size();const int pages=qMax(1,(count+HistoryPageSize-1)/HistoryPageSize);historyPage_=qBound(0,historyPage_,pages-1);const int begin=historyPage_*HistoryPageSize,end=qMin(count,begin+HistoryPageSize);
 historyTable_->setUpdatesEnabled(false);historyTable_->setRowCount(end-begin);
 for(int at=begin;at<end;++at){int row=at-begin,index=historyMatches_[at];const auto e=history_[index].toObject();QStringList cells{e["expression"].toString(),e["result"].toString(),QDateTime::fromString(e["time"].toString(),Qt::ISODate).toLocalTime().toString("MM-dd HH:mm")};for(int col=0;col<3;++col){auto item=new QTableWidgetItem(col<2?QString():cells[col]);item->setData(Qt::UserRole,index);item->setToolTip((col==1?exportedValue(e,"result"):cells[col])+(col==0?QString("\n%1 · %2").arg(e["base"].toInt(10)).arg(e["angle"].toInt()==1?"DEG":e["angle"].toInt()==2?"GRAD":"RAD"):QString()));historyTable_->setItem(row,col,item);if(col<2){auto math=new MathEditor;math->setReadOnly(true);math->setUnitTokens(unitTokens_);math->setUnitLabels(unitLabels_);math->setConstantNames(constantTokens_);math->setStructuredInput(col==1?true:structuredInput_);math->setNumberFormat(digitGrouping_,decimalSeparator_);math->setNumberBase(e["base"].toInt(10));if(col==1)math->setResultPresentation(true);if(col==1&&e.contains("resultApproximate"))math->setResultRelation(e["resultApproximate"].toBool()?"≈":"=");math->setPlainText(cells[col]);if(col==1)math->setRecurringDigits(e["resultRepeatStart"].toInt(-1),e["resultRepeatLength"].toInt(),e["resultCopyValue"].toString());math->home();QFont mf=historyTable_->font();mf.setPixelSize(14);math->setFont(mf);math->setAttribute(Qt::WA_TransparentForMouseEvents);math->setFocusPolicy(Qt::NoFocus);math->setToolTip(col==1?exportedValue(e,"result"):cells[col]);QPalette palette=historyTable_->palette();if(row%2)palette.setColor(QPalette::Base,palette.color(QPalette::AlternateBase));math->setPalette(palette);historyTable_->setCellWidget(row,col,math);}}}
 sizeHistoryRows();historyTable_->setUpdatesEnabled(true);historyCount_->setText(trc("%1 of %2 calculations · Page %3 of %4").arg(count).arg(history_.size()).arg(historyPage_+1).arg(pages));historyCount_->setToolTip(trc("The last 10,000 calculations are stored compressed."));historyPrevious_->setEnabled(historyPage_>0);historyNext_->setEnabled(historyPage_+1<pages);
}
void MainWindow::sizeHistoryRows(){
 if(!historyTable_)return;
 for(int row=0;row<historyTable_->rowCount();++row){
  int height=62;
  for(int col=0;col<2;++col)if(auto math=dynamic_cast<MathEditor*>(historyTable_->cellWidget(row,col))){
   math->resize(qMax(1,historyTable_->columnWidth(col)-1),historyTable_->rowHeight(row)-1);
   height=qMax(height,int(std::ceil(math->naturalSize().height()))+4);
  }
  if(historyTable_->rowHeight(row)!=height)historyTable_->setRowHeight(row,height);
 }
}
void MainWindow::recallHistoryEntry(const QJsonObject& row){
 QString snapshotError;selectedResult_={};if(row.contains("snapshotV2"))WorkspaceModel::resultFromJson(row.value("snapshotV2").toObject(),selectedResult_,&snapshotError);else{selectedResult_.ok=!row.value("serialized").toString().isEmpty();selectedResult_.input=row.value("expression").toString();selectedResult_.serialized=row.value("serialized").toString();selectedResult_.result=row.value("result").toString();selectedResult_.valueApproximate=row.value("valueApproximate").toBool();}if(workbenchHub_)workbenchHub_->setResult(selectedResult_);
 ++evaluationGeneration_;++renderGeneration_;formatPending_=false;cancelPreview();
 baseDrafts_[numberBase_]=input_->text();int base=row["base"].toInt(10);if(!QList<int>{2,8,10,16}.contains(base))base=10;
 numberBase_=base;angle_=qBound(0,row["angle"].toInt(0),2);notation_=qBound(0,row["notation"].toInt(0),2);exact_=row["exact"].toBool(true);complexEnabled_=row["complexEnabled"].toBool(true);
 {QSignalBlocker a(baseBox_),b(angleBox_),c(notationBox_),d(exactBox_),e(complexBox_);baseBox_->setCurrentIndex(baseBox_->findData(base));angleBox_->setCurrentIndex(angle_);notationBox_->setCurrentIndex(notation_);exactBox_->setCurrentIndex(exact_?0:1);complexBox_->setCurrentIndex(complexEnabled_?1:0);}
 input_->setNumberBase(base);result_->setNumberBase(base);calculator_->setComplexEnabled(complexEnabled_);calculator_->syncIndicators();
 // Recall restores the selected value, never the previous calculation's value.
 // The engine's Ans is deliberately unchanged until explicit evaluation.
 committedDisplayData_={};committedResult_.clear();committedApproximation_.clear();clearResultDisplay();
 restoringDraft_=true;input_->setText(row["expression"].toString());input_->end(false);restoringDraft_=false;historyCursor_=-1;
 lastSerialized_=row["serialized"].toString();lastValueApproximate_=row["valueApproximate"].toBool();cancelPreview();previewDraft_=false;
 if(!lastSerialized_.isEmpty())reformatResult();
 else if(!row.contains("result"))schedulePreview();
 else if(!continuousEvaluation_&&!row["result"].toString().isEmpty()){
  CalcResult display;display.result=row["result"].toString();display.approximate=row["approximate"].toString();display.resultApproximate=row["resultApproximate"].toBool();display.approximateApproximate=row["approximateApproximate"].toBool();
  display.resultRepeatStart=row["resultRepeatStart"].toInt(-1);display.resultRepeatLength=row["resultRepeatLength"].toInt();display.resultCopyValue=row["resultCopyValue"].toString();
  display.approximateRepeatStart=row["approximateRepeatStart"].toInt(-1);display.approximateRepeatLength=row["approximateRepeatLength"].toInt();display.approximateCopyValue=row["approximateCopyValue"].toString();commitDisplay(display);
 }
 saveTimer_->start();
}
void MainWindow::setApproximation(const QString& canonical,bool approximate,int repeatStart,int repeatLength,const QString& copyValue){
 approx_->setProperty("canonicalNumber",canonical);approx_->setProperty("isApproximate",approximate);approx_->setProperty("canonicalRepeatStart",repeatStart);approx_->setProperty("canonicalRepeatLength",repeatLength);approx_->setProperty("canonicalCopyValue",copyValue);
 // The compact secondary line is scalar-only. Matrix alternatives belong in
 // the full mathematical result view, selected with S⇔D, not flattened text.
 if(canonical.trimmed().startsWith('[')){calculator_->setApproximation({},-1,0,{});approx_->setToolTip({});return;}
 QString prefix=canonical.isEmpty()?QString():QString(approximate?"≈ ":"= ");if(previewDisplayed_)prefix=trc("Preview")+(canonical.isEmpty()?QString():QString(" · ")+prefix);
 const QString display=formatSolutionDisplay(formatNumericDisplay(canonical,digitGrouping_,decimalSeparator_,numberBase_));
 int start=-1,length=0;if(repeatStart>=0&&repeatLength>0&&repeatStart+repeatLength<=canonical.size()){
  const int before=formatNumericDisplay(canonical.left(repeatStart),digitGrouping_,decimalSeparator_,numberBase_).size();
  start=prefix.size()+before;length=formatNumericDisplay(canonical.left(repeatStart+repeatLength),digitGrouping_,decimalSeparator_,numberBase_).size()-before;
 }
 calculator_->setApproximation(prefix+display,start,length,copyValue);approx_->setToolTip(repeatLength>0?copyValue:QString());
}
void MainWindow::refreshApproximation(){setApproximation(approx_->property("canonicalNumber").toString(),approx_->property("isApproximate").toBool(),approx_->property("canonicalRepeatStart").toInt(),approx_->property("canonicalRepeatLength").toInt(),approx_->property("canonicalCopyValue").toString());}
void MainWindow::applyMathPreferences(){
 if(decimalSeparator_==','&&digitGrouping_==2){digitGrouping_=1;if(groupingBox_){QSignalBlocker block(groupingBox_);groupingBox_->setCurrentIndex(1);}}
 for(auto editor:{input_,result_})if(editor){editor->setStructuredInput(editor==result_?true:structuredInput_);editor->setNumberFormat(digitGrouping_,decimalSeparator_);}
 if(calculator_){calculator_->setDecimalSeparator(decimalSeparator_);calculator_->setStructuredInput(structuredInput_);}
 if(graph_)graph_->setMathPresentation(structuredInput_,digitGrouping_,decimalSeparator_);
 if(workbenchHub_)workbenchHub_->graphs->setNumberPresentation(digitGrouping_,decimalSeparator_);
 for(auto tool:{catalogTools_,converterTools_})if(tool)tool->setMathPresentation(structuredInput_,digitGrouping_,decimalSeparator_);
 if(currencyPanel_)currencyPanel_->setMathPresentation(structuredInput_,digitGrouping_,decimalSeparator_);
 if(registersPanel_)registersPanel_->setMathPresentation(digitGrouping_,decimalSeparator_);
 if(approx_)refreshApproximation();
 if(historyTable_&&pages_&&pages_->currentIndex()==1)fillHistory();
}
void MainWindow::changeBase(int base){
 if(!QList<int>{2,8,10,16}.contains(base))return;
 if(baseBox_->currentData().toInt()!=base){QSignalBlocker block(baseBox_);baseBox_->setCurrentIndex(baseBox_->findData(base));}
 calculator_->syncIndicators();
 if(base==numberBase_)return;
 baseDrafts_[numberBase_]=input_->text();baseCursors_[numberBase_]={input_->cursorPosition(),input_->hasSelectedText()?(input_->cursorPosition()==input_->selectionStart()?input_->selectionStart()+int(input_->selectedText().size()):input_->selectionStart()):-1};
 cancelPreview();previewDraft_=false;previewDisplayed_=false;baseResultDetached_=!lastSerialized_.isEmpty();
 restoringDraft_=true;numberBase_=base;input_->setNumberBase(base);result_->setNumberBase(base);input_->setText(baseDrafts_.value(base));
 const auto cursor=baseCursors_.value(base,{int(input_->text().size()),-1});
 if(cursor.second>=0)input_->setSelection(cursor.second,cursor.first-cursor.second);else input_->setCursorPosition(cursor.first);
 restoringDraft_=false;calculator_->setLiteralInputBase(input_->explicitLiteralBase());clearResultDisplay();historyCursor_=-1;historyDraft_.clear();
 reformatResult();if(saveTimer_)saveTimer_->start();
}
void MainWindow::historyExport(){QString p=QFileDialog::getSaveFileName(this,trc("Export history"),QDir(lastDir_).filePath("EMCICalc-history.csv"),"CSV (*.csv)");if(p.isEmpty())return;QSaveFile f(p);if(!f.open(QIODevice::WriteOnly)){setStatus(trc("Could not save file"));return;}QByteArray out="expression,result,approximate,time_utc,base,angle,notation,exact,serialized\r\n";for(const auto&row:history_){auto e=row.toObject();QStringList cells;for(const auto&key:{"expression","result","approximate","time","base","angle","notation","exact","serialized"})cells<<csv(exportedValue(e,key));out+=cells.join(',').toUtf8()+"\r\n";}f.write(out);if(!f.commit()){setStatus(trc("Could not save file"));return;}lastDir_=QFileInfo(p).absolutePath();graph_->setExportDirectory(lastDir_);saveTimer_->start();setStatus(trc("File saved"));}
QWidget* MainWindow::makeToolbox(){
 toolboxTabs_=new QTabWidget;toolboxTabs_->setObjectName("toolboxTabs");
 auto w=new QWidget;auto v=new QVBoxLayout(w);auto note=new QLabel;addText(note,"Choose a function, fill its empty fields, then evaluate. Constants and unit conversion have their own tools.");note->setWordWrap(true);v->addWidget(note);
 auto scroll=new QScrollArea;scroll->setWidgetResizable(true);scroll->setFrameShape(QFrame::NoFrame);scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);auto content=new QWidget;auto grid=new QGridLayout(content);toolboxGrid_=grid;
 const QList<QPair<const char*,QString>> tools={{"Solve equation","solve("},{"Solve system","multisolve("},{"Derivative","diff("},{"Integral","integrate("},{"Limit","limit("},{"Simplify","@expression"},{"Unit conversion","@units"},{"Currency converter","@currency"},{"Complex numbers","@complexTemplate"},{"Matrix determinant","@detTemplate"},{"Matrix inverse","@inverseTemplate"},{"Mean","mean("},{"Median","median("},{"Standard deviation","stdev("},{"Combinations","binomial("},{"Integer factors","factor("},{"Number bases","@bases"},{"Greatest common divisor","gcd("},{"Physical constants","@constants"},{"Store variable","@assignTemplate"},{"Fractions","@fraction"},{"Statistics","@statistics"},{"Matrix","@matrix"},{"Transpose","transpose("}};
 for(int i=0;i<tools.size();++i){auto button=new QPushButton;addText(button,tools[i].first);button->setMinimumHeight(47);button->setMinimumWidth(0);button->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);toolboxButtons_<<button;grid->addWidget(button,i/2,i%2);auto token=tools[i].second;
 connect(button,&QPushButton::clicked,this,[this,token,button]{
  if(token=="@constants"||token=="@currency"||token=="@units"||token=="@statistics"||token=="@matrix"){handleAction(token);return;}
  if(token=="@bases"){auto menu=new QMenu(button);menu->setAttribute(Qt::WA_DeleteOnClose);for(int base:{2,8,10,16}){const QString label=base==2?"BIN":base==8?"OCT":base==10?"DEC":"HEX";auto action=menu->addAction(label);action->setCheckable(true);action->setChecked(numberBase_==base);connect(action,&QAction::triggered,this,[this,base]{changeBase(base);showPage(0);input_->setFocus();});}menu->popup(button->mapToGlobal(QPoint(0,button->height())));return;}
  startTemplate(token);
 });}
 grid->setRowStretch((tools.size()+1)/2,1);scroll->setWidget(content);v->addWidget(scroll);
 toolboxTabs_->addTab(w,trc("Functions"));registersPanel_=new RegistersPanel;toolboxTabs_->addTab(registersPanel_,trc("Registers"));registersPanel_->onClear=[this](QStringList names){clearRegisters(names);};
 connect(toolboxTabs_,&QTabWidget::currentChanged,this,[this](int index){if(index==1)refreshRegisters();});return toolboxTabs_;
}
void MainWindow::openMatrixTools(){
 const auto dimensions=input_->matrixDimensions();catalogTools_->setMatrixDimensions(dimensions.height(),dimensions.width());
 catalogTools_->setView(CalculatorTools::View::Matrix);showPage(7);
}
void MainWindow::startTemplate(QString token){
 if(token=="@currency"){showPage(9);return;}
 if(token=="["&&structuredInput_){openMatrixTools();return;}
 if(token=="@detTemplate")token="det(";
 if(token=="@inverseTemplate")token="inverse(";
 const QMap<QString,QString> structures{{"@expression","(□)"},{"@complexTemplate","(□)+(□)*i"},{"@assignTemplate","□:=□"}};
 if(structures.contains(token)){
  const int start=input_->hasSelectedText()?input_->selectionStart():input_->cursorPosition();input_->insert(structures.value(token));
  const int slot=input_->text().indexOf(QChar(0x25a1),start);if(slot>=0)input_->setCursorPosition(slot);
 }else input_->insertTemplate(token);
 showPage(0);if(!handheld_&&narrow_)compact_->setCurrentWidget(calc_);input_->setFocus();
}
QWidget* MainWindow::makeSettings(){
 auto w=new QWidget;settingsPage_=w;auto v=new QVBoxLayout(w);auto title=new QLabel;settingsTitle_=title;addText(title,"Make EMCICalc yours");QFont f=font();f.setPointSize(18);f.setBold(true);title->setFont(f);title->setWordWrap(true);v->addWidget(title);
 auto form=new QFormLayout;settingsForm_=form;form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);auto addRow=[&](const char*label,QWidget*field){auto l=new QLabel;addText(l,label);l->setBuddy(field);settingsRows_.append({l,field});field->setProperty("settingsOriginalMinimumHeight",field->minimumHeight());form->addRow(l,field);};
 auto choice=[&](const QStringList& names,const QString& selected){auto box=new QComboBox;for(const auto&name:names)box->addItem(trc(name.toUtf8().constData()),name);box->setCurrentIndex(qMax(0,box->findData(selected)));box->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);box->setMinimumContentsLength(8);box->setMinimumWidth(0);box->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);return box;};
 designBox_=choice({"Desktop","Calculator"},design_);addRow("Mode",designBox_);
 calculatorThemeBox_=choice(calculatorThemeNames(),calculatorTheme_);addRow("Calculator theme",calculatorThemeBox_);
 calculatorLayoutBox_=choice({"Pocket","Compact","Expanded"},calculatorLayout_);addRow("Calculator layout",calculatorLayoutBox_);
 desktopEffectsBox_=choice({"Flat","Glass","Raised","Frosted Glass"},QStringList{"Flat","Glass","Raised","Frosted Glass"}[desktopEffects_]);addRow("Desktop effects",desktopEffectsBox_);
 themeBox_=choice(desktopThemeNames(),theme_);addRow("Desktop theme",themeBox_);
 appearanceBox_=choice({"System","Light","Dark"},appearance_);addRow("Appearance",appearanceBox_);
 fontSizeBox_=choice({"System","Small","Medium","Large"},fontSizeMode_);fontSizeBox_->setObjectName("fontSizeMode");addRow("Font size",fontSizeBox_);
 decimalPlacesBox_=new QSpinBox;decimalPlacesBox_->setRange(0,200);decimalPlacesBox_->setValue(decimalPlaces_);addRow("FIX decimal places",decimalPlacesBox_);
 percentModeBox_=choice({"Relative percentage","Direct percentage"},percentMode_?"Direct percentage":"Relative percentage");addRow("Percentage",percentModeBox_);
 implicitModeBox_=choice({"Adaptive (existing behavior)","Weak multiplication","Strong multiplication"},QStringList{"Adaptive (existing behavior)","Weak multiplication","Strong multiplication"}[implicitMode_]);addRow("Implicit multiplication",implicitModeBox_);
 connect(fontSizeBox_,&QComboBox::currentIndexChanged,this,[this]{if(deferSettingsChange())return;fontSizeMode_=fontSizeBox_->currentData().toString();DesktopFont::setSizeMode(fontSizeMode_);applyTheme();updateAuxiliaryLayouts();saveTimer_->start();});
 connect(decimalPlacesBox_,&QSpinBox::valueChanged,this,[this](int n){if(deferSettingsChange())return;decimalPlaces_=n;reformatResult();saveTimer_->start();});
 connect(percentModeBox_,&QComboBox::currentIndexChanged,this,[this](int n){if(deferSettingsChange())return;percentMode_=n;invalidateResult();saveTimer_->start();});
 connect(implicitModeBox_,&QComboBox::currentIndexChanged,this,[this](int n){if(deferSettingsChange())return;implicitMode_=n;invalidateResult();saveTimer_->start();});
 layoutBox_=choice({"Adaptive","Calculator left","Calculator right","Tabbed"},layout_);addRow("Desktop layout",layoutBox_);
 inputStyleBox_=choice({"Natural","Linear"},structuredInput_?"Natural":"Linear");addRow("Input style",inputStyleBox_);
 mixedFractionsBox_=choice({"Improper","Mixed"},mixedFractions_?"Mixed":"Improper");mixedFractionsBox_->setObjectName("exactFractions");addRow("Exact fractions",mixedFractionsBox_);
 continuousEvaluationBox_=choice({"Off","On"},continuousEvaluation_?"On":"Off");continuousEvaluationBox_->setObjectName("continuousEvaluation");addRow("Continuous evaluation",continuousEvaluationBox_);
 keyTooltipsBox_=choice({"Show","Hide"},keyTooltipsEnabled_?"Show":"Hide");keyTooltipsBox_->setObjectName("keyTooltips");addRow("Key tooltips",keyTooltipsBox_);
 screenTooltipsBox_=choice({"Show","Hide"},screenTooltipsEnabled_?"Show":"Hide");screenTooltipsBox_->setObjectName("screenTooltips");addRow("Screen tooltips",screenTooltipsBox_);
 groupingBox_=choice({"None","Space","Comma"},QStringList{"None","Space","Comma"}[digitGrouping_]);addRow("Digit grouping",groupingBox_);
 decimalSeparatorBox_=choice({"Dot","Comma"},decimalSeparator_==','?"Comma":"Dot");addRow("Decimal separator",decimalSeparatorBox_);
 complexBox_=choice({"Real numbers","Complex numbers"},complexEnabled_?"Complex numbers":"Real numbers");addRow("Number domain",complexBox_);
 languageBox_=new QComboBox;auto codes=languageCodes(),names=languageNames();for(int i=0;i<codes.size();++i)languageBox_->addItem(names[i],codes[i]);languageBox_->setCurrentIndex(qMax(0,languageBox_->findData(language_)));languageBox_->setMinimumWidth(0);languageBox_->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);addRow("Language",languageBox_);
 precisionBox_=new QSpinBox;precisionBox_->setRange(10,200);precisionBox_->setValue(precision_);addRow("Significant digits",precisionBox_);v->addLayout(form);
 for(const auto *name:{"Appearance","Input","Number notation","Help"}){
  auto section=DesktopWorkbench::card(w);section->setObjectName("settingsSection");section->setBackgroundRole(QPalette::Button);auto row=new QHBoxLayout(section);row->setContentsMargins(10,8,10,8);auto heading=DesktopWorkbench::sectionTitle({},section);addText(heading,name);QFont font=heading->font();font.setBold(true);heading->setFont(font);row->addWidget(heading);section->hide();settingsSections_.append(section);
 }
 settingsAboutTitle_=DesktopWorkbench::sectionTitle({},w);addText(settingsAboutTitle_,"About");settingsAboutTitle_->hide();v->addWidget(settingsAboutTitle_);
 settingsAbout_=DesktopWorkbench::card(w);settingsAbout_->setObjectName("settingsAbout");auto about=new QVBoxLayout(settingsAbout_);about->setContentsMargins(0,0,0,0);v->addWidget(settingsAbout_);
 auto note=new QLabel;settingsSummary_=note;note->setObjectName("settingsSummary");note->setWordWrap(true);QFont small=font();small.setPointSizeF(qMax(10.0,font().pointSizeF()));note->setFont(small);about->addWidget(note);
 settingsVersion_=new QLabel;settingsVersion_->setObjectName("settingsVersion");settingsVersion_->setFont(small);settingsVersion_->setWordWrap(true);settingsVersion_->setTextInteractionFlags(Qt::TextSelectableByMouse|Qt::TextSelectableByKeyboard);about->addWidget(settingsVersion_);
 auto storage=new QLabel;addText(storage,"Saved data");storage->setFont(small);about->addWidget(storage);
 auto location=new QLabel(QDir::toNativeSeparators(QFileInfo(configPath_).absolutePath()));location->setTextFormat(Qt::PlainText);location->setWordWrap(true);location->setFont(small);location->setTextInteractionFlags(Qt::TextSelectableByMouse|Qt::TextSelectableByKeyboard);location->setObjectName("settingsStoragePath");location->setMinimumWidth(0);location->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);about->addWidget(location);v->addStretch();
 connect(designBox_,&QComboBox::currentIndexChanged,this,[this]{if(deferSettingsChange())return;design_=designBox_->currentData().toString();applyDesign();if(!handheld_&&pages_->currentIndex()==3){settingsReturnPage_=0;beginSettingsDraft();}saveTimer_->start();});
 connect(calculatorThemeBox_,&QComboBox::currentIndexChanged,this,[this]{if(deferSettingsChange())return;calculatorTheme_=calculatorThemeBox_->currentData().toString();calculator_->setCalculatorTheme(calculatorTheme_);applyTheme();saveTimer_->start();});
 connect(calculatorLayoutBox_,&QComboBox::currentIndexChanged,this,[this]{if(deferSettingsChange())return;calculatorLayout_=calculatorLayoutBox_->currentData().toString();calculator_->setCalculatorLayout(calculatorLayout_);keys_=calculator_->keyButtons();refreshKeys();saveTimer_->start();});
 connect(inputStyleBox_,&QComboBox::currentIndexChanged,this,[this](int n){if(deferSettingsChange())return;structuredInput_=n==0;applyMathPreferences();saveTimer_->start();});
 connect(mixedFractionsBox_,&QComboBox::currentIndexChanged,this,[this](int index){if(deferSettingsChange())return;mixedFractions_=index==1;schedulePreview();reformatResult();saveTimer_->start();});
 connect(continuousEvaluationBox_,&QComboBox::currentIndexChanged,this,[this](int index){if(deferSettingsChange())return;continuousEvaluation_=index==1;schedulePreview();saveTimer_->start();});
 connect(keyTooltipsBox_,&QComboBox::currentIndexChanged,this,[this](int index){if(deferSettingsChange())return;keyTooltipsEnabled_=index==0;calculator_->setKeyTooltipsEnabled(keyTooltipsEnabled_);saveTimer_->start();});
 connect(screenTooltipsBox_,&QComboBox::currentIndexChanged,this,[this](int index){if(deferSettingsChange())return;screenTooltipsEnabled_=index==0;calculator_->setScreenTooltipsEnabled(screenTooltipsEnabled_);saveTimer_->start();});
 connect(groupingBox_,&QComboBox::currentIndexChanged,this,[this](int n){if(deferSettingsChange())return;digitGrouping_=n;applyMathPreferences();saveTimer_->start();});
 connect(decimalSeparatorBox_,&QComboBox::currentIndexChanged,this,[this](int n){if(deferSettingsChange())return;decimalSeparator_=n==1?',':'.';applyMathPreferences();saveTimer_->start();});
 connect(desktopEffectsBox_,&QComboBox::currentIndexChanged,this,[this](int n){if(deferSettingsChange())return;desktopEffects_=n;applyTheme();saveTimer_->start();});
 connect(themeBox_,&QComboBox::currentIndexChanged,this,[this]{if(deferSettingsChange())return;theme_=themeBox_->currentData().toString();applyTheme();saveTimer_->start();});
 connect(appearanceBox_,&QComboBox::currentIndexChanged,this,[this]{if(deferSettingsChange())return;appearance_=appearanceBox_->currentData().toString();applyTheme();saveTimer_->start();});
 connect(layoutBox_,&QComboBox::currentIndexChanged,this,[this]{if(deferSettingsChange())return;layout_=layoutBox_->currentData().toString();updateLayout();saveTimer_->start();});
 connect(complexBox_,&QComboBox::currentIndexChanged,this,[this](int index){if(deferSettingsChange())return;complexEnabled_=index==1;calculator_->setComplexEnabled(complexEnabled_);invalidateResult();schedulePreview();saveTimer_->start();});
 connect(languageBox_,&QComboBox::currentIndexChanged,this,[this]{if(deferSettingsChange())return;language_=languageBox_->currentData().toString();updateTranslations();saveTimer_->start();});
 connect(precisionBox_,&QSpinBox::valueChanged,this,[this](int n){if(deferSettingsChange())return;precision_=n;schedulePreview();reformatResult();saveTimer_->start();});
 auto scroll=new SettingsScrollArea;
 settingsActions_=new QWidget;settingsActions_->setObjectName("settingsActions");auto actions=new QHBoxLayout(settingsActions_);actions->setContentsMargins(8,8,8,8);
 settingsBack_=new QPushButton;settingsBack_->setObjectName("settingsBack");addText(settingsBack_,"Back");settingsBack_->setIcon(emciBackIcon(palette().color(QPalette::WindowText)));actions->addWidget(settingsBack_);actions->addStretch();
 settingsCancel_=new QPushButton;settingsCancel_->setObjectName("settingsCancel");addText(settingsCancel_,"Cancel");actions->addWidget(settingsCancel_);
 settingsApply_=new QPushButton;settingsApply_->setObjectName("settingsApply");addText(settingsApply_,"Apply");actions->addWidget(settingsApply_);
 connect(settingsBack_,&QPushButton::clicked,this,[this]{goBack();});connect(settingsCancel_,&QPushButton::clicked,this,[this]{goBack();});connect(settingsApply_,&QPushButton::clicked,this,[this]{applySettingsDraft();});
 scroll->setActions(settingsActions_);scroll->setActionsVisible(!handheld_);scroll->setFrameShape(QFrame::NoFrame);scroll->setWidgetResizable(true);scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);scroll->setWidget(w);
 auto wheelGuard=new SettingsWheelGuard(scroll);
 for(auto combo:w->findChildren<QComboBox*>())wheelGuard->protect(combo);
 for(auto spin:w->findChildren<QAbstractSpinBox*>())wheelGuard->protect(spin);
 return scroll;
}
QJsonObject MainWindow::settingsControlValues() const {
 return {{"mode",designBox_->currentData().toString()},{"calculatorTheme",calculatorThemeBox_->currentData().toString()},{"calculatorLayout",calculatorLayoutBox_->currentData().toString()},
  {"theme",themeBox_->currentData().toString()},{"appearance",appearanceBox_->currentData().toString()},{"layout",layoutBox_->currentData().toString()},{"language",languageBox_->currentData().toString()},
  {"structuredInput",inputStyleBox_->currentIndex()==0},{"mixedFractions",mixedFractionsBox_->currentIndex()==1},{"continuousEvaluation",continuousEvaluationBox_->currentIndex()==1},
  {"keyTooltipsEnabled",keyTooltipsBox_->currentIndex()==0},{"screenTooltipsEnabled",screenTooltipsBox_->currentIndex()==0},{"digitGrouping",groupingBox_->currentIndex()},
  {"decimalSeparator",decimalSeparatorBox_->currentIndex()==1?",":"."},{"desktopEffects",desktopEffectsBox_->currentIndex()},{"complexEnabled",complexBox_->currentIndex()==1},{"precision",precisionBox_->value()},{"fontSizeMode",fontSizeBox_->currentData().toString()},{"decimalPlacesV2",decimalPlacesBox_->value()},{"percentModeV2",percentModeBox_->currentIndex()},{"implicitModeV2",implicitModeBox_->currentIndex()}};
}
void MainWindow::syncSettingsControls(){
 auto select=[](QComboBox *box,const QVariant &value){QSignalBlocker block(box);box->setCurrentIndex(qMax(0,box->findData(value)));};
 select(fontSizeBox_,fontSizeMode_);
 {QSignalBlocker b(decimalPlacesBox_),p(percentModeBox_),i(implicitModeBox_);decimalPlacesBox_->setValue(decimalPlaces_);percentModeBox_->setCurrentIndex(percentMode_);implicitModeBox_->setCurrentIndex(implicitMode_);}
 select(designBox_,design_);select(calculatorThemeBox_,calculatorTheme_);select(calculatorLayoutBox_,calculatorLayout_);select(themeBox_,theme_);select(appearanceBox_,appearance_);select(layoutBox_,layout_);select(languageBox_,language_);
 auto index=[](QComboBox *box,int value){QSignalBlocker block(box);box->setCurrentIndex(value);};
 index(inputStyleBox_,structuredInput_?0:1);index(mixedFractionsBox_,mixedFractions_?1:0);index(continuousEvaluationBox_,continuousEvaluation_?1:0);index(keyTooltipsBox_,keyTooltipsEnabled_?0:1);index(screenTooltipsBox_,screenTooltipsEnabled_?0:1);
 index(groupingBox_,digitGrouping_);index(decimalSeparatorBox_,decimalSeparator_==','?1:0);index(desktopEffectsBox_,desktopEffects_);index(complexBox_,complexEnabled_?1:0);
 {QSignalBlocker block(precisionBox_);precisionBox_->setValue(precision_);}
}
void MainWindow::updateSettingsPresentation(){
 if(!settingsForm_||!settingsAbout_)return;
 const QString mode=handheld_?"Calculator":"Desktop";
 const bool orderChanged=settingsPage_->property("settingsPresentationMode").toString()!=mode;
 if(orderChanged){
  // Reorder existing label/control pairs only when presentation mode changes.
  // Taking layout items leaves each widget, edit buffer, focus and signal intact.
  auto focused=QPointer<QWidget>(QApplication::focusWidget());
  while(settingsForm_->rowCount()){auto row=settingsForm_->takeRow(0);delete row.labelItem;delete row.fieldItem;}
  for(auto *section:settingsSections_)section->hide();
  auto append=[this](QWidget *field){for(const auto &row:settingsRows_)if(row.second==field){settingsForm_->addRow(row.first,row.second);return;}};
  if(handheld_){
   const QList<QList<QWidget*>> groups={
    {designBox_,calculatorThemeBox_,calculatorLayoutBox_,fontSizeBox_,languageBox_},
    {inputStyleBox_,continuousEvaluationBox_,complexBox_,percentModeBox_,implicitModeBox_},
    {precisionBox_,decimalPlacesBox_,mixedFractionsBox_,groupingBox_,decimalSeparatorBox_},
    {keyTooltipsBox_,screenTooltipsBox_}};
   for(int i=0;i<groups.size();++i){settingsForm_->addRow(settingsSections_[i]);settingsSections_[i]->show();for(auto *field:groups[i])append(field);}
   for(QWidget *field:{desktopEffectsBox_,themeBox_,appearanceBox_,layoutBox_})append(field);
  }else for(const auto &row:settingsRows_)settingsForm_->addRow(row.first,row.second);
  settingsTitle_->setVisible(!handheld_);settingsAboutTitle_->setVisible(handheld_);
  settingsPage_->setProperty("settingsPresentationMode",mode);
  if(focused&&focused->isVisible())focused->setFocus(Qt::OtherFocusReason);
 }
 const auto margins=handheld_?QMargins(12,10,12,14):QMargins(9,9,9,9);
 if(settingsPage_->layout()->contentsMargins()!=margins)settingsPage_->layout()->setContentsMargins(margins);
 settingsForm_->setHorizontalSpacing(handheld_?16:-1);settingsForm_->setVerticalSpacing(handheld_?8:-1);
 const int wrapWidth=handheld_?qRound(560*DesktopFont::presetScale()):540;
 settingsForm_->setRowWrapPolicy(width()<wrapWidth?QFormLayout::WrapAllRows:QFormLayout::DontWrapRows);
 if(handheld_||settingsPage_->property("twScope").toBool())DesktopWorkbench::apply(settingsPage_,qApp->palette());
 for(const auto &row:settingsRows_){
  row.second->setAccessibleName(row.first->text());
  const int minimum=handheld_?DesktopWorkbench::controlHeight(row.second):row.second->property("settingsOriginalMinimumHeight").toInt();
  if(!qobject_cast<QAbstractSpinBox*>(row.second)&&row.second->minimumHeight()!=minimum)row.second->setMinimumHeight(minimum);
 }
 for(auto *spin:{precisionBox_,decimalPlacesBox_}){DesktopStepper::setActive(spin,handheld_);if(handheld_){auto policy=spin->sizePolicy();policy.setHorizontalPolicy(QSizePolicy::Expanding);spin->setSizePolicy(policy);}spin->setMaximumWidth(handheld_?240:QWIDGETSIZE_MAX);}
 const auto colors=DesktopWorkbench::colors(qApp->palette());
 const QString sectionStyle=QString("QFrame#settingsSection{background:%1;border:1px solid %2;border-radius:8px;}").arg(colors.raised.name(),colors.border.name());
 for(auto *section:settingsSections_)if(section->styleSheet()!=sectionStyle)section->setStyleSheet(sectionStyle);
 const QString aboutStyle=handheld_?QString("QFrame#settingsAbout{background:%1;border:1px solid %2;border-radius:8px;}").arg(colors.background.name(),colors.border.name()):QString("QFrame#settingsAbout{border:0;background:transparent;}");
 if(settingsAbout_->styleSheet()!=aboutStyle)settingsAbout_->setStyleSheet(aboutStyle);
 settingsAbout_->layout()->setContentsMargins(handheld_?12:0,handheld_?12:0,handheld_?12:0,handheld_?12:0);
 if(orderChanged){
  // Layout moves do not change Qt's creation-time focus chain. Keep keyboard
  // traversal in the same visual order, including the custom stepper actions.
  QList<QWidget*> order;
  for(int row=0;row<settingsForm_->rowCount();++row)if(auto *item=settingsForm_->itemAt(row,QFormLayout::FieldRole))if(auto *field=item->widget()){
   if(!(field->focusPolicy()&Qt::TabFocus))continue;order.append(field);
   if(handheld_&&qobject_cast<QAbstractSpinBox*>(field))for(const auto *name:{"desktopStepDecrease","desktopStepIncrease"})if(auto *button=field->findChild<QAbstractButton*>(name))order.append(button);
  }
  order.append(settingsVersion_);if(auto *path=settingsPage_->findChild<QLabel*>("settingsStoragePath"))order.append(path);
  if(!handheld_)order.append({settingsBack_,settingsCancel_,settingsApply_});
  for(int i=1;i<order.size();++i)QWidget::setTabOrder(order[i-1],order[i]);
 }
}
void MainWindow::updateSettingsUi(){
 if(!settingsForm_)return;updateSettingsPresentation();
 if(navigation_&&pages_){QSignalBlocker block(navigation_);const bool desktopSettings=!handheld_&&pages_->currentIndex()==3;navigation_->setTabVisible(3,desktopSettings);if(desktopSettings)navigation_->setCurrentIndex(3);}
 const bool selectedCalculator=settingsDraftActive_?designBox_->currentData().toString()=="Calculator":handheld_;
 for(QWidget *field:{desktopEffectsBox_,layoutBox_,themeBox_,appearanceBox_})settingsForm_->setRowVisible(field,!selectedCalculator);
 for(QWidget *field:{calculatorThemeBox_,calculatorLayoutBox_})settingsForm_->setRowVisible(field,selectedCalculator);
 if(settingsActions_){
  if(auto scroll=dynamic_cast<SettingsScrollArea*>(settingsActions_->parentWidget()))scroll->setActionsVisible(!handheld_);
  settingsApply_->setEnabled(settingsDraftActive_&&settingsControlValues()!=settingsDraftBaseline_);
  settingsBack_->setToolTip(trc("Back"));settingsBack_->setAccessibleName(trc("Back"));
 }
 if(settingsSummary_)settingsSummary_->setText(trc(handheld_?"Changes are saved automatically. Each mode keeps its own appearance and window size. Calculations stay local.":"Apply saves your changes. Cancel or Back discards pending changes. Calculations stay local."));
}
void MainWindow::beginSettingsDraft(){
 syncSettingsControls();settingsDraftBaseline_=settingsControlValues();settingsDraftActive_=true;updateSettingsUi();
}
void MainWindow::discardSettingsDraft(){
 // Live preferences never changed, so cancel needs no theme/layout/engine work.
 settingsDraftActive_=false;syncSettingsControls();settingsDraftBaseline_={};updateSettingsUi();
}
bool MainWindow::deferSettingsChange(){
 if(!settingsDraftActive_)return false;
 updateSettingsUi();return true;
}
void MainWindow::applySettingsDraft(){
 if(!settingsDraftActive_)return;
 const auto draft=settingsControlValues();if(draft==settingsDraftBaseline_)return;
 const auto old=settingsDraftBaseline_;
 const bool designChanged=draft["mode"]!=old["mode"]||draft["calculatorTheme"]!=old["calculatorTheme"]||draft["calculatorLayout"]!=old["calculatorLayout"];
 const bool fontChanged=draft["fontSizeMode"]!=old["fontSizeMode"];
 const bool themeChanged=draft["theme"]!=old["theme"]||draft["appearance"]!=old["appearance"]||draft["desktopEffects"]!=old["desktopEffects"];
 const bool mathChanged=draft["structuredInput"]!=old["structuredInput"]||draft["digitGrouping"]!=old["digitGrouping"]||draft["decimalSeparator"]!=old["decimalSeparator"];
 const bool domainChanged=draft["complexEnabled"]!=old["complexEnabled"]||draft["percentModeV2"]!=old["percentModeV2"]||draft["implicitModeV2"]!=old["implicitModeV2"];
 const bool outputChanged=draft["precision"]!=old["precision"]||draft["mixedFractions"]!=old["mixedFractions"]||draft["decimalPlacesV2"]!=old["decimalPlacesV2"];
 const bool previewChanged=domainChanged||outputChanged||draft["continuousEvaluation"]!=old["continuousEvaluation"];
 design_=draft["mode"].toString();calculatorTheme_=draft["calculatorTheme"].toString();calculatorLayout_=draft["calculatorLayout"].toString();
 theme_=draft["theme"].toString();appearance_=draft["appearance"].toString();layout_=draft["layout"].toString();language_=draft["language"].toString();
 structuredInput_=draft["structuredInput"].toBool();mixedFractions_=draft["mixedFractions"].toBool();continuousEvaluation_=draft["continuousEvaluation"].toBool();
 keyTooltipsEnabled_=draft["keyTooltipsEnabled"].toBool();screenTooltipsEnabled_=draft["screenTooltipsEnabled"].toBool();digitGrouping_=draft["digitGrouping"].toInt();
 decimalSeparator_=draft["decimalSeparator"].toString().at(0);desktopEffects_=draft["desktopEffects"].toInt();complexEnabled_=draft["complexEnabled"].toBool();precision_=draft["precision"].toInt();
 fontSizeMode_=draft["fontSizeMode"].toString();decimalPlaces_=draft["decimalPlacesV2"].toInt();percentMode_=draft["percentModeV2"].toInt();implicitMode_=draft["implicitModeV2"].toInt();
 settingsDraftActive_=false;
 // Commit each affected subsystem once. In particular, mode switching already
 // applies its theme and layout, so do not repolish the whole app a second time.
 if(fontChanged)DesktopFont::setSizeMode(fontSizeMode_);
 if(draft["language"]!=old["language"])updateTranslations();
 if(designChanged)applyDesign();
 else{if(themeChanged||fontChanged)applyTheme();if(draft["layout"]!=old["layout"])updateLayout();if(domainChanged)calculator_->setComplexEnabled(complexEnabled_);}
 if(mathChanged)applyMathPreferences();
 calculator_->setKeyTooltipsEnabled(keyTooltipsEnabled_);calculator_->setScreenTooltipsEnabled(screenTooltipsEnabled_);
 if(domainChanged)invalidateResult();
 if(previewChanged)schedulePreview();
 if(outputChanged&&!domainChanged)reformatResult();
 syncSettingsControls();
 if(!handheld_)beginSettingsDraft();else{settingsDraftBaseline_={};updateSettingsUi();}
 updateAuxiliaryLayouts();
 if(saveTimer_)saveTimer_->stop();saveSettings();
}

QWidget* MainWindow::makeMenu(){
 auto w=new QWidget;auto v=new QVBoxLayout(w);v->setContentsMargins(0,0,0,6);v->setSpacing(2);modeMenu_=new CalculatorMenu;modeMenu_->onAction=[this](QString action){menuAction(action);};v->addWidget(modeMenu_,1);
 auto memoryRow=new QHBoxLayout;memoryRow->setContentsMargins(8,0,8,0);const char*memoryTips[]={"Store result in memory","Recall memory","Add result to memory","Clear memory"};int mi=0;for(const QString op:QStringList{"MS","MR","M+","MC"}){auto b=new QPushButton(op);addText(b,memoryTips[mi++]);b->setProperty("memoryCaption",op);memoryRow->addWidget(b);connect(b,&QPushButton::clicked,this,[this,op]{memory(op);if(op=="MR"){showPage(0);input_->setFocus();}});}v->addLayout(memoryRow);
 auto workbenches=new QPushButton;addText(workbenches,"Workbenches");connect(workbenches,&QPushButton::clicked,this,[this]{showWorkspace();});v->addWidget(workbenches);
 auto footer=new QHBoxLayout;footer->setContentsMargins(8,0,8,0);auto copy=new QPushButton;addText(copy,"Copy");connect(copy,&QPushButton::clicked,this,[this]{copyResult();});footer->addWidget(copy);auto full=new QPushButton("F11");connect(full,&QPushButton::clicked,this,[this]{full_->click();});footer->addWidget(full);v->addLayout(footer);return w;
}
void MainWindow::menuAction(QString action){
 if(action=="workbenches"){showWorkspace();return;}
 if(action=="calculate"){showPage(0);input_->setFocus();}
 else if(action=="graph")showGraph();else if(action=="history")showPage(1);else if(action=="toolbox")showPage(2);else if(action=="settings")showPage(3);else if(action=="guide")showPage(4);
 else if(action=="solve"||action=="matrix")startTemplate(action=="solve"?"solve(":"[");
}
QWidget* MainWindow::makeGuide(){guide_=new QTextBrowser;guide_->setOpenExternalLinks(false);return guide_;}
void MainWindow::updateTranslations(){
 if(workbenchHub_)workbenchHub_->retranslate([this](const char*s){return trc(s);});
 if(navigation_->count()>5)navigation_->setTabText(5,trc("Workbenches"));
 const char*nav[]={"Workspace","History","Toolbox","Settings","Guide"};for(int i=0;i<5;++i)navigation_->setTabText(i,trc(nav[i]));brandSub_->setText(trc("Scientific & graphing calculator"));full_->setToolTip(trc("Toggle fullscreen"));for(auto gear:{gear_,auxGear_}){gear->setToolTip(trc("Settings"));gear->setAccessibleName(trc("Settings"));}back_->setToolTip(trc("Back"));back_->setAccessibleName(trc("Back"));auxMenu_->setToolTip(trc("Calculator menu"));for(auto pair:labels_){auto text=trc(pair.second.toUtf8().constData());if(auto b=qobject_cast<QPushButton*>(pair.first)){if(b->property("memoryCaption").isValid()){b->setText(b->property("memoryCaption").toString());b->setToolTip(text);b->setAccessibleName(text);}else b->setText(text);}else if(auto l=qobject_cast<QLabel*>(pair.first))l->setText(text);}
 input_->setPlaceholderText(trc("Enter an expression…"));input_->setAccessibleName(trc("Expression"));result_->setAccessibleName(trc("Result"));angleBox_->setAccessibleName(trc("Angle unit"));exactBox_->setAccessibleName(trc("Result format"));exactBox_->setItemText(0,trc("Exact"));exactBox_->setItemText(1,trc("Decimal"));historySearch_->setPlaceholderText(trc("Search history…"));historyTable_->setHorizontalHeaderLabels({trc("Expression"),trc("Result"),trc("Time")});
 const char*tips[]={"Store result in memory","Recall memory","Add result to memory","Clear memory"};for(int i=0;i<memoryButtons_.size();++i)memoryButtons_[i]->setToolTip(trc(tips[i]));
 for(auto box:{designBox_,themeBox_,appearanceBox_,layoutBox_,calculatorThemeBox_,calculatorLayoutBox_,complexBox_,inputStyleBox_,groupingBox_,decimalSeparatorBox_,desktopEffectsBox_,continuousEvaluationBox_,mixedFractionsBox_,keyTooltipsBox_,screenTooltipsBox_,fontSizeBox_,percentModeBox_,implicitModeBox_}){QSignalBlocker block(box);for(int i=0;i<box->count();++i)box->setItemText(i,trc(box->itemData(i).toString().toUtf8().constData()));}
 settingsVersion_->setText(trc("Version %1").arg("1.0.0")+" · davoda1");
 registersPanel_->retranslate([this](const char*s){return trc(s);});toolboxTabs_->setTabText(0,trc("Functions"));toolboxTabs_->setTabText(1,trc("Registers"));
 calculator_->retranslate([this](const char*s){return trc(s);});modeMenu_->retranslate([this](const char*s){return trc(s);});
 graph_->retranslate([this](const char*s){return trc(s);});for(auto tool:{catalogTools_,converterTools_})if(tool)tool->retranslate([this](const char*s){return trc(s);});if(currencyPanel_){currencyPanel_->retranslate([this](const char*s){return trc(s);});if(exchangeRates_)currencyPanel_->setRateState(activeRatesSnapshot_,exchangeRates_->refreshing(),trc(exchangeRates_->lastError().toUtf8().constData()));}if(compact_->count()>0){compact_->setTabText(compact_->indexOf(calc_),trc("Calculator"));if(workbenchHub_)compact_->setTabText(compact_->indexOf(workbenchHub_),trc("Workbenches"));}
 QString html="<h1>EMCICalc 1.0.0</h1>";for(auto p:QList<QPair<const char*,const char*>>{{"Calculate","Natural input provides editable fractions, roots, powers, and boxed slots. Linear input uses ordinary syntax; choose it in Settings. Function templates start empty. Arrow keys can leave an empty slot and return to it. Typing at an empty argument fills its slot; another Right leaves the function. Tab visits other slots. Operators stay inside an exponent until Right exits it. At the end of a completed fraction denominator, + or − starts the next term; use parentheses for a compound denominator. Up/Down moves between display lines, Shift+Enter adds a line break, and Enter evaluates. Input style affects entry only. Results always use mathematical typesetting, including in History and the unit converter."},{"Calculator designs","Desktop offers eight palettes, including Porcelain and Aurora, with Flat, Glass, Raised, or Frosted Glass effects. Frosted Glass blurs a cached background inside the app while text stays sharp; no compositor support is required. Calculator retains its eight skins and three layouts, with a 320 × 540 minimum. Desktop uses 680 × 540. Each mode remembers its own appearance and window size. Font size offers System, Small 90%, Medium 100%, and Large 120% without changing operating-system DPI scaling."},{"Exact and decimal","Exact shows simplified fractions, radicals, and symbolic powers. Decimal shows a numerical value. = marks an exact displayed value; ≈ marks an approximation. S⇔D reformats the stored result without evaluating again. Square roots use the principal root; equation solving returns all solutions supplied by the solver. Precision controls significant digits. Ans retains the previous confirmed result; MS, MR, M+, and MC manage memory. Exact fractions in Settings selects Improper or Mixed for rational results, such as 20/3 or 6 2/3. An overline marks the repeating digits of an exact rational expansion: 3 in 0.3 for 1/3, 6 in 0.16 for 1/6, and 09 in 0.09 for 1/11. Complete cycles use =; cycles exceeding the selected precision budget use a rounded value with ≈. Copying a recurring result preserves its exact rational value."},{"Continuous evaluation","Continuous evaluation is off by default. Enable it in Settings to preview complete expressions after a short pause. Previews do not change Ans, registers, or history. Assignments require = or Enter. Explicit evaluation still confirms the result; random functions generate a new value when evaluated again. At startup and after AC the result is blank. With continuous evaluation enabled, empty, incomplete, invalid, or pending input leaves the result blank instead of showing an older answer."},{"Number systems","SHIFT with ×, ÷, +, or − selects BIN, OCT, DEC, or HEX. Changing base reformats the last confirmed value without evaluating, changing registers, or adding History. Each base keeps its own draft. ALPHA A–F adds one 0x prefix to the edited number; ALPHA 5 inserts 0x. ALPHA 4 is labelled b and inserts canonical 0b. Prefixes determine a literal’s base, including fractions: 0xF.A = 15.625 and 0b10.01 = 2.25. Unprefixed 1010 in DEC is decimal. Uppercase A–F are hexadecimal digits; decimal 1E3 and 1E-3 remain scientific notation and lowercase e is Euler’s constant."},{"Registers","SHIFT+. activates REG on the existing keypad. Digits select R0–R9 at the current cursor; the layer stays active until . or SHIFT exits. Escape also cancels it without clearing input. Type R0–R9 directly or paste canonical syntax. Registers persist across restart, modes, and bases; AC leaves them intact. Toolbox → Registers shows values and clears selected registers or all after in-window confirmation. Preview, History recall, and formatting never assign. R0 := rand() stores one confirmed sample."},{"Number notation","Normal, SCI, ENG, FIX, and SI control decimal numeric output. FIX uses the selected number of decimal places; SI applies engineering prefixes. Engineering exponents use multiples of three. Exact fractions and symbolic results remain exact. The number domain selects real or complex calculations, shown as REAL or CPLX on the display. SHIFT 7 shows the destination mode: CPLX in REAL, and REAL in CPLX."},{"Random and comparisons","ALPHA with the decimal key inserts ran# (rand()), sampled on explicit evaluation. rand(n) returns an integer from 1 through n. ALPHA keys provide comparisons; ordinary typed = enters equation text and Enter evaluates. SHIFT with the decimal key selects REG. SHIFT 8 inserts infinity. Genuine signed real infinity is a valid extended-real result. Evaluated undefined results show a transient NaN marker and a specific explanation; syntax errors and unsupported expressions retain their own diagnostics."},{"Angles","RAD, DEG, and GRAD affect trigonometric functions in calculations and graphs. Explicit angle units label a quantity without changing the global angle mode. In Natural input, a unit key wraps the current value or creates an empty quantity slot. Fill the slot, then press Right to leave the quantity. Unit labels remain atomic during editing and copying."},{"Graphing","New graph fields start empty. The Desktop companion workspace and Workbenches → Graphs support up to twenty Cartesian, polar, parametric, implicit, and interactive 3D series with its table view. Saved functions are restored when reopening the app. Clear graphs removes all functions and plotted samples. Reset view restores the coordinate range while keeping your functions. Drag to pan, use the wheel to zoom, and move the pointer to inspect coordinates. Table, Fit Y, PNG, and CSV become available when finite samples exist. Editing a function clears stale results. Discontinuities may require a closer view; plots are numerical samples."},{"Syntax","Display grouping and decimal marks do not change canonical copying. With comma decimals, type ; between function arguments. Pocket provides x, y, and argument comma on ALPHA ×, ÷, and +. Other layouts retain their existing actions. ALPHA ) inserts :=. Assign only to R0–R9 or M: R3 := 125, R3 := R3 + 10, or R6 := Ans. Empty registers report an error; zero is a stored value. An assignment stores the evaluated value, not a live formula."},{"Keyboard","F11 toggles fullscreen. Escape leaves fullscreen first, returns from calculator pages, cancels SHIFT or ALPHA, or clears the expression. Ctrl+G opens graphing, Ctrl+H opens history, Ctrl+, opens settings, Ctrl+L returns to the editor, and F1 opens this guide. In MENU, use arrows and Enter or number keys 1–8."},{"Calculator keyboard","Tap Shift or Alt, or press F2 or F3, to select one-shot SHIFT or ALPHA. Hold Shift or Alt for the matching layer until release. Top-row and numpad digits, and virtual-key clicks, use the held layer. Shift+3 uses the SHIFT action on 3 instead of typing its punctuation. Shift+arrows, Shift+Enter, Alt letter shortcuts, Ctrl shortcuts, and AltGr retain their normal functions. Backspace deletes left; SHIFT Backspace deletes right; ALPHA Backspace undoes. Enter evaluates; ALPHA equals inserts =. F4 cycles angles, F5 exact/decimal, Alt+Up/Down browses history, and Ctrl+Shift+C copies the result. A quick unused tap toggles the same one-shot state as clicking SHIFT or ALPHA. A hold of at least 350 ms, or a used modifier chord of any duration, returns to normal on release, clearing any earlier one-shot state. In Calculator, SHIFT 4/5/6 remain factorial/combinations/permutations in both REAL and CPLX. Mapped calculator combinations consume their normal typing and navigation effects: Shift+numpad 4 uses the action on 4 rather than Left, and Alt+numpad 9 does not insert an Alt-code character. Dedicated Shift+arrow selection and input outside the calculator remain available. Unassigned printable combinations are ignored while a calculator layer is held; release the modifier to resume ordinary typing."},{"Constants and units","CNST opens a searchable catalog of built-in constants and units. Choose an item and insert its symbol into your expression. CONV opens a separate unit converter: enter a value and source and destination units, then convert. It supports compound units and metric prefixes, rejects incompatible dimensions, and leaves calculator Ans unchanged. Successful conversions appear in history. The unit converter has its own saved Exact/Decimal mode and output indicators. S⇔D reformats the stored conversion without calculating again or adding history. Currency converter is a separate Toolbox page; its results always use normal decimal notation, with overlines for repeating digits. It uses dated ECB reference rates, refreshes when opened if needed, and supports manual refresh. The last downloaded rates remain usable offline with their date shown. A first download is required; unavailable currencies are not offered."},{"Structured entry and tools","SHIFT 9, labelled with overlined 123, inserts an editable recurring-digit field. Right leaves the field. ALPHA pi enters degree, minute, and second marks in sequence: 1, DMS, 2, DMS, 3, DMS gives 1°2′3″; 1, DMS, 5 remains 1°5 until the next DMS press. SHIFT MENU opens Statistics. nCr and nPr keep their key labels but use binomial and permutation notation on the screen. Constants are indivisible tokens; function arguments remain editable. Matrix tools choose dimensions and protect cell boundaries. Clear filled cells before shrinking a matrix. Invalid calculations clear the answer; history recall restores its matching result."},{"Toolbox","OTHER opens the function toolbox. Its functions start with empty editable fields, never sample values. Matrix tools use empty cells. Divide inserts an inline division operator. SHIFT Ans creates a fraction template with numerator and denominator fields in Natural input, or ordinary division in Linear input. These are distinct editing actions. Memory controls remain grouped. S⇔D reformats the existing result, while SHIFT equals evaluates the input again as decimal. ALPHA Ans creates a mixed fraction with separate whole-number, numerator, and denominator fields."},{"Storage and privacy","Preferences and per-base input drafts are stored in settings.json. The last 10,000 successful calculations are compressed in history.qz. History searches all entries and displays 100 per page; CSV export includes every retained calculation. Older JSON history migrates automatically. Writes are atomic. Workspace export is versioned and checksummed, with selectable sections and explicit import merge choices. A shared profile lock protects multi-file commits and recovery. If another window changed settings, a settings-conflict file preserves this window’s settings and drafts without overwriting the other window; history merges independently. No accounts or telemetry are used. Currency conversion downloads public reference rates into exchange-rates.zip in the same configuration directory. Calculation inputs are not sent."},{"About","Original EMCICalc code and artwork: MIT, davoda1. Uses Qt and libqalculate under their respective licenses. Distribution of EMCICalc with libqalculate must comply with GPLv3. Each dependency retains its own license terms. See the included notices and source information. Independent software, not affiliated with Casio or HiPER."}}){html+="<h2>"+trc(p.first).toHtmlEscaped()+"</h2><p>"+trc(p.second).toHtmlEscaped()+"</p>";}html+="<h2>"+trc("Workbenches").toHtmlEscaped()+"</h2><p>"+trc("Details retains a frozen result context. Inspection, graph sampling, Steps, and previews never commit Ans or registers. Explicit Recalculate is a separate action. Guided Algebra and Calculus, Matrix, Data and Statistics, Finance, Definitions, Expression/Classic/RPN input, Programmer, and custom keypad tools are available from Workbenches. Steps reports only its supported offline families; unsupported cases are labeled honestly.").toHtmlEscaped()+"</p>";guide_->setHtml(html);refreshApproximation();if(pages_->currentIndex()==1)fillHistory();updateSettingsUi();setStatus(trc("Ready"));if(layoutReady_)updateAuxiliaryLayouts();
}
void MainWindow::showPage(int i,bool remember){
 if(i!=0){calculator_->setRegisterLayer(false);shift_=alpha_=false;refreshKeys();}
 if(i==5&&workbenchHub_){showGraph();return;}if(i<0||i>=pages_->count())return;
 const int previous=pages_->currentIndex();
 if(previous==3&&i!=3&&settingsDraftActive_)discardSettingsDraft();
 if(i==3&&previous!=3&&!handheld_){settingsReturnPage_=previous;beginSettingsDraft();}
 if(handheld_&&remember&&previous!=i&&i!=0)pageBackStack_.append(previous);if(i==0)pageBackStack_.clear();
 {QSignalBlocker block(navigation_);navigation_->setCurrentIndex(i==10?5:i<5&&i!=3?i:-1);}pages_->setCurrentIndex(i);if(layoutReady_)updateLayout();
 if(i==2&&toolboxTabs_->currentIndex()==1)refreshRegisters();
 auxHeader_->setVisible(handheld_&&i!=0);status_->setVisible(!handheld_||i!=0);updateAuxiliaryLayouts();
 // Initial Qt stylesheet polish can restore a hidden page's prior palette.
 // Repair only the page just shown; unchanged scoped palettes are a no-op.
 if(auto *current=pages_->currentWidget();current->property("twScope").toBool())DesktopWorkbench::apply(current,qApp->palette());
 if(i==10)DesktopFont::apply(workbenchHub_);
 if(i==1)fillHistory();
 if(i==9&&exchangeRates_&&!testMode_)exchangeRates_->refresh(false);
 if(i==6&&!modeMenu_->cardButtons().isEmpty())modeMenu_->cardButtons().first()->setFocus();
}
void MainWindow::goBack(){if(pages_->currentIndex()==10&&workbenchHub_&&workbenchHub_->goBack())return;if(settingsDraftActive_&&pages_->currentIndex()==3){const int next=settingsReturnPage_;showPage(next,false);if(next==0)input_->setFocus();return;}int next=pageBackStack_.isEmpty()?0:pageBackStack_.takeLast();showPage(next,false);if(next==0)input_->setFocus();}
void MainWindow::showGraph(){
 if(workbenchHub_){showWorkspace("graphs");return;}
 if(handheld_)showPage(5);else{showPage(0);if(narrow_)compact_->setCurrentWidget(graph_);}graph_->setFocus();
}
void MainWindow::updateAuxiliaryLayouts(){
 if(!pages_)return;const int current=pages_->currentIndex();const char* titles[]={"Calculator","History","Toolbox","Settings","Guide","Graph","Calculator menu","Constants and units","Unit converter","Currency converter"};
 // The caption grows with the user's font preference; fixed artwork buttons
 // can retain their dimensions, but MENU needs its full styled text extent.
 const QSize menuHint=auxMenu_->sizeHint();const QSize menuSize(qMax(52,menuHint.width()),qMax(32,menuHint.height()));
 if(auxMenu_->size()!=menuSize)auxMenu_->setFixedSize(menuSize);
 if(current==10&&workbenchHub_){auxTitle_->setText(workbenchHub_->currentTitle());auxTitle_->setToolTip(auxTitle_->text());}
 const char* pageTitle=current==7&&catalogTools_->view()==CalculatorTools::View::Statistics?"Statistics":current==7&&catalogTools_->view()==CalculatorTools::View::Matrix?"Matrix":titles[qBound(0,current,9)];if(current!=10)auxTitle_->setText(trc(pageTitle));auxTitle_->setToolTip(auxTitle_->text());auxGear_->setEnabled(current!=3);auxMenu_->setEnabled(current!=6);
 for(auto tool:{catalogTools_,converterTools_})if(tool)tool->setHeaderVisible(!handheld_);if(currencyPanel_)currencyPanel_->setHeaderVisible(!handheld_);
 updateSettingsUi();
 if(toolboxGrid_){const int columns=width()<520?1:2;if(toolboxGrid_->property("activeColumns").toInt()!=columns){toolboxGrid_->setProperty("activeColumns",columns);for(int i=0;i<toolboxButtons_.size();++i)toolboxGrid_->addWidget(toolboxButtons_[i],i/columns,i%columns);for(int r=0;r<toolboxButtons_.size()+1;++r)toolboxGrid_->setRowStretch(r,0);toolboxGrid_->setRowStretch((toolboxButtons_.size()+columns-1)/columns,1);}}
 if(historyControls_&&historyControls_->property("compactArrangement").toInt()!=(width()<560?1:2)){historyControls_->setProperty("compactArrangement",width()<560?1:2);if(width()<560){historyControls_->addWidget(historySearch_,0,0,1,2);historyControls_->addWidget(historyExportButton_,1,0);historyControls_->addWidget(historyClearButton_,1,1);historyControls_->setColumnStretch(0,1);historyControls_->setColumnStretch(1,1);historyControls_->setColumnStretch(2,0);}else{historyControls_->addWidget(historySearch_,0,0);historyControls_->addWidget(historyExportButton_,0,1);historyControls_->addWidget(historyClearButton_,0,2);historyControls_->setColumnStretch(0,1);historyControls_->setColumnStretch(1,0);}}
}
void MainWindow::updateLayout(){
 if(!layoutReady_||rearranging_||!workbenchHub_)return;rearranging_=true;setMinimumSize(handheld_?320:680,540);
 desktopHeader_->setVisible(!handheld_);navigation_->setVisible(!handheld_);status_->setVisible(!handheld_||pages_->currentIndex()!=0);auxHeader_->setVisible(handheld_&&pages_->currentIndex()!=0);
 outer_->setContentsMargins(handheld_?0:10,handheld_?0:8,handheld_?0:10,handheld_?0:6);outer_->setSpacing(handheld_?0:6);
 const bool small=!handheld_&&(layout_=="Tabbed"||width()<1000),fullWorkspace=handheld_||pages_->currentIndex()==10;
 const QString baseArrangement=handheld_?"handheld":small?"tabs":layout_=="Calculator right"?"right":"left";
 const QString arrangement=baseArrangement+(fullWorkspace?"-full":"-split");
 const bool shortSplit=!handheld_&&!small&&height()<620;const int calculatorMaximum=shortSplit?650:530;const int calculatorPreferred=shortSplit?qMin(600,width()-440):420;
 if(arrangement_!=arrangement){
  QPointer<QWidget> focused=QApplication::focusWidget();
  const bool toolsSelected=(arrangement_.startsWith("tabs")&&compact_->currentWidget()==workbenchHub_)||(focused&&(focused==workbenchHub_||workbenchHub_->isAncestorOf(focused)));
  calc_->hide();workbenchHub_->hide();while(compact_->count())compact_->removeTab(0);
  calc_->setParent(nullptr);workbenchHub_->setParent(nullptr);split_->hide();compact_->hide();handheldHost_->hide();calc_->setMaximumWidth(QWIDGETSIZE_MAX);
  if(handheld_){handheldLayout_->addWidget(calc_);calc_->show();handheldHost_->show();}
  else if(small){compact_->addTab(calc_,trc("Calculator"));if(!fullWorkspace)compact_->addTab(workbenchHub_,trc("Workbenches"));compact_->setCurrentWidget(toolsSelected&&!fullWorkspace?static_cast<QWidget*>(workbenchHub_):calc_);compact_->show();}
  else{
   if(baseArrangement=="right"){if(!fullWorkspace)split_->addWidget(workbenchHub_);split_->addWidget(calc_);}else{split_->addWidget(calc_);if(!fullWorkspace)split_->addWidget(workbenchHub_);}
   calc_->setMaximumWidth(calculatorMaximum);calc_->show();if(!fullWorkspace)workbenchHub_->show();split_->show();
   if(!fullWorkspace)split_->setSizes(baseArrangement=="right"?QList<int>{width()-calculatorPreferred-20,calculatorPreferred}:QList<int>{calculatorPreferred,width()-calculatorPreferred-20});
  }
  if(fullWorkspace){workspaceHostLayout_->addWidget(workbenchHub_);workbenchHub_->show();}
  arrangement_=arrangement;if(focused&&focused->isVisible())focused->setFocus(Qt::OtherFocusReason);
 }
 if(!handheld_&&!small&&calc_->maximumWidth()!=calculatorMaximum){calc_->setMaximumWidth(calculatorMaximum);if(!fullWorkspace)split_->setSizes(baseArrangement=="right"?QList<int>{width()-calculatorPreferred-20,calculatorPreferred}:QList<int>{calculatorPreferred,width()-calculatorPreferred-20});}
 graph_->hide();narrow_=small;brandSub_->setVisible(width()>840);rearranging_=false;updateAuxiliaryLayouts();
}
void MainWindow::rememberWindowGeometry(){if(!layoutReady_||isFullScreen()||isMaximized())return;if(handheld_)calculatorGeometry_=saveGeometry();else desktopGeometry_=saveGeometry();}
void MainWindow::resizeEvent(QResizeEvent*e){QMainWindow::resizeEvent(e);updateLayout();if(saveTimer_&&layoutReady_)saveTimer_->start();}
void MainWindow::closeEvent(QCloseEvent*e){
 if(!saveSettings()){
  e->ignore();
  // A busy archive retains unsaved rows; do not discard them by closing.
  if(historyArchive_.busy())QTimer::singleShot(1000,this,[this]{close();});
  return;
 }
 closing_=true;
 QMainWindow::closeEvent(e);
}
void MainWindow::setStatus(QString s){if(baseResultDetached_&&!committedResult_.isEmpty()){const auto marker=trc("Last confirmed result · %1").arg(numberBase_==2?"BIN":numberBase_==8?"OCT":numberBase_==16?"HEX":"DEC");if(!s.startsWith(marker))s=marker+" · "+s;}if(status_){status_->setText(fontMetrics().elidedText(s,Qt::ElideRight,qMax(200,width()-30)));status_->setToolTip(s);}if(calculator_)calculator_->setStatus(s);}
void MainWindow::watchSystemTheme(){
 systemDark_=QApplication::palette().color(QPalette::Window).lightness()<128;
#if QT_VERSION >= QT_VERSION_CHECK(6,5,0)
 connect(QGuiApplication::styleHints(),&QStyleHints::colorSchemeChanged,this,[this]{if(appearance_=="System")applyTheme();});
#else
#ifdef Q_OS_LINUX
 if(testMode_)return;
 auto detectKde=[this]{QSettings kde(QDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)).filePath("kdeglobals"),QSettings::IniFormat);const auto color=kde.value("Colors:Window/BackgroundNormal").toStringList();if(color.size()==3){QColor c(color[0].toInt(),color[1].toInt(),color[2].toInt());systemDark_=c.lightness()<128;}};
 detectKde();
 const auto executable=QStandardPaths::findExecutable("gsettings");
 if(!executable.isEmpty()){
  QProcess query;query.start(executable,{"get","org.gnome.desktop.interface","color-scheme"});if(query.waitForFinished(200)&&query.exitCode()==0){auto answer=query.readAllStandardOutput();if(answer.contains("dark"))systemDark_=true;else if(answer.contains("light"))systemDark_=false;}else{query.kill();query.waitForFinished(100);}
  systemThemeProcess_=new QProcess(this);connect(systemThemeProcess_,&QProcess::readyReadStandardOutput,this,[this]{auto line=systemThemeProcess_->readAllStandardOutput();if(line.contains("dark"))systemDark_=true;else if(line.contains("light")||line.contains("default"))systemDark_=false;if(appearance_=="System")applyTheme();});systemThemeProcess_->start(executable,{"monitor","org.gnome.desktop.interface","color-scheme"});
 }
 auto watcher=new QFileSystemWatcher(this);auto path=QDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)).filePath("kdeglobals");if(QFile::exists(path))watcher->addPath(path);connect(watcher,&QFileSystemWatcher::fileChanged,this,[this,watcher,path,detectKde]{detectKde();if(QFile::exists(path)&&!watcher->files().contains(path))watcher->addPath(path);if(appearance_=="System")applyTheme();});
#endif
#endif
}
void MainWindow::applyTheme(){
 if(registersPanel_)registersPanel_->setCalculatorMode(handheld_);
 bool dark=appearance_=="Dark";
 if(appearance_=="System"){
#if QT_VERSION >= QT_VERSION_CHECK(6,5,0)
 dark=QGuiApplication::styleHints()->colorScheme()==Qt::ColorScheme::Dark;
#else
 dark=systemDark_;
#endif
 }
 QPalette p=desktopThemePalette(theme_,dark);QColor bg=p.color(QPalette::Window),fg=p.color(QPalette::WindowText),base=p.color(QPalette::Base),button=p.color(QPalette::Button),accent=p.color(QPalette::Highlight);if(handheld_)p=calculatorThemePalette(calculatorTheme_);if(qApp->palette()!=p)qApp->setPalette(p);
 QString styleSheet=QString("QWidget{font-family:'Segoe UI','Noto Sans',sans-serif;} QLabel#brand{font-size:19pt;font-weight:700;} QLineEdit,QComboBox,QSpinBox{padding:4px;border:1px solid %1;border-radius:5px;} QLineEdit:focus,QComboBox:focus,QSpinBox:focus{border:1px solid %2;} QPushButton{padding:5px 9px;border:1px solid %1;border-radius:5px;background:%3;} QPushButton#settingsGear,QPushButton#calculatorBack{padding:3px;} QPushButton:hover{border-color:%2;} QPushButton:focus{border:2px solid %2;} QTabBar::tab{padding:5px 12px;background:transparent;border:0;border-bottom:2px solid transparent;} QTabBar::tab:selected{color:%2;border-bottom:2px solid %2;} QLabel#muted{color:%4;font-size:9pt;} QHeaderView::section{padding:5px;background:%3;border:0;} QToolTip{color:%6;background:%5;border:1px solid %1;} ").arg(p.color(QPalette::Mid).name(),accent.name(),button.name(),dark?"#b1bdca":"#526371",base.name(),fg.name());
 if(handheld_)styleSheet=calculatorThemeStyleSheet(calculatorTheme_);
 else styleSheet+=desktopEffectsStyleSheet(p,desktopEffects_);
 styleSheet+=QString("QPushButton:checked{background:%1;color:%2;} QPushButton:disabled{background:%3;color:%4;border-color:%5;} QLineEdit:disabled,QComboBox:disabled,QSpinBox:disabled{color:%6;}").arg(p.color(QPalette::Highlight).name(),p.color(QPalette::HighlightedText).name(),p.color(QPalette::Window).name(),p.color(QPalette::Disabled,QPalette::ButtonText).name(),p.color(QPalette::Window).name(),p.color(QPalette::Disabled,QPalette::Text).name());
 // Use the active mode's palette for readable, padded tooltip cards.
 const auto tipTheme=calculatorThemeDefinition(calculatorTheme_);
 const QColor tipBase=handheld_?tipTheme.body:p.color(QPalette::ToolTipBase);
 const QColor tipText=handheld_?tipTheme.bodyText:p.color(QPalette::ToolTipText);
 QPalette tipPalette=p;tipPalette.setColor(QPalette::ToolTipBase,tipBase);tipPalette.setColor(QPalette::ToolTipText,tipText);QToolTip::setPalette(tipPalette);
 QFont tipFont=DesktopFont::baseFont();tipFont.setPointSizeF(DesktopFont::scale(10));QToolTip::setFont(tipFont);
 styleSheet+=QString("QToolTip{background:%1;color:%2;border:1px solid %3;border-radius:7px;padding:8px 10px;font-size:%4pt;opacity:255;}").arg(tipBase.name(),tipText.name(),p.color(QPalette::Mid).name()).arg(DesktopFont::scale(10));
 // The OS/Qt font and DesktopFont own text sizing. Reapplying a global
 // font-family rule on every palette/effect change needlessly resets fonts
 // throughout hidden workbenches and triggers their layout callbacks.
 styleSheet.remove(QStringLiteral("QWidget{font-family:'Segoe UI','Noto Sans',sans-serif;} "));
 if(qApp->styleSheet()!=styleSheet){qApp->setStyleSheet(styleSheet);setProperty("themeStyleInstalls",property("themeStyleInstalls").toInt()+1);}
 if(workbenchHub_){workbenchHub_->setCalculatorMode(handheld_);DesktopWorkbench::apply(workbenchHub_,p);}updateSettingsPresentation();
 for(auto *tool:{catalogTools_,converterTools_})if(tool)DesktopWorkbench::apply(tool,p);
 if(desktopSurface_){desktopSurface_->setCalculatorMode(handheld_);desktopSurface_->setAppearance(p,desktopEffects_);}calculator_->setDesktopEffects(desktopEffects_);
 if(historyTable_)historyTable_->setPalette(p);
 gear_->setIcon(emciGearIcon(p.color(QPalette::WindowText)));auxGear_->setIcon(emciGearIcon(p.color(QPalette::WindowText)));back_->setIcon(emciBackIcon(p.color(QPalette::WindowText)));if(settingsBack_)settingsBack_->setIcon(emciBackIcon(p.color(QPalette::WindowText)));calculator_->setGearIcon(emciGearIcon(handheld_?calculatorThemeDefinition(calculatorTheme_).text:p.color(QPalette::WindowText)));modeMenu_->setGearIcon(emciGearIcon(p.color(QPalette::WindowText)));if(graph_){graph_->setPalette(p);graph_->update();}refreshKeys();if(pages_->currentIndex()==1)fillHistory();
}
void MainWindow::loadSettings(){
 QString root;
 if(testMode_){root=QDir::tempPath()+"/EMCICalc-smoke-"+QUuid::createUuid().toString(QUuid::WithoutBraces);auto args=QCoreApplication::arguments();int index=args.indexOf("--config-root");if(index>=0&&index+1<args.size())root=args[index+1];}
 else{
#ifdef Q_OS_WIN
 root=qEnvironmentVariable("APPDATA");root=root.isEmpty()?QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation):QDir(root).filePath("davoda1/EMCICalc");
#else
 root=QDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)).filePath("davoda1/EMCICalc");
#endif
 }
 QDir().mkpath(root);configPath_=QDir(root).filePath("settings.json");
 {auto transaction=lockProfile();if(!transaction){configSaveBlocked_=true;if(restoreNotice_.isEmpty())restoreNotice_="Another window is saving this profile. Reopen this window after it finishes.";}}
 configPath_=QDir(root).filePath("settings.json");lastDir_=QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);if(!QDir(lastDir_).exists())lastDir_=QDir::homePath();QFile file(configPath_);if(!file.exists())return;if(!file.open(QIODevice::ReadOnly)){restoreNotice_="Settings could not be read; defaults restored";configSaveBlocked_=true;return;}if(file.size()>64LL*1024*1024){file.close();configSaveBlocked_=!QFile::copy(configPath_,configPath_+".invalid-"+QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmss-zzz"));restoreNotice_="Invalid settings preserved; defaults restored";return;}QJsonParseError error;auto doc=QJsonDocument::fromJson(file.readAll(),&error);file.close();if(error.error!=QJsonParseError::NoError||!doc.isObject()||doc.object()["schema"].toInt()!=1||doc.object().value("profileFormat").toInt(1)!=1){configSaveBlocked_=!QFile::copy(configPath_,configPath_+".invalid-"+QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmss-zzz"));restoreNotice_="Invalid settings preserved; defaults restored";return;}settingsFingerprint_=QCryptographicHash::hash(doc.toJson(QJsonDocument::Compact),QCryptographicHash::Sha256);const auto o=doc.object();loadedSettings_=o;language_=o["language"].toString("en");theme_=o["theme"].toString("Graphite");appearance_=o["appearance"].toString("System");layout_=o["layout"].toString("Adaptive");design_=o["mode"].toString(o["design"].toString("Desktop"));calculatorTheme_=o["calculatorTheme"].toString(design_=="Modern LCD"?"Modern Light":"Classic");if(design_=="Classic LCD"||design_=="Modern LCD")design_="Calculator";if(design_!="Calculator")design_="Desktop";if(!calculatorThemeNames().contains(calculatorTheme_))calculatorTheme_="Classic";calculatorLayout_=o["calculatorLayout"].toString("Compact");if(!QStringList{"Pocket","Compact","Expanded"}.contains(calculatorLayout_))calculatorLayout_="Compact";numberBase_=o["numberBase"].toInt(10);if(!QList<int>{2,8,10,16}.contains(numberBase_))numberBase_=10;notation_=qBound(0,o["notation"].toInt(0),4);complexEnabled_=o["complexEnabled"].toBool(true);structuredInput_=o["structuredInput"].toBool(true);continuousEvaluation_=o["continuousEvaluation"].toBool(false);keyTooltipsEnabled_=o["keyTooltipsEnabled"].toBool(true);screenTooltipsEnabled_=o["screenTooltipsEnabled"].toBool(true);mixedFractions_=o["mixedFractions"].toBool(false);digitGrouping_=qBound(0,o["digitGrouping"].toInt(0),2);decimalSeparator_=o["decimalSeparator"].toString(".")==","?',':'.';if(decimalSeparator_==','&&digitGrouping_==2)digitGrouping_=1;desktopEffects_=qBound(0,o["desktopEffects"].toInt(1),3);const auto drafts=o["inputDrafts"].toObject();for(int b:{2,8,10,16})baseDrafts_[b]=drafts[QString::number(b)].toString();precision_=qBound(10,o["precision"].toInt(30),200);angle_=qBound(0,o["angle"].toInt(0),2);exact_=o["exact"].toBool(true);converterExact_=o["converterExact"].toBool(true);memoryValue_=o["memory"].toString();memoryValueApproximate_=o["memoryApproximate"].toBool(false);history_=o["history"].toArray();while(history_.size()>HistoryArchive::Limit)history_.removeLast();savedGeometry_=QByteArray::fromBase64(o["geometry"].toString().toLatin1());savedMaximized_=o["maximized"].toBool(false);desktopGeometry_=QByteArray::fromBase64(o["desktopGeometry"].toString().toLatin1());calculatorGeometry_=QByteArray::fromBase64(o["calculatorGeometry"].toString().toLatin1());if(desktopGeometry_.isEmpty())desktopGeometry_=savedGeometry_;if(calculatorGeometry_.isEmpty()&&design_=="Calculator")calculatorGeometry_=savedGeometry_;auto d=o["lastDirectory"].toString();if(!d.isEmpty()&&QDir(d).exists())lastDir_=d;
}
void MainWindow::restoreGeometrySafe(){auto geometry=handheld_?calculatorGeometry_:desktopGeometry_;if(!geometry.isEmpty())restoreGeometry(geometry);else resize(handheld_?QSize(380,720):QSize(1180,760));bool visible=false;for(auto screen:QGuiApplication::screens())if(screen->availableGeometry().intersects(frameGeometry()))visible=true;if(!visible)move(QGuiApplication::primaryScreen()->availableGeometry().topLeft()+QPoint(40,40));if(savedMaximized_)showMaximized();if(!loadedSettings_.isEmpty()){auto o=loadedSettings_;if(o["graphState"].isObject())graph_->restoreState(o["graphState"].toObject());else{QStringList expressions;for(auto e:o["graphs"].toArray())expressions<<e.toString();if(!expressions.isEmpty())graph_->setExpressions(expressions);}}if(!memoryValue_.isEmpty()){auto m=memoryValue_;const bool approximate=memoryValueApproximate_;dispatch([m,approximate](CalcEngine*e){e->setVariable("M",m,approximate);});}}
bool MainWindow::saveSettings(){
 auto transaction=lockProfile();if(!transaction)return false;
 if(!graph_)return false;rememberWindowGeometry();if(configSaveBlocked_){setStatus(trc("Could not save settings"));return false;}
 QDir().mkpath(QFileInfo(configPath_).absolutePath());
 if(!historyArchive_.save()){setStatus(trc("Could not save history")+": "+historyArchive_.error());if(historyArchive_.busy()&&saveTimer_)saveTimer_->start(1000);return false;}
 if(history_!=historyArchive_.entries()){history_=historyArchive_.entries();fillHistory();}
 baseDrafts_[numberBase_]=input_->text();QJsonObject drafts;for(int base:{2,8,10,16})drafts[QString::number(base)]=baseDrafts_.value(base);
 QJsonArray graphs;for(auto e:graph_->expressions())graphs.append(e);
 QJsonObject o{{"schema",1},{"version","1.0.0"},{"profileFormat",1},{"mode",design_},{"design",design_},{"calculatorTheme",calculatorTheme_},{"calculatorLayout",calculatorLayout_},{"theme",theme_},{"appearance",appearance_},{"layout",layout_},{"language",language_},{"precision",precision_},{"angle",angle_},{"exact",exact_},{"converterExact",converterExact_},{"numberBase",numberBase_},{"notation",notation_},{"complexEnabled",complexEnabled_},{"structuredInput",structuredInput_},{"continuousEvaluation",continuousEvaluation_},{"keyTooltipsEnabled",keyTooltipsEnabled_},{"screenTooltipsEnabled",screenTooltipsEnabled_},{"mixedFractions",mixedFractions_},{"digitGrouping",digitGrouping_},{"decimalSeparator",QString(decimalSeparator_)},{"desktopEffects",desktopEffects_},{"inputDrafts",drafts},{"memory",memoryValue_},{"memoryApproximate",memoryValueApproximate_},{"lastDirectory",lastDir_},{"graphs",graphs},{"graphState",graph_->saveState()},{"desktopGeometry",QString(desktopGeometry_.toBase64())},{"calculatorGeometry",QString(calculatorGeometry_.toBase64())},{"geometry",QString(saveGeometry().toBase64())},{"maximized",isMaximized()}};
 o.insert("fontSizeMode",fontSizeMode_);o.insert("financeDraftV2",catalogTools_->financePanel()->saveState());o.insert("answerV2",QJsonObject{{"canonical",answerState_.canonical},{"approximate",answerState_.approximate}});
 if(workbenchHub_){o.insert("catalogBrowserV2",catalogTools_->saveBrowserState());o.insert("workspaceV2",workbenchHub_->saveState());o.insert("menuLayoutV2",workbenchHub_->menuLayout());o.insert("customKeypadAppliedV2",calculator_->hasCustomLayout()?CustomKeypad::toJson(calculator_->customLayout()):QJsonObject{});QJsonArray defs;for(const auto &d:definitions_)defs.append(WorkspaceModel::definitionToJson(d));o.insert("definitionsV2",defs);o.insert("selectedResultV2",WorkspaceModel::resultToJson(selectedResult_));o.insert("decimalPlacesV2",decimalPlaces_);o.insert("percentModeV2",percentMode_);o.insert("implicitModeV2",implicitMode_);}
 const auto bytes=QJsonDocument(o).toJson();if(bytes.size()>64LL*1024*1024){setStatus(trc("Could not save settings")+": "+trc("Workspace exceeds the 64 MiB saved-state limit"));return false;}
 QByteArray diskFingerprint;
 {
  // Windows QFile readers do not share deletion. Release this handle before
  // QSaveFile atomically replaces the target, including on repeated autosave.
  QFile current(configPath_);if(current.exists()){if(!current.open(QIODevice::ReadOnly)){setStatus(trc("Could not save settings")+": "+current.errorString());return false;}const auto currentBytes=current.readAll();if(current.error()!=QFileDevice::NoError){setStatus(trc("Could not save settings")+": "+current.errorString());return false;}const auto currentDoc=QJsonDocument::fromJson(currentBytes);diskFingerprint=QCryptographicHash::hash(currentDoc.toJson(QJsonDocument::Compact),QCryptographicHash::Sha256);}
 }
 const bool conflict=diskFingerprint!=settingsFingerprint_;QString path=configPath_;
 if(conflict){if(settingsConflictPath_.isEmpty())settingsConflictPath_=QFileInfo(configPath_).absolutePath()+"/settings-conflict-"+QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmss-zzz")+"-"+QString::number(QCoreApplication::applicationPid())+".json";path=settingsConflictPath_;}
 QSaveFile f(path);f.setDirectWriteFallback(false);if(!f.open(QIODevice::WriteOnly)){setStatus(trc("Could not save settings")+": "+f.errorString());return false;}if(f.write(bytes)!=bytes.size()){const auto error=f.errorString();f.cancelWriting();setStatus(trc("Could not save settings")+": "+error);return false;}const bool ok=f.commit();if(!ok)setStatus(trc("Could not save settings")+": "+f.errorString());else if(conflict)setStatus(trc("Another window changed settings; this window was preserved in %1").arg(QDir::toNativeSeparators(path)));else settingsFingerprint_=QCryptographicHash::hash(QJsonDocument(o).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256);return ok;
}
bool MainWindow::smokeTest(const QString& output){
 bool ok=true;int checks=0;auto check=[&](bool pass,const char*what){++checks;qInfo().noquote()<<(pass?"PASS":"FAIL")<<what;ok&=pass;};
 auto wait=[&](std::function<bool()>done,int ms=7000){QElapsedTimer t;t.start();while(!done()&&t.elapsed()<ms){QApplication::processEvents(QEventLoop::AllEvents,20);QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);QThread::msleep(4);}return done();};
 auto idle=[&]{return wait([&]{return !busy_&&!formatPending_;});};wait([&]{return !graphRunning_;},10000);
 check(windowTitle()=="EMCICalc"&&!guide_->toPlainText().contains("Revision"),"application title is only EMCICalc and guide omits the packaging revision");
 check(wait([&]{return catalogReady_;})&&!unitTokens_.isEmpty(),"built-in catalogs load asynchronously without calculator examples");
 if(loadedSettings_.isEmpty())check(result_->text().isEmpty()&&result_->resultRelation().isEmpty()&&approx_->text().isEmpty(),"startup has no result or equality marker");
 if(loadedSettings_.isEmpty())check(!continuousEvaluation_&&continuousEvaluationBox_->currentIndex()==0,"continuous evaluation defaults to Off");
 if(loadedSettings_.isEmpty())check(workbenchHub_&&workbenchHub_->graphs->saveState().value("series").toArray().isEmpty()&&input_->text().isEmpty(),"clean startup has empty calculator and graph expressions");
 check(converterTools_->amountEditor()->text().isEmpty()&&converterTools_->sourceUnitEditor()->currentText().isEmpty()&&converterTools_->destinationUnitEditor()->currentText().isEmpty()&&converterTools_->resultEditor()->text().isEmpty(),"converter starts without invented values or units");

 inputStyleBox_->setCurrentIndex(0);groupingBox_->setCurrentIndex(0);decimalSeparatorBox_->setCurrentIndex(0);desktopEffectsBox_->setCurrentIndex(1);applyMathPreferences();
 baseBox_->setCurrentIndex(baseBox_->findData(10));idle();notationBox_->setCurrentIndex(0);idle();exactBox_->setCurrentIndex(0);idle();complexEnabled_=true;calculator_->setComplexEnabled(true);
 changeBase(16);check(idle()&&result_->text().isEmpty()&&result_->resultRelation().isEmpty()&&input_->numberBase()==16&&result_->numberBase()==16,"base change before first calculation leaves the result display blank");changeBase(10);idle();
 input_->setText("1/3+1/6");evaluate();check(idle()&&lastSerialized_.contains("1/2"),"UI exact fraction evaluation");
 int count=history_.size();handleAction("@format");check(idle()&&result_->text()=="0.5"&&lastSerialized_.contains("1/2"),"format converts canonical fraction without reevaluation");handleAction("@format");check(idle()&&history_.size()==count&&input_->text()=="1/3+1/6","format preserves expression and history");
 // Paired reference regressions: exact symbols, decimal truth, and distinct division entry.
 for(const QString& mode:QStringList{"Desktop","Calculator"}){
  design_=mode;applyDesign();
  input_->setText("6/5");evaluate();check(idle()&&approx_->text()=="= 1.2"&&result_->resultRelation()=="=","terminating decimal is labelled equal in each main mode");
  check(history_.first().toObject().contains("approximateApproximate")&&!history_.first().toObject()["approximateApproximate"].toBool(),"history preserves exactness metadata");
  groupingBox_->setCurrentIndex(1);decimalSeparatorBox_->setCurrentIndex(1);check(approx_->text()=="= 1,2","number formatting preserves exact equality marker");groupingBox_->setCurrentIndex(0);decimalSeparatorBox_->setCurrentIndex(0);
  input_->setText("1/3");evaluate();check(idle()&&approx_->text()=="= 0.3"&&approx_->property("repeatLength").toInt()==1&&result_->resultRelation()=="=","recurring decimal uses an exact overline while the primary fraction stays exact");
  handleAction("@format");check(idle()&&result_->resultRelation()=="="&&!result_->recurringBarRects().isEmpty(),"Decimal represents a recurring rational exactly with an overline");handleAction("@format");check(idle()&&result_->resultRelation()=="=","S to D restores the exact fraction marker");
  exactBox_->setCurrentIndex(1);idle();input_->setText("sqrt(2)");evaluate();check(idle()&&result_->resultRelation()=="≈","Decimal-first radical result is marked approximate");const int radicalHistory=history_.size();handleAction("@format");check(idle()&&lastSerialized_.contains("sqrt")&&result_->text().contains("sqrt")&&result_->resultRelation()=="="&&history_.size()==radicalHistory,"Decimal-first evaluation preserves exact radical for S to D without another history entry");
  input_->clear();handleAction("6");handleAction("/");handleAction("5");check(input_->text().contains(QChar(0x00f7))&&input_->canonicalText()=="6/5","ordinary division remains inline with canonical arithmetic");evaluate();check(idle()&&approx_->text()=="= 1.2","ordinary division evaluates to exact one point two");
  input_->clear();handleAction("6");handleAction("@fraction");handleAction("5");check(input_->text().contains("/(")&&!input_->text().contains(QChar(0x00f7)),"fraction key creates a different editable input structure");evaluate();check(idle()&&approx_->text()=="= 1.2","fraction template and division retain equivalent arithmetic");
  input_->clear();handleAction("pi");handleAction("pi");handleAction("pi");evaluate();check(idle()&&QString(lastSerialized_).remove(' ')=="pi^3"&&result_->resultRelation()=="=","three pi keys produce an exact symbolic cube");
  input_->setText("56*sqrt(2)/99");evaluate();check(idle()&&lastSerialized_.contains("sqrt")&&lastSerialized_.contains("99")&&result_->resultRelation()=="="&&approx_->text().startsWith("≈ "),"rational radical keeps exact symbolic result with approximate secondary");
  input_->clear();handleAction("@mixedFraction");check(input_->slotRects().size()==3,"mixed fraction starts with three empty fields");handleAction("6");input_->focusNextSlot();handleAction("2");input_->focusNextSlot();handleAction("3");evaluate();check(idle()&&QString(lastSerialized_).remove(' ').contains("20/3"),"mixed whole numerator denominator input evaluates to twenty thirds");
  const int mixedHistory=history_.size();mixedFractionsBox_->setCurrentIndex(1);check(idle()&&result_->text().contains("mixed(")&&history_.size()==mixedHistory,"mixed result preference reformats the stored value without another evaluation");
  handleAction("@format");check(idle()&&result_->resultRelation()=="="&&!result_->recurringBarRects().isEmpty(),"mixed fraction toggles to an exact recurring Decimal");handleAction("@format");check(idle()&&result_->text().contains("mixed("),"S to D restores selected mixed Exact form");mixedFractionsBox_->setCurrentIndex(0);check(idle()&&!result_->text().contains("mixed("),"improper preference restores the same exact rational");
  input_->setText("mixed(-6,2,3)");evaluate();check(idle()&&QString(lastSerialized_).replace(QChar(0x2212),'-').remove(' ').contains("-20/3"),"negative mixed entry applies minus to the whole mixed value");
 }
 design_="Desktop";applyDesign();
 input_->setText("6/5");handleAction("@approx");check(idle()&&result_->text()=="1.2"&&lastSerialized_.contains("6/5")&&history_.first().toObject()["expression"]=="6/5","approximate key evaluates current input after changing result mode");exactBox_->setCurrentIndex(0);idle();check(result_->resultRelation()=="="&&result_->text().contains("6"),"Desktop Exact control reformats committed value with continuous evaluation off");
 input_->setText("1/3");evaluate();idle();exactBox_->setCurrentIndex(1);idle();precisionBox_->setValue(10);check(idle()&&result_->text()=="0.3"&&result_->resultRelation()=="="&&!result_->recurringBarRects().isEmpty(),"precision change preserves a complete exact repetend");precisionBox_->setValue(30);idle();exactBox_->setCurrentIndex(0);idle();
 input_->setText("2+3");evaluate();check(idle()&&result_->text()=="5","UI expression evaluation");
 const QString beforeTools=input_->text();handleAction("@constants");check(pages_->currentIndex()==7&&catalogTools_->view()==CalculatorTools::View::Constants&&catalogTools_->catalogList()->count()>0,"CNST opens real constant catalog");
 catalogTools_->setView(CalculatorTools::View::Units);check(catalogTools_->catalogList()->count()>0,"unit insertion catalog is populated from engine definitions");
 handleAction("@units");check(pages_->currentIndex()==8&&converterTools_->view()==CalculatorTools::View::Converter,"CONV opens independent converter");
 const int conversionHistory=history_.size();converterTools_->amountEditor()->setText("2");converterTools_->sourceUnitEditor()->setEditText("km");converterTools_->destinationUnitEditor()->setEditText("m");auto convertButton=converterTools_->findChild<QPushButton*>("convertUnits");convertButton->click();
 check(wait([&]{return convertButton->isEnabled();})&&converterTools_->resultEditor()->text().contains("2000")&&history_.size()==conversionHistory+1&&lastSerialized_=="5"&&input_->text()==beforeTools,"unit conversion records history without changing calculator input or result");
 converterTools_->amountEditor()->setText("3");check(converterTools_->resultEditor()->text().isEmpty(),"editing conversion input clears stale result");
 handleAction("@toolbox");check(pages_->currentIndex()==2&&input_->text()==beforeTools,"OTHER opens function toolbox without changing expression");showPage(0);
 input_->setText("Ans");evaluate();check(idle()&&lastSerialized_=="5","converter preserves calculator Ans");
memory("MS");check(idle()&&memoryValue_=="5","memory store completed");input_->clear();memory("MR");check(input_->text()=="M","memory recall inserts base-independent register");evaluate();check(idle()&&lastSerialized_=="5","recalled register evaluates correctly");memory("M+");check(idle()&&memoryValue_=="10","memory addition preserves canonical total");
 input_->setText("M:=7");evaluate();check(idle()&&memoryValue_=="7","typed memory assignment synchronized");handleAction("@clear");memory("MS");check(idle()&&memoryValue_=="7"&&lastSerialized_.isEmpty()&&result_->text().isEmpty(),"AC removes the current result without storing an invented zero in memory");
 input_->clear();handleAction("sqrt(");check(!input_->isComplete()&&!input_->slotRects().isEmpty(),"root key creates visible editable slot");input_->insert("9");check(input_->isComplete(),"typing completes root slot");evaluate();check(idle()&&lastSerialized_=="3","structured root evaluates canonical expression");
 input_->setText("3");input_->end();handleAction("@fraction");check(!input_->isComplete()&&!input_->slotRects().isEmpty(),"fraction wraps preceding operand and creates denominator slot");input_->insert("4");evaluate();check(idle()&&lastSerialized_.contains("3/4"),"structured fraction evaluates correctly");
 input_->setText("2");input_->end();handleAction("^");input_->insert("3");evaluate();check(idle()&&lastSerialized_=="8","superscript input evaluates correctly");
 input_->clear();handleAction("tan(");handleAction("@right");handleAction("4");handleAction("5");check(input_->text()=="tan(45)"&&input_->isComplete(),"typing at the inside-right boundary fills an empty argument rectangle");
 input_->clear();handleAction("sin(");handleAction("@right");handleAction("@right");check(input_->cursorPosition()==input_->text().size()&&!input_->isComplete(),"Right can leave an unfilled function for later");handleAction("@left");handleAction("5");check(input_->text()=="sin(5)"&&input_->isComplete(),"Left returns to an empty argument and typing fills it");
 input_->clear();handleAction("cos(");handleAction(" deg");handleAction("4");handleAction("5");check(input_->text()=="cos(((45) deg))"&&input_->isComplete(),"unit key owns a filled quantity instead of leaving an orphan slot");
 handleAction("+");handleAction("1");handleAction("5");evaluate();check(idle()&&lastSerialized_.contains("1/2"),"operators extend the angle quantity before its unit");
 input_->clear();handleAction("2");handleAction("^2");handleAction("+");handleAction("5");evaluate();check(idle()&&lastSerialized_=="128","addition remains in active fixed-square exponent");
 input_->clear();handleAction("2");handleAction("^2");handleAction("@right");handleAction("+");handleAction("5");evaluate();check(idle()&&lastSerialized_=="9","Right exits the exponent before outside addition");
 input_->clear();handleAction("pi");handleAction("pi");check(input_->text()=="pi*pi","two pi keys keep separate constant identities");evaluate();check(idle()&&QString(lastSerialized_).remove(' ')=="pi^2","two pi keys evaluate as the product of constants");input_->backspace();check(input_->text()=="pi","Backspace removes an adjacent pi atom as a whole");
 input_->clear();handleAction("5");handleAction("1/(");evaluate();check(idle()&&lastSerialized_.contains("1/5"),"reciprocal wraps the preceding operand");
 input_->clear();handleAction("5");handleAction("/");handleAction("2");handleAction("+");handleAction("9");evaluate();check(idle()&&lastSerialized_.contains("23/2"),"ordinary division followed by addition means five halves plus nine");
 input_->setText("(5)/(2)");input_->setCursorPosition(2);input_->cursorForward();check(input_->cursorPosition()==5,"one Right moves numerator end to denominator start");
 const QString preserved=input_->text();inputStyleBox_->setCurrentIndex(1);check(input_->text()==preserved&&!input_->structuredInput()&&result_->structuredInput(),"linear input setting preserves expression and mathematical result presentation");input_->clear();handleAction("5");handleAction("/");handleAction("2");handleAction("+");handleAction("9");evaluate();check(idle()&&input_->canonicalText()=="5/2+9"&&lastSerialized_.contains("23/2"),"linear keypad keeps ordinary syntax and arithmetic");inputStyleBox_->setCurrentIndex(0);
 input_->setText("8000000.25");groupingBox_->setCurrentIndex(2);decimalSeparatorBox_->setCurrentIndex(1);check(digitGrouping_==1&&input_->text()=="8000000.25"&&formatNumericDisplay(input_->text(),digitGrouping_,decimalSeparator_).replace(QChar(0x202f),' ')=="8 000 000,25","decimal comma resolves conflicting comma grouping without changing source");check(workbenchHub_->graphs->displayNumber(8000000.25,12).replace(QChar(0x202f),' ')=="8 000 000,25","graph numbers follow presentation preferences");groupingBox_->setCurrentIndex(0);decimalSeparatorBox_->setCurrentIndex(0);
 input_->setText("123");input_->setCursorPosition(1);handleAction("@forwardDelete");check(input_->text()=="13","forward-delete action removes character on right");handleAction("@del");check(input_->text()=="3","backspace action removes character on left");
 const int randomHistory=history_.size();input_->setText("rand()");evaluate();check(idle()&&history_.size()==randomHistory+1&&result_->text()!="—","random keypad expression evaluates successfully");
 input_->setText("sin(");const int before=history_.size();evaluate();check(!busy_&&history_.size()==before,"unfinished expression does not reach engine or history");
 input_->setText("1+\n2+\n3");evaluate();check(idle()&&lastSerialized_=="6"&&input_->visualLineCount()>=3,"multiline input is one evaluable expression");
 input_->setText("draft expression");historyCursor_=-1;stepHistory(-1);QString newest=input_->text();stepHistory(-1);check(input_->text()!=newest,"history browses older expression");idle();stepHistory(1);check(input_->text()==newest,"history browses newer expression");idle();stepHistory(1);check(input_->text()=="draft expression","history restores unfinished draft");idle();
 input_->setText("2^10");evaluate();handleAction("@clear");check(idle()&&result_->text().isEmpty()&&result_->resultRelation().isEmpty()&&lastSerialized_.isEmpty(),"AC cannot be overwritten by pending result");
 input_->setText("1/3");evaluate();idle();handleAction("@format");handleAction("@clear");check(idle()&&result_->text().isEmpty()&&result_->resultRelation().isEmpty()&&approx_->text().isEmpty(),"AC cannot be overwritten by an in-flight format request");exactBox_->setCurrentIndex(0);idle();
 input_->setText("FF");evaluate();check(idle()&&lastSerialized_=="255"&&result_->text()=="255"&&result_->resultRelation()=="=","unprefixed uppercase hexadecimal input in DEC produces decimal 255");
 baseBox_->setCurrentIndex(baseBox_->findData(16));idle();input_->setText("FF");evaluate();check(idle()&&lastSerialized_=="255"&&result_->text()=="FF","same hexadecimal input in HEX remains FF without a redundant prefix");baseBox_->setCurrentIndex(baseBox_->findData(10));idle();
 input_->setText("255");evaluate();check(idle()&&lastSerialized_=="255","decimal input before base switch");const int baseHistory=history_.size();baseBox_->setCurrentIndex(baseBox_->findData(16));check(idle()&&result_->text().toLower()=="ff"&&history_.size()==baseHistory,"HEX reformats result without adding history");input_->setText("FF+1");evaluate();check(idle()&&lastSerialized_=="256","HEX expression uses hexadecimal digits");
 baseBox_->setCurrentIndex(baseBox_->findData(2));idle();input_->setText("1010+1");evaluate();check(idle()&&lastSerialized_=="11"&&result_->text()=="1011","binary input and output agree");baseBox_->setCurrentIndex(baseBox_->findData(16));check(idle()&&input_->text()=="FF+1","base switch restores its own draft");baseBox_->setCurrentIndex(baseBox_->findData(10));check(idle()&&input_->text()=="255","decimal draft survives base switches");
 recallHistoryEntry({{"expression","FF"},{"base",16}});recallHistoryEntry({{"expression","1011"},{"base",2}});recallHistoryEntry({{"expression","17"},{"base",8}});check(idle()&&numberBase_==8&&input_->numberBase()==8&&result_->numberBase()==8&&result_->text().isEmpty()&&input_->text()=="17","rapid mixed-base draft recall leaves no unrelated answer");baseBox_->setCurrentIndex(baseBox_->findData(10));idle();
 exactBox_->setCurrentIndex(1);idle();input_->setText("12345");evaluate();idle();notationBox_->setCurrentIndex(2);check(idle()&&result_->text().contains('E'),"engineering notation formats numeric result");notationBox_->setCurrentIndex(0);idle();exactBox_->setCurrentIndex(0);idle();
 const auto realHistory=history_;QJsonArray many;for(int i=0;i<201;++i)many.append(QJsonObject{{"expression",QString("marker%1+sqrt(2)").arg(i)},{"result","3"},{"time","2026-10-03T00:00:00Z"}});history_=many;historyPage_=0;fillHistory();check(historyTable_->rowCount()==100&&historyNext_->isEnabled(),"history displays bounded pages");historyNext_->click();historyNext_->click();check(historyTable_->rowCount()==1&&!historyNext_->isEnabled(),"history last page and next control are bounded");historySearch_->setText("marker200");check(historyTable_->rowCount()==1&&historyTable_->item(0,0)->data(Qt::UserRole).toInt()==200,"history search finds entries outside current page");check(historyTable_->cellWidget(0,0)!=nullptr,"history expressions use mathematical rendering");historySearch_->clear();history_=realHistory;historyPage_=0;fillHistory();
 auto switchMode=[&](const QString& mode){designBox_->setCurrentIndex(designBox_->findData(mode));if(settingsDraftActive_)applySettingsDraft();showPage(0);QApplication::processEvents();};
 auto switchSkin=[&](const QString& skin){calculatorThemeBox_->setCurrentIndex(calculatorThemeBox_->findData(skin));QApplication::processEvents();};
 auto switchKeys=[&](const QString& layout){calculatorLayoutBox_->setCurrentIndex(calculatorLayoutBox_->findData(layout));QApplication::processEvents();};
 auto fitsKeys=[&]{bool fits=true;for(auto key:keys_)fits&=calc_->rect().contains(QRect(key->mapTo(calc_,QPoint()),key->size()));for(int i=0;i<keys_.size();++i)for(int j=i+1;j<keys_.size();++j)fits&=!keys_[i]->geometry().intersects(keys_[j]->geometry());return fits;};
 auto fullClient=[&]{return calc_->mapTo(centralWidget(),QPoint())==QPoint()&&calc_->size()==centralWidget()->size()&&!desktopHeader_->isVisible()&&!navigation_->isVisible()&&!status_->isVisible()&&!auxHeader_->isVisible();};
 check(designBox_->count()==2&&designBox_->findData("Calculator")>=0,"settings offers exactly two main modes");
 auto settingsMatchMode=[&]{bool match=true;for(QWidget*field:{desktopEffectsBox_,themeBox_,appearanceBox_,layoutBox_})match&=field->isHidden()==handheld_&&settingsForm_->labelForField(field)->isHidden()==handheld_;for(QWidget*field:{calculatorThemeBox_,calculatorLayoutBox_})match&=field->isHidden()!=handheld_&&settingsForm_->labelForField(field)->isHidden()!=handheld_;return match;};
 for(const auto&mode:QStringList{"Desktop","Calculator"}){switchMode(mode);check(settingsMatchMode(),"mode-specific settings hide both irrelevant fields and labels");}
 auto uniqueFunctions=[&]{QSet<QString>seen;bool unique=true;auto add=[&](QString token){if(token.isEmpty())return;if(token=="@mr")token="M";if(seen.contains(token))unique=false;seen.insert(token);};for(auto key:calculator_->keyButtons()){add(key->primaryToken());add(key->alternateToken());add(key->alphaToken());}if(!handheld_)for(auto button:calculator_->memoryButtons())add(static_cast<CalculatorKey*>(button)->primaryToken());return unique;};
 switchMode("Desktop");check(uniqueFunctions(),"Desktop keypad has no duplicate assigned functions including memory");switchMode("Calculator");for(const auto&layout:QStringList{"Pocket","Compact","Expanded"}){switchKeys(layout);check(uniqueFunctions(),"Calculator keypad has no duplicate assigned functions in active layout");}switchKeys("Compact");
 // Real MainWindow action handlers consume one-shot layers but preserve physical holds.
 auto physicalKey=[&](QEvent::Type type,int key,Qt::KeyboardModifiers modifiers=Qt::NoModifier,const QString& text=QString{}){QKeyEvent event(type,key,modifiers,text);QApplication::sendEvent(input_,&event);};
 auto keyStroke=[&](int key,Qt::KeyboardModifiers modifiers=Qt::NoModifier,const QString& text=QString{}){physicalKey(QEvent::KeyPress,key,modifiers,text);physicalKey(QEvent::KeyRelease,key,modifiers,text);};
 for(const QString& mode:QStringList{"Desktop","Calculator"}){
  switchMode(mode);input_->setFocus();input_->clear();
  QString shiftedThree;CalculatorKey* three=nullptr;for(auto key:calculator_->keyButtons())if(key->primaryToken()=="3"){three=key;shiftedThree=key->alternateToken();break;}
  physicalKey(QEvent::KeyPress,Qt::Key_Shift,Qt::ShiftModifier);
  check(calculator_->hasActiveLayer()&&three&&three->token()==shiftedThree,"holding physical Shift selects the SHIFT face immediately in both main modes");
  keyStroke(Qt::Key_3,Qt::ShiftModifier,"#");
  check(!shiftedThree.isEmpty()&&input_->text().startsWith(shiftedThree)&&calculator_->hasActiveLayer(),"held Shift plus top-row 3 invokes its function and remains active after MainWindow consumes the action");
  input_->clear();keyStroke(Qt::Key_3,Qt::ShiftModifier|Qt::KeypadModifier,"3");
  check(input_->text().startsWith(shiftedThree)&&calculator_->hasActiveLayer(),"the same held Shift works for the next numpad 3");
  input_->clear();if(three)three->click();
  check(input_->text().startsWith(shiftedThree)&&calculator_->hasActiveLayer(),"physical hold also applies to virtual calculator clicks");
  physicalKey(QEvent::KeyRelease,Qt::Key_Shift);
  check(!calculator_->hasActiveLayer()&&!shift_&&!alpha_,"releasing a used Shift hold restores the normal layer without latching");
  input_->clear();physicalKey(QEvent::KeyPress,Qt::Key_Alt,Qt::AltModifier);keyStroke(Qt::Key_7,Qt::AltModifier,"7");keyStroke(Qt::Key_8,Qt::AltModifier|Qt::KeypadModifier,"8");
  check(input_->text().contains('A')&&input_->text().contains('B')&&calculator_->hasActiveLayer(),"held Alt keeps ALPHA active for successive top-row and numpad keys");
  physicalKey(QEvent::KeyRelease,Qt::Key_Alt);check(!calculator_->hasActiveLayer()&&!alpha_,"used Alt hold ends without toggling ALPHA");
  for(const auto& pair:QList<QPair<int,QString>>{{16,"FF.CA+0.36"},{2,"10.01+0.11"},{8,"7.4+0.4"}}){
   changeBase(pair.first);idle();input_->setText(pair.second);evaluate();const QString wanted=pair.first==16?"100":pair.first==2?"11":"10";
   check(idle()&&result_->text()==wanted&&result_->resultRelation()=="=","fractional radix input evaluates exactly in both main modes");
  }
  changeBase(16);idle();input_->setText("FF.CA");evaluate();check(idle()&&result_->text()=="FF.CA"&&result_->resultRelation()=="=","HEX keeps a terminating radix fraction exact");
  copyResult();check(QApplication::clipboard()->text()=="FF.CA","copy preserves canonical radix point syntax");
  memory("MS");check(idle()&&memoryValue_.contains("32741/128"),"memory stores the exact canonical HEX fraction");changeBase(2);idle();input_->setText("M");evaluate();check(idle()&&lastSerialized_.contains("32741/128")&&result_->text()=="11111111.1100101","memory fraction reformats exactly in binary");changeBase(16);idle();check(input_->text()=="FF.CA","fractional HEX draft survives a binary memory calculation");evaluate();idle();
  const QJsonObject fractionalRow=history_.first().toObject();decimalSeparatorBox_->setCurrentIndex(1);recallHistoryEntry(fractionalRow);check(idle()&&input_->text()=="FF.CA"&&formatNumericDisplay(input_->text(),digitGrouping_,decimalSeparator_,numberBase_).contains("FF,CA"),"fractional history recall retains canonical syntax and the selected decimal mark");decimalSeparatorBox_->setCurrentIndex(0);
  const int fractionHistory=history_.size();handleAction("@format");check(idle()&&result_->text()=="FF.CA"&&result_->resultRelation()=="="&&history_.size()==fractionHistory,"S to D preserves an exact terminating HEX value without another history entry");handleAction("@format");idle();
  changeBase(2);idle();input_->setText("1/11");evaluate();check(idle()&&result_->resultRelation()=="="&&QString(result_->text()).replace(QChar(0x2215),'/').contains('/'),"a repeating base expansion remains an exact fraction in Exact mode");handleAction("@format");check(idle()&&result_->resultRelation()=="="&&result_->text()=="0.01"&&!result_->recurringBarRects().isEmpty(),"Decimal mode marks the complete binary cycle with an exact overline");handleAction("@format");idle();
  changeBase(10);idle();input_->setText("FF.CA");evaluate();check(idle()&&lastSerialized_.contains("32741/128"),"DEC convenience hexadecimal fractions preserve exact canonical arithmetic");handleAction("@format");check(idle()&&result_->text()=="255.7890625"&&result_->resultRelation()=="=","DEC renders the exact decimal value of FF.CA");handleAction("@format");idle();
 }
 switchMode("Calculator");
 // Recurrence is exact rational metadata, preserved through every result surface.
 for(const QString& mode:QStringList{"Desktop","Calculator"}){
  switchMode(mode);changeBase(10);idle();exactBox_->setCurrentIndex(1);idle();
  for(const auto& fixture:QList<QStringList>{{"1/3","0.3","2","1"},{"1/6","0.16","3","1"},{"1/11","0.09","2","2"},{"20/3","6.6","2","1"}}){
   input_->setText(fixture[0]);evaluate();check(idle()&&result_->text()==fixture[1]&&result_->resultRelation()=="="&&result_->recurringBarRects().size()==1,"complete repeating rational has one exact overline in either main mode");
   const auto row=history_.first().toObject();check(row["resultRepeatStart"].toInt(-1)==fixture[2].toInt()&&row["resultRepeatLength"].toInt()==fixture[3].toInt()&&!row["resultCopyValue"].toString().isEmpty(),"history stores cycle boundaries and exact copy value");
   copyResult();check(QApplication::clipboard()->text().remove(' ')==fixture[0],"copying a repeating result keeps the exact rational instead of a truncated decimal");
   auto historyResult=dynamic_cast<MathEditor*>(historyTable_->cellWidget(0,1));check(historyResult&&historyResult->recurringBarRects().size()==1&&historyResult->toPlainText().remove(' ')==fixture[0],"history uses the same precise overline and copy representation");
   check(exportedValue(row,"result").remove(' ')==fixture[0],"CSV result export preserves the exact repeating value");
  }
  input_->setText("1/6");evaluate();idle();handleAction("@format");check(idle()&&approx_->text()=="= 0.16"&&approx_->property("canonicalRepeatStart").toInt()==3&&approx_->property("repeatLength").toInt()==1,"Exact secondary display marks only the repeating six");
  decimalSeparatorBox_->setCurrentIndex(1);check(approx_->text()=="= 0,16"&&approx_->property("repeatLength").toInt()==1,"decimal mark changes preserve the secondary overline");decimalSeparatorBox_->setCurrentIndex(0);
  handleAction("@format");idle();memory("MS");check(idle()&&memoryValue_.remove(' ')=="1/6","memory retains the exact value of the repeating display");
  input_->setText("1/2");evaluate();check(idle()&&result_->text()=="0.5"&&result_->recurringBarRects().isEmpty()&&result_->resultRelation()=="=","finite decimal removes any old repeating bar");
  precisionBox_->setValue(10);idle();input_->setText("1/97");evaluate();check(idle()&&result_->resultRelation()=="≈"&&result_->recurringBarRects().isEmpty(),"cycle beyond the precision budget remains an honest rounded approximation");precisionBox_->setValue(30);idle();
  input_->setText("sqrt(2)");evaluate();check(idle()&&result_->resultRelation()=="≈"&&result_->recurringBarRects().isEmpty(),"irrational digit coincidences never create a repeating bar");
  continuousEvaluationBox_->setCurrentIndex(1);const int beforePreview=history_.size();input_->setText("1/11");check(wait([&]{return !previewRunning_&&!previewPending_&&!previewTimer_->isActive()&&!busy_&&!formatPending_;})&&result_->text()=="0.09"&&result_->recurringBarRects().size()==1&&history_.size()==beforePreview,"automatic preview renders the repetend without recording history");
  input_->setText("1/");check(wait([&]{return !previewRunning_&&!previewPending_&&!previewTimer_->isActive()&&!busy_&&!formatPending_;})&&result_->text().isEmpty()&&result_->recurringBarRects().isEmpty()&&approx_->property("repeatLength").toInt()==0,"incomplete preview clears every stale overline");continuousEvaluationBox_->setCurrentIndex(0);exactBox_->setCurrentIndex(0);idle();
 }
 switchMode("Calculator");switchKeys("Compact");changeBase(10);idle();
 for(bool complex:{false,true}){
  complexBox_->setCurrentIndex(complex?1:0);input_->setText("5");input_->end();input_->setFocus();physicalKey(QEvent::KeyPress,Qt::Key_Shift,Qt::ShiftModifier);keyStroke(Qt::Key_4,Qt::ShiftModifier,"$");
  check(input_->text()=="5!"&&calculator_->effectiveShift(),"held Shift plus 4 inserts factorial in REAL and CPLX Calculator layouts");physicalKey(QEvent::KeyRelease,Qt::Key_Shift);evaluate();check(idle()&&result_->text()=="120"&&!calculator_->hasActiveLayer(),"factorial evaluates correctly and releasing the hold returns to normal");
 }
 // Raw keypad identities must win over Windows navigation/Alt-code translation.
 auto nativeStroke=[&](int key,Qt::KeyboardModifiers modifiers,quint32 scan,quint32 virtualKey,const QString& text=QString{}){
  bool consumed=true;
  for(auto type:{QEvent::ShortcutOverride,QEvent::KeyPress,QEvent::KeyRelease}){QKeyEvent event(type,key,modifiers,scan,virtualKey,0,text);event.setAccepted(false);QApplication::sendEvent(input_,&event);consumed&=event.isAccepted();}
  return consumed;
 };
 input_->setText("5");input_->end();input_->setFocus();physicalKey(QEvent::KeyPress,Qt::Key_Shift,Qt::ShiftModifier);
 check(nativeStroke(Qt::Key_Left,Qt::ShiftModifier,0x4b,0x25)&&input_->text()=="5!"&&calculator_->effectiveShift(),"native Shift plus Num4 consumes all phases and inserts factorial instead of moving left");
 physicalKey(QEvent::KeyRelease,Qt::Key_Shift);evaluate();check(idle()&&result_->text()=="120"&&!calculator_->hasActiveLayer(),"native keypad factorial evaluates and its hold releases normally");
 input_->clear();physicalKey(QEvent::KeyPress,Qt::Key_Alt,Qt::AltModifier);
 check(nativeStroke(Qt::Key_9,Qt::AltModifier|Qt::KeypadModifier,0x49,0x69,"9")&&input_->text()=="0xC"&&calculator_->effectiveAlpha(),"native Alt plus Num9 consumes all phases and selects ALPHA C exactly once");
 physicalKey(QEvent::KeyRelease,Qt::Key_Alt);check(input_->text()=="0xC"&&!calculator_->hasActiveLayer(),"Alt keypad release leaves only the calculator action without an extra character");
 // A long unused hold also exits to normal; a short unused tap still toggles.
 handleAction("@alpha");physicalKey(QEvent::KeyPress,Qt::Key_Shift,Qt::ShiftModifier);QElapsedTimer heldDuration;heldDuration.start();wait([&]{return heldDuration.elapsed()>=400;},700);physicalKey(QEvent::KeyRelease,Qt::Key_Shift);check(!shift_&&!alpha_&&!calculator_->hasActiveLayer(),"releasing a long unused hold clears even a previously latched layer");keyStroke(Qt::Key_Shift);check(shift_&&calculator_->hasActiveLayer(),"quick physical Shift tap still matches clicking SHIFT");handleAction("@shift");
 // Continuous evaluation is a display preview, independent of committed result state.
 auto previewIdle=[&]{return wait([&]{return !previewRunning_&&!previewPending_&&!previewTimer_->isActive()&&!busy_&&!formatPending_;});};
 for(const QString& mode:QStringList{"Desktop","Calculator"}){
  switchMode(mode);input_->setText("7");evaluate();idle();const int committedHistory=history_.size();
  continuousEvaluationBox_->setCurrentIndex(1);input_->setText("2+3");
  check(result_->text().isEmpty()&&result_->resultRelation().isEmpty()&&approx_->text().isEmpty(),"editing clears old result immediately while preview is pending");
  check(previewIdle()&&previewDisplayed_&&result_->text()=="5"&&approx_->text().startsWith(trc("Preview"))&&lastSerialized_=="7"&&history_.size()==committedHistory,"continuous evaluation shows labelled preview without changing committed result or history in each mode");
  check(approx_->isVisible()&&approx_->height()>=approx_->fontMetrics().height()&&!approx_->geometry().intersects(result_->geometry()),"preview label has visible space separate from result in each mode");
  memory("MS");check(idle()&&memoryValue_=="7","memory store uses confirmed value rather than automatic preview");
  input_->setText("Ans+1");check(previewIdle()&&result_->text()=="8"&&lastSerialized_=="7","automatic preview leaves Ans unchanged");
  input_->setText("M:=99");check(previewIdle()&&!previewDisplayed_&&result_->text().isEmpty()&&approx_->text().isEmpty()&&memoryValue_=="7"&&history_.size()==committedHistory,"automatic preview does not execute assignments");
  for(const QString& incomplete:QStringList{"sin(□)","2+","x^","sqrt(","sin(2+)","2/(+)","x^(2+)"}){
   input_->setText(incomplete);check(previewIdle()&&!previewDisplayed_&&result_->text().isEmpty()&&result_->resultRelation().isEmpty()&&approx_->text().isEmpty()&&history_.size()==committedHistory,"incomplete input clears the preview and is not evaluated");
  }
  input_->setText("1/0");check(previewIdle()&&!previewDisplayed_&&result_->text().isEmpty()&&result_->resultRelation().isEmpty()&&approx_->text().isEmpty(),"preview evaluation errors leave a blank display");
  input_->clear();check(previewIdle()&&result_->text().isEmpty()&&approx_->text().isEmpty(),"empty input with automatic evaluation keeps the result blank");
  input_->setText("sin(□)");handleAction("@format");check(previewIdle()&&result_->text().isEmpty()&&approx_->text().isEmpty(),"format callback cannot resurrect old result for incomplete preview input");handleAction("@format");previewIdle();precisionBox_->setValue(12);check(previewIdle()&&result_->text().isEmpty(),"precision callback cannot resurrect old result for incomplete input");precisionBox_->setValue(30);previewIdle();
  input_->setText("2+3");check(previewIdle()&&previewDisplayed_,"preview resumes after input becomes complete");evaluate();
  check(idle()&&!previewDisplayed_&&result_->text()=="5"&&lastSerialized_=="5"&&history_.size()==committedHistory+1,"explicit evaluation commits exactly one history entry after a preview");
  input_->setText("21*2");previewTimer_->stop();startPreview();input_->setText("40+3");
  check(previewIdle()&&result_->text()=="43"&&lastSerialized_=="5","stale preview cannot overwrite a newer expression");
  input_->setText("12*12");previewTimer_->stop();startPreview();continuousEvaluationBox_->setCurrentIndex(0);
  check(previewIdle()&&!previewDisplayed_&&result_->text()=="5"&&history_.size()==committedHistory+1,"turning continuous evaluation off cancels pending results and restores confirmed display");
 }
 switchMode("Calculator");continuousEvaluationBox_->setCurrentIndex(1);input_->setText("sin(90)");angleBox_->setCurrentIndex(1);
 check(previewIdle()&&result_->text()=="1","preview follows angle setting changes");angleBox_->setCurrentIndex(0);
 check(previewIdle()&&result_->text()!="1","stale degree preview clears when radians are selected");
 input_->setText("sqrt(-1)");complexBox_->setCurrentIndex(1);check(previewIdle()&&previewDisplayed_&&result_->text().contains('i'),"complex preview follows CPLX domain");complexBox_->setCurrentIndex(0);
 check(previewIdle()&&!previewDisplayed_,"REAL mode rejects complex automatic results");complexBox_->setCurrentIndex(1);
 input_->setText("1/2");exactBox_->setCurrentIndex(0);idle();check(previewIdle()&&QString(result_->text()).replace(QChar(0x2215),'/').remove(' ')=="1/2","exact preview retains rational result");const auto previewFormatCommitted=lastSerialized_;const int previewFormatHistory=history_.size();handleAction("@format");check(previewIdle()&&result_->text()=="0.5"&&lastSerialized_==previewFormatCommitted&&history_.size()==previewFormatHistory,"S to D formats preview without committing it");exactBox_->setCurrentIndex(0);idle();
 input_->setText("100");previewTimer_->stop();startPreview();handleAction("@clear");check(previewIdle()&&!previewDisplayed_&&result_->text().isEmpty()&&result_->resultRelation().isEmpty()&&approx_->text().isEmpty(),"AC invalidates an in-flight automatic preview");
 changeBase(16);idle();input_->setText("FF+1");check(previewIdle()&&result_->text().toLower()=="100","continuous evaluation uses current input and output radix");changeBase(10);idle();
 input_->setText("10");previewIdle();changeBase(2);idle();input_->setText("10");check(previewIdle()&&result_->text()=="10","binary preview interprets its own draft");changeBase(10);check(previewIdle()&&!previewDisplayed_&&result_->text().isEmpty(),"base switch restores a draft without previewing it when there is no confirmed result");
 input_->setText("sin(90)");previewIdle();recallHistoryEntry({{"expression","sin(90)"},{"angle",1},{"base",10}});check(previewIdle()&&result_->text()=="1","same-text history recall recomputes preview with recalled angle");
 input_->setText("M+1");memory("MC");check(previewIdle()&&result_->text()=="1","memory clear refreshes a preview that references M");input_->setText("4");evaluate();idle();input_->setText("M+1");previewIdle();memory("MS");check(previewIdle()&&memoryValue_=="4"&&result_->text()=="5","memory store refreshes a preview after its mutation");
 input_->setText("0");evaluate();check(idle()&&!previewDisplayed_&&result_->text()=="0"&&result_->resultRelation()=="=","explicitly calculated zero remains a legitimate result");handleAction("@clear");handleAction("@format");check(previewIdle()&&result_->text().isEmpty()&&result_->resultRelation().isEmpty()&&approx_->text().isEmpty()&&lastSerialized_.isEmpty(),"AC and format changes retain a blank display with automatic evaluation on");
 continuousEvaluationBox_->setCurrentIndex(0);
 for(const QString&token:QStringList{"solve(","multisolve(","diff(","integrate(","limit(","sum(","product(","mean(","median(","stdev(","binomial(","factor(","gcd(","@expression","@complexTemplate","@detTemplate","@inverseTemplate","@assignTemplate","@fraction"}){input_->clear();startTemplate(token);check(!input_->isComplete()&&input_->text().contains(QChar(0x25a1))&&!QRegularExpression("[0-9]").match(input_->text()).hasMatch(),"function toolbox templates have blank values and no sample numbers");}

 switchMode("Calculator");check(!angleBox_->isVisible()&&!baseBox_->isVisible()&&!exactBox_->isVisible()&&!notationBox_->isVisible(),"Calculator uses status indicators without dropdown controls");
 for(int effect:{0,1,2}){switchMode("Desktop");desktopEffectsBox_->setCurrentIndex(effect);QApplication::processEvents();check(!angleBox_->isHidden()&&desktopEffects_==effect&&fitsKeys(),"Desktop keeps editable controls with each surface effect");}desktopEffectsBox_->setCurrentIndex(1);

 for(const auto& mode:QStringList{"Desktop","Calculator"}){
  switchMode(mode);const int targetWidth=handheld_?320:680;
  for(const auto& language:languageCodes()){
   showPage(0);language_=language;updateTranslations();resize(targetWidth,540);QApplication::processEvents();check(width()==targetWidth&&height()==540&&fitsKeys(),"mode minimum width and key geometry in each locale");
   if(handheld_){check(fullClient(),"calculator fills client area with desktop chrome hidden");showPage(3);QApplication::processEvents();auto scroll=qobject_cast<QScrollArea*>(pages_->widget(3));check(scroll&&scroll->horizontalScrollBar()->maximum()==0&&settingsPage_->width()<=scroll->viewport()->width(),"settings fit portrait width in each locale");goBack();
    showGraph();QApplication::processEvents();auto *graphs=workbenchHub_->graphs;auto *graphTabs=graphs->findChild<QTabWidget*>("graphTabs");bool fits=graphTabs&&graphTabs->count()==4;const int savedTab=graphTabs?graphTabs->currentIndex():0;
    if(graphTabs)for(int tab=0;tab<graphTabs->count();++tab){graphTabs->setCurrentIndex(tab);QApplication::processEvents();QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);QApplication::processEvents();fits&=graphs->isVisible()&&graphs->width()<=320&&graphs->height()<=pages_->height();for(auto scroll:graphs->findChildren<QScrollArea*>())if(scroll->isVisibleTo(graphs))fits&=scroll->horizontalScrollBar()->maximum()==0;for(auto button:graphs->findChildren<QAbstractButton*>())if(button->isVisibleTo(graphs)){const QRect bounds(button->mapTo(graphs,QPoint()),button->size());fits&=bounds.left()>=0&&bounds.right()<graphs->width();}}if(graphTabs)graphTabs->setCurrentIndex(savedTab);check(fits,"all four active graph tabs fit portrait width in each locale with local scrolling");showPage(0);
    showPage(6);QApplication::processEvents();bool fitsMenu=true;for(auto card:modeMenu_->cardButtons())fitsMenu&=modeMenu_->rect().contains(QRect(card->mapTo(modeMenu_,QPoint()),card->size()));check(fitsMenu,"menu and memory controls fit portrait width in each locale");goBack();
    for(int page:{7,8}){showPage(page);QApplication::processEvents();bool toolsFit=pages_->currentWidget()->width()<=320;for(auto scroll:pages_->currentWidget()->findChildren<QScrollArea*>())toolsFit&=scroll->horizontalScrollBar()->maximum()==0;check(toolsFit,"catalog and converter fit portrait width in each locale");goBack();}
    for(auto view:{CalculatorTools::View::Statistics,CalculatorTools::View::Matrix}){catalogTools_->setView(view);showPage(7);QApplication::processEvents();bool fits=catalogTools_->width()<=320;for(auto button:catalogTools_->findChildren<QPushButton*>())if(button->isVisibleTo(catalogTools_))fits&=catalogTools_->rect().contains(QRect(button->mapTo(catalogTools_,QPoint()),button->size()));check(fits,"statistics and matrix tools fit minimum portrait in every locale");goBack();}catalogTools_->setView(CalculatorTools::View::Constants);
}
  }
 }
 language_="en";updateTranslations();switchMode("Calculator");
 for(const auto& skin:calculatorThemeNames()){
  showPage(0);switchSkin(skin);check(qApp->palette().color(QPalette::Window)==calculatorThemeDefinition(skin).body&&historyTable_->palette().color(QPalette::AlternateBase)==calculatorThemePalette(skin).color(QPalette::AlternateBase),"calculator skin colors the whole application and alternating history rows");
  for(const auto& layout:QStringList{"Pocket","Compact","Expanded"}){
   switchKeys(layout);check(keys_.size()==(layout=="Pocket"?39:layout=="Compact"?51:57),"calculator layout exposes its expected keys");
   for(const auto& size:QList<QSize>{{320,540},{360,720},{800,540}}){resize(size);QApplication::processEvents();bool banks=true;if(size.width()>size.height()){auto first=keys_.first();auto seven=std::find_if(keys_.begin(),keys_.end(),[](CalculatorKey*k){return k->primaryToken()=="7";});banks=seven!=keys_.end()&&first->geometry().right()<(*seven)->geometry().left()&&qAbs(first->geometry().top()-(*seven)->geometry().top())<3&&input_->mapTo(calc_,QPoint()).y()<first->geometry().top();}check(this->size()==size&&fullClient()&&fitsKeys()&&banks,"skin and keypad layout fit portrait and reference landscape banks");}
  }
  for(int page:{1,3,5,6,7,8}){showPage(page);QApplication::processEvents();check(pages_->currentWidget()->palette().color(QPalette::Window)==calculatorThemeDefinition(skin).body,"auxiliary page follows Calculator theme");goBack();}
 }
 switchSkin("Classic");switchKeys("Compact");resize(320,640);showPage(1);QApplication::processEvents();sizeHistoryRows();bool historyFits=true;for(int row=0;row<historyTable_->rowCount();++row)for(int col=0;col<2;++col)if(auto math=dynamic_cast<MathEditor*>(historyTable_->cellWidget(row,col)))historyFits&=historyTable_->rowHeight(row)>=std::ceil(math->naturalSize().height());check(historyFits,"narrow History rows fit multiline mathematical input and results");showPage(0);QApplication::processEvents();input_->setText("147+375");evaluate();check(idle()&&result_->text()=="522","reference-style example evaluates");const auto kept=input_->text();
 check(!calculator_->settingsButton()->icon().isNull()&&!gear_->icon().isNull(),"scalable gear icons use active theme");calculator_->settingsButton()->click();check(pages_->currentIndex()==3&&auxHeader_->isVisible(),"gear opens themed settings");back_->click();check(pages_->currentIndex()==0&&input_->text()==kept,"Back preserves expression");handleAction("@menu");check(pages_->currentIndex()==10&&workbenchHub_->currentDestination()=="home","MENU opens workspace categories");workbenchHub_->open("graphs");check(pages_->currentIndex()==10&&workbenchHub_->currentDestination()=="graphs","Graph opens its dedicated workbench");back_->click();check(workbenchHub_->currentDestination()=="category:calculate","Back returns to graph category");back_->click();check(workbenchHub_->currentDestination()=="home","Back returns to menu");back_->click();check(pages_->currentIndex()==0&&input_->text()==kept,"final Back restores calculator");
 menuAction("solve");check(input_->text().contains(kept)&&input_->text().contains("solve(")&&!input_->isComplete()&&pages_->currentIndex()==0,"equation menu preserves existing input and inserts empty arguments");input_->clear();menuAction("matrix");check(pages_->currentIndex()==7&&catalogTools_->view()==CalculatorTools::View::Matrix&&input_->text().isEmpty(),"matrix menu opens dimension controls without inserting sample values");catalogTools_->findChild<QPushButton*>("insertMatrix")->click();check(input_->text().startsWith("[")&&!input_->isComplete()&&pages_->currentIndex()==0,"matrix insert creates empty editable grid slots");input_->setText(kept);
 switchMode("Desktop");resize(1000,680);rememberWindowGeometry();switchMode("Calculator");resize(360,700);rememberWindowGeometry();switchMode("Desktop");check(size()==QSize(1000,680),"Desktop geometry restored independently");switchMode("Calculator");check(size()==QSize(360,700)&&fullClient(),"Calculator geometry restored independently");
 if(!output.isEmpty()){
  QDir().mkpath(output);auto shot=[&](const QString&name){QApplication::processEvents();QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);grab().save(output+"/"+name+".png");};
  workbenchHub_->graphs->clearGraphs();workbenchHub_->graphs->addExpression("sin(x)");workbenchHub_->graphs->addExpression("cos(x)");workbenchHub_->graphs->requestPlot();check(wait([&]{return workbenchHub_->graphs->result().series.size()==2;},15000),"gallery uses two actually sampled graph series");workbenchHub_->open("graphs");
  switchMode("Desktop");layout_="Adaptive";theme_="Graphite";appearance_="Dark";desktopEffectsBox_->setCurrentIndex(1);applyTheme();resize(1180,760);updateLayout();shot("desktop-dark");appearance_="Light";applyTheme();shot("desktop-light");
  for(const auto& theme:QStringList{"Graphite","Classic","Ocean","Forest","Amber","Rose"}){theme_=theme;appearance_="Dark";applyTheme();shot("desktop-theme-"+theme.toLower());}
  for(int effect:{0,1,2}){desktopEffectsBox_->setCurrentIndex(effect);shot("desktop-effect-"+QStringList{"flat","glass","raised"}[effect]);}desktopEffectsBox_->setCurrentIndex(1);
  switchMode("Calculator");switchKeys("Compact");resize(360,720);
  input_->setText("sqrt(2)+(3)/(4)+sin(pi/6)");evaluate();idle();
  for(const auto& skin:calculatorThemeNames()){switchSkin(skin);QString slug=skin.toLower().replace(' ','-');shot("calculator-"+slug);}
  switchSkin("Classic");resize(320,640);shot("classic-portrait");resize(800,540);shot("classic-landscape");resize(1440,900);shot("classic-landscape-large");handleAction("@shift");shot("shift-layer");handleAction("@alpha");shot("alpha-layer");handleAction("@alpha");resize(360,720);
  input_->setText("sqrt(□)+(□)/(□)+(□)^(□)");shot("structured-slots");input_->setText("(1+2)/(3+4)+sqrt(5)+\nsin(pi/6)+2^3");evaluate();idle();shot("structured-multiline");
  inputStyleBox_->setCurrentIndex(1);input_->setText("5/2+9");evaluate();idle();shot("linear-input");inputStyleBox_->setCurrentIndex(0);
  input_->setText("8000000.25");exactBox_->setCurrentIndex(1);idle();evaluate();idle();groupingBox_->setCurrentIndex(1);decimalSeparatorBox_->setCurrentIndex(1);shot("number-formatting");groupingBox_->setCurrentIndex(0);decimalSeparatorBox_->setCurrentIndex(0);exactBox_->setCurrentIndex(0);idle();
  switchSkin("Modern Light");switchKeys("Pocket");shot("pocket-layout");switchSkin("Deep Ocean");switchKeys("Expanded");shot("expanded-layout");
  baseBox_->setCurrentIndex(baseBox_->findData(16));idle();input_->setText("FF+1");evaluate();idle();shot("hexadecimal");baseBox_->setCurrentIndex(baseBox_->findData(10));idle();input_->setText("12345");exactBox_->setCurrentIndex(1);idle();evaluate();idle();notationBox_->setCurrentIndex(2);idle();shot("engineering");notationBox_->setCurrentIndex(0);idle();exactBox_->setCurrentIndex(0);idle();
  switchSkin("Classic");switchKeys("Compact");resize(320,640);showPage(1);shot("history-narrow");showPage(6);shot("menu-narrow");showPage(3);shot("settings-narrow");showPage(5);shot("graph-narrow");
  handleAction("@constants");shot("constants-catalog");catalogTools_->setView(CalculatorTools::View::Units);shot("units-catalog");
  handleAction("@units");converterTools_->amountEditor()->setText("25");converterTools_->sourceUnitEditor()->setCurrentIndex(converterTools_->sourceUnitEditor()->findData("oC"));converterTools_->destinationUnitEditor()->setCurrentIndex(converterTools_->destinationUnitEditor()->findData("oF"));converterTools_->findChild<QPushButton*>("convertUnits")->click();check(wait([&]{return converterTools_->findChild<QPushButton*>("convertUnits")->isEnabled();})&&converterTools_->resultEditor()->text().contains("77"),"converter gallery shows a real Celsius to Fahrenheit result");shot("unit-converter");
  handleAction("@toolbox");shot("functions-catalog");handleAction("@clear");startTemplate("solve(");shot("blank-solver-template");
  input_->setText("sin(90 deg)+cos(pi rad)");evaluate();idle();shot("angle-unit-tokens");
  handleAction("@clear");handleAction("tan(");handleAction("@right");shot("slot-boundary");
  handleAction("@clear");handleAction("cos(");handleAction(" deg");handleAction("4");handleAction("5");shot("unit-quantity");
  handleAction("@clear");handleAction("x");handleAction("^2");handleAction("+");handleAction("5");shot("exponent-inside");
  handleAction("@clear");handleAction("x");handleAction("^2");handleAction("@right");handleAction("+");handleAction("5");shot("exponent-outside");
  handleAction("@clear");handleAction("pi");handleAction("pi");shot("pi-product");
  handleAction("@clear");handleAction("sqrt(");handleAction("@right");handleAction("+");handleAction("5");shot("filled-slot");
  complexBox_->setCurrentIndex(0);switchSkin("Classic");switchKeys("Compact");resize(360,720);shot("real-keypad-portrait");resize(800,540);shot("real-keypad-landscape");resize(360,720);
  continuousEvaluationBox_->setCurrentIndex(1);input_->setText("sqrt(2)");check(previewIdle()&&previewDisplayed_&&approx_->height()>=approx_->fontMetrics().height(),"screenshot includes a visibly labelled automatic preview");shot("continuous-preview");
  input_->setText("sin(□)");shot("incomplete-preview");showPage(3);shot("continuous-settings");showPage(0);
  continuousEvaluationBox_->setCurrentIndex(0);complexBox_->setCurrentIndex(1);
  exactBox_->setCurrentIndex(0);idle();angleBox_->setCurrentIndex(0);inputStyleBox_->setCurrentIndex(0);switchSkin("Classic");switchKeys("Compact");resize(360,720);
  input_->setText("56*sqrt(2)÷99");evaluate();idle();shot("exact-radical-fraction");
  input_->clear();handleAction("pi");handleAction("pi");handleAction("pi");evaluate();idle();shot("exact-pi-cube");
  input_->setText("6÷5");evaluate();idle();shot("exact-terminating-decimal");
  input_->setText("(6)/(5)");evaluate();idle();shot("fraction-template-result");
  input_->setText("sqrt(2)+3÷4+sin(pi/6)");evaluate();idle();shot("exact-simplified-expression");
  input_->clear();handleAction("@mixedFraction");handleAction("6");input_->focusNextSlot();handleAction("2");input_->focusNextSlot();handleAction("3");evaluate();idle();shot("mixed-fraction-entry");mixedFractionsBox_->setCurrentIndex(1);idle();shot("mixed-fraction-result");mixedFractionsBox_->setCurrentIndex(0);idle();
  continuousEvaluationBox_->setCurrentIndex(0);handleAction("@clear");shot("cleared-display");
  input_->setText("7");evaluate();idle();continuousEvaluationBox_->setCurrentIndex(1);input_->setText("sin(□)");previewIdle();shot("empty-preview");continuousEvaluationBox_->setCurrentIndex(0);
  handleAction("@clear");complexBox_->setCurrentIndex(1);shot("complex-switch-label");
  for(int radix:{2,8,16}){baseBox_->setCurrentIndex(baseBox_->findData(radix));idle();handleAction("@clear");shot(radix==2?"binary-keypad":radix==8?"octal-keypad":"hex-keypad");}
  baseBox_->setCurrentIndex(baseBox_->findData(10));idle();input_->setText("FF");evaluate();idle();shot("decimal-hex-input");
  for(const auto& pair:QList<QPair<int,QString>>{{16,"FF.CA+0.36"},{2,"10.01+0.11"},{8,"7.4+0.4"}}){baseBox_->setCurrentIndex(baseBox_->findData(pair.first));idle();input_->setText(pair.second);evaluate();idle();shot(pair.first==16?"hex-fraction":pair.first==2?"binary-fraction":"octal-fraction");}
  baseBox_->setCurrentIndex(baseBox_->findData(10));idle();exactBox_->setCurrentIndex(1);idle();input_->setText("FF.CA");evaluate();idle();shot("decimal-hex-fraction");exactBox_->setCurrentIndex(0);idle();
  handleAction("@clear");input_->setFocus();physicalKey(QEvent::KeyPress,Qt::Key_Shift,Qt::ShiftModifier);shot("held-shift");keyStroke(Qt::Key_3,Qt::ShiftModifier,"#");physicalKey(QEvent::KeyRelease,Qt::Key_Shift);handleAction("@clear");
  changeBase(10);idle();exactBox_->setCurrentIndex(1);idle();
  for(const auto& fixture:QList<QPair<QString,QString>>{{"1/3","recurring-third"},{"1/6","recurring-sixth"},{"1/11","recurring-eleventh"},{"20/3","recurring-mixed"}}){input_->setText(fixture.first);evaluate();idle();shot(fixture.second);}
  showPage(1);shot("recurring-history");showPage(0);input_->setText("1/6");evaluate();idle();exactBox_->setCurrentIndex(0);idle();shot("recurring-secondary");
  changeBase(2);idle();input_->setText("1/11");exactBox_->setCurrentIndex(1);idle();evaluate();idle();shot("recurring-binary");changeBase(10);idle();exactBox_->setCurrentIndex(0);idle();complexBox_->setCurrentIndex(0);
  handleAction("@clear");input_->setText("5");input_->end();input_->setFocus();physicalKey(QEvent::KeyPress,Qt::Key_Shift,Qt::ShiftModifier);keyStroke(Qt::Key_4,Qt::ShiftModifier,"$");evaluate();idle();shot("held-factorial");physicalKey(QEvent::KeyRelease,Qt::Key_Shift);complexBox_->setCurrentIndex(1);
  workbenchHub_->graphs->clearGraphs();showGraph();shot("blank-graph");
  switchMode("Desktop");resize(960,760);showPage(3);shot("desktop-settings");switchMode("Calculator");showPage(0);
 }
 switchMode("Calculator");switchSkin("Deep Ocean");switchKeys("Expanded");resize(360,700);layout_="Calculator right";theme_="Ocean";appearance_="Dark";baseBox_->setCurrentIndex(baseBox_->findData(16));idle();notationBox_->setCurrentIndex(2);idle();complexEnabled_=true;calculator_->setComplexEnabled(true);
 inputStyleBox_->setCurrentIndex(1);groupingBox_->setCurrentIndex(1);decimalSeparatorBox_->setCurrentIndex(1);desktopEffectsBox_->setCurrentIndex(2);
 mixedFractionsBox_->setCurrentIndex(1);idle();continuousEvaluationBox_->setCurrentIndex(1);previewIdle();
 check(saveSettings(),"atomic settings and compressed history save");QFile file(configPath_);check(file.open(QIODevice::ReadOnly),"config reopens");auto saved=QJsonDocument::fromJson(file.readAll()).object();file.close();check(saved["mode"]=="Calculator"&&saved["version"]=="1.0.0"&&saved["mixedFractions"].toBool()&&saved["continuousEvaluation"].toBool()&&saved["calculatorTheme"]=="Deep Ocean"&&saved["calculatorLayout"]=="Expanded"&&!saved.contains("history")&&saved["numberBase"]==16&&!saved["structuredInput"].toBool(true)&&saved["digitGrouping"]==1&&saved["decimalSeparator"]==","&&saved["desktopEffects"]==2,"Preferences saved without uncompressed history");QFile archive(historyArchive_.filePath());check(archive.open(QIODevice::ReadOnly)&&!archive.readAll().isEmpty()&&history_.size()>=2,"compressed history exists separately");archive.close();check(!windowIcon().isNull(),"application icon loaded");
 QProcess restarted;restarted.start(QCoreApplication::applicationFilePath(),{"--verify-settings","--config-root",QFileInfo(configPath_).absolutePath()});check(restarted.waitForFinished(15000)&&restarted.exitCode()==0,"fresh process restores mode skin bases geometry and compressed history");
 // Optional settings fields restore their documented defaults independently.
 for(int omitted=0;omitted<16;++omitted){
  QTemporaryDir profileDir;auto partial=saved;
  for(const QString key:{"continuousEvaluation","mixedFractions","keyTooltipsEnabled","screenTooltipsEnabled"})partial.remove(key);
  if(omitted&1)for(const QString key:{"structuredInput","digitGrouping","decimalSeparator","desktopEffects"})partial.remove(key);
  if(omitted&2)for(const QString key:{"calculatorLayout","numberBase","notation","inputDrafts","desktopGeometry","calculatorGeometry"})partial.remove(key);
  if(omitted&4)partial.remove("mode");
  if(omitted&8)partial["history"]=history_;else QFile::copy(historyArchive_.filePath(),profileDir.filePath("history.qz"));
  QSaveFile fixture(profileDir.filePath("settings.json"));bool written=fixture.open(QIODevice::WriteOnly);if(written){fixture.write(QJsonDocument(partial).toJson());written=fixture.commit();}
  QProcess reopened;reopened.start(QCoreApplication::applicationFilePath(),{"--verify-settings","--config-root",profileDir.path()});bool restored=written&&reopened.waitForFinished(15000)&&reopened.exitCode()==0&&QFileInfo::exists(profileDir.filePath("history.qz"));
  if(!restored)qWarning().noquote()<<"OPTIONAL SETTINGS"<<omitted<<reopened.readAllStandardError();check(restored,"optional fields and inline history restore in a fresh process");
 }
 // Every explicitly saved graph survives a profile round trip unchanged.
 for(const QJsonArray functions:{QJsonArray{"sin(x)","cos(x)",""},QJsonArray{"x^2+5","",""}}){
  QTemporaryDir profileDir;auto settings=saved;
  const QJsonObject state{{"functions",functions},{"xMin",-10},{"xMax",10},{"yMin",-6},{"yMax",6},{"table",false}};
  settings["graphState"]=state;settings["graphs"]=functions;
  QFile::copy(historyArchive_.filePath(),profileDir.filePath("history.qz"));
  QSaveFile fixture(profileDir.filePath("settings.json"));bool written=fixture.open(QIODevice::WriteOnly);if(written){fixture.write(QJsonDocument(settings).toJson());written=fixture.commit();}
  QProcess reopened;reopened.start(QCoreApplication::applicationFilePath(),{"--verify-settings","--config-root",profileDir.path()});bool pass=written&&reopened.waitForFinished(15000)&&reopened.exitCode()==0;
  QFile result(profileDir.filePath("settings.json"));pass&=result.open(QIODevice::ReadOnly);const auto after=QJsonDocument::fromJson(result.readAll()).object();
  pass&=after["graphState"].toObject()==state;check(pass,"explicitly saved graph survives a profile round trip unchanged");
 }
 showPage(0);showFullScreen();QApplication::processEvents();check(isFullScreen()&&fullClient(),"Calculator fullscreen fills client area");showNormal();check(!isFullScreen(),"fullscreen exit");qInfo()<<"SMOKE COMPLETE"<<ok<<checks<<"checks";return ok;
}
bool MainWindow::verifyLoadedSettings(){
 QString expectedMode=loadedSettings_.value("mode").toString(loadedSettings_.value("design").toString("Desktop"));QString expectedSkin=loadedSettings_.value("calculatorTheme").toString(expectedMode=="Modern LCD"?"Modern Light":"Classic");if(expectedMode=="Classic LCD"||expectedMode=="Modern LCD")expectedMode="Calculator";
 bool ok=screenTooltipsEnabled_==loadedSettings_.value("screenTooltipsEnabled").toBool(true)&&screenTooltipsBox_->currentIndex()==(screenTooltipsEnabled_?0:1)&&calculator_->screenTooltipsEnabled()==screenTooltipsEnabled_&&keyTooltipsEnabled_==loadedSettings_.value("keyTooltipsEnabled").toBool(true)&&keyTooltipsBox_->currentIndex()==(keyTooltipsEnabled_?0:1)&&calculator_->keyTooltipsEnabled()==keyTooltipsEnabled_&&mixedFractions_==loadedSettings_.value("mixedFractions").toBool(false)&&mixedFractionsBox_->currentIndex()==(mixedFractions_?1:0)&&continuousEvaluation_==loadedSettings_.value("continuousEvaluation").toBool(false)&&continuousEvaluationBox_->currentIndex()==(continuousEvaluation_?1:0)&&!loadedSettings_.isEmpty()&&design_==expectedMode&&designBox_->currentData().toString()==design_&&calculatorTheme_==expectedSkin&&history_.size()>=2&&!memoryValue_.isEmpty()&&graph_->saveState()==loadedSettings_["graphState"].toObject()&&handheld_==(design_=="Calculator")&&minimumWidth()==(handheld_?320:680);
 ok&=calculatorLayout_==loadedSettings_.value("calculatorLayout").toString("Compact")&&numberBase_==loadedSettings_.value("numberBase").toInt(10)&&notation_==loadedSettings_.value("notation").toInt(0)&&width()==360&&height()==700;
 ok&=structuredInput_==loadedSettings_.value("structuredInput").toBool(true)&&digitGrouping_==loadedSettings_.value("digitGrouping").toInt(0)&&decimalSeparator_==(loadedSettings_.value("decimalSeparator").toString(".")==","?',':'.')&&desktopEffects_==loadedSettings_.value("desktopEffects").toInt(1);
 if(!ok)qWarning()<<"RELOAD STATE"<<design_<<expectedMode<<calculatorTheme_<<expectedSkin<<history_.size()<<memoryValue_<<handheld_<<minimumWidth()<<numberBase_<<calculatorLayout_<<(graph_->saveState()==loadedSettings_["graphState"].toObject());ok&=saveSettings();ok&=!historyArchive_.needsMigration();qInfo()<<"SETTINGS RELOAD"<<ok;return ok;
}

void MainWindow::refreshFunctionMetadata(){dispatch([this](CalcEngine*engine){const auto metadata=engine->functionMetadata();const auto entries=engine->catalog();QMetaObject::invokeMethod(this,[this,metadata,entries]{if(!closing_)applyFunctionMetadata(metadata,entries);},Qt::QueuedConnection);});}
void MainWindow::applyFunctionMetadata(const QVector<FunctionMetadata>&metadata,const QVector<CatalogEntry>&entries){catalogTools_->setCatalog(entries);MathEditor::setDefaultFunctionMetadata(metadata);}

std::shared_ptr<QLockFile> MainWindow::lockProfile(){
 if(auto active=profileTransaction_.lock())return active;
 const QString directory=QFileInfo(configPath_).absolutePath();QDir().mkpath(directory);
 auto lock=std::make_shared<QLockFile>(QDir(directory).filePath("desktop-profile.lock"));lock->setStaleLockTime(0);
 if(!lock->tryLock(0)){if(status_)setStatus(trc("Another window is saving this profile. Try again shortly."));if(saveTimer_)saveTimer_->start(1000);return {};}
 // The next writer may be an already-open window after a crashed peer.
 // Recover before every new transaction, not only at process startup, so no
 // successful later write can leave an older pending journal behind it.
 QString recoveryError;if(!WorkspaceJournal::recover(directory,&recoveryError)){configSaveBlocked_=true;restoreNotice_=recoveryError;if(status_)setStatus(recoveryError);return {};}
 profileTransaction_=lock;return lock;
}
