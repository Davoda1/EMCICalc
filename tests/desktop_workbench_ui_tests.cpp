// SPDX-License-Identifier: MIT
#include "workspacehub.h"
#include "desktopworkbench.h"
#include "workbenchviews.h"
#include "workbenchpalette.h"
#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QTest>
#include <iostream>
#include <stdexcept>

namespace {
int checks=0;
void check(bool value,const char *message){++checks;if(!value)throw std::runtime_error(message);}
void settle(){QApplication::processEvents();QCoreApplication::sendPostedEvents(nullptr,QEvent::LayoutRequest);QApplication::processEvents();}
QComboBox *choice(QWidget &w,const char *id){auto *c=w.findChild<QComboBox*>(id);check(c,id);return c;}
QPushButton *button(QWidget &w,const char *id){auto *c=w.findChild<QPushButton*>(id);check(c,id);return c;}
}
int main(int argc,char **argv){QApplication app(argc,argv);try{
 WorkspaceHub hub;hub.resize(680,540);hub.show();settle();
 check(hub.graphs->result().series.isEmpty(),"fresh graph has no sample content");
 const auto baseline=hub.saveState();
 auto *matrixOperation=choice(*hub.matrix,"matrixOperation");
 WorkbenchData::MatrixRequest matrixRequest;hub.matrix->onCompute=[&](const auto &r){matrixRequest=r;};
 hub.open("matrix");settle();
 auto *source=hub.matrix->findChild<QSpinBox*>("matrixSourceIndex");auto *target=hub.matrix->findChild<QSpinBox*>("matrixTargetIndex");auto *factor=hub.matrix->findChild<QLineEdit*>("matrixScaleFactor");
 check(source&&target&&factor,"matrix row operation inputs available");
 target->setValue(2);source->setValue(3);factor->setText("7/3");
 matrixOperation->setCurrentIndex(matrixOperation->findData("addrow"));button(*hub.matrix,"matrixCompute")->click();
 check(matrixRequest.row==2&&matrixRequest.column==3&&matrixRequest.parameter=="7/3","add-scaled row submits target source and exact factor");
 hub.matrix->cancel();matrixOperation->setCurrentIndex(matrixOperation->findData("scalerow"));button(*hub.matrix,"matrixCompute")->click();
 check(matrixRequest.row==2&&matrixRequest.column==1&&matrixRequest.parameter=="7/3"&&matrixRequest.b.isEmpty(),"scale omits irrelevant source and second matrix");
 hub.matrix->cancel();matrixOperation->setCurrentIndex(matrixOperation->findData("det"));button(*hub.matrix,"matrixCompute")->click();
 check(matrixRequest.parameter.isEmpty()&&matrixRequest.row==1,"determinant omits row-operation draft fields");hub.matrix->cancel();
 auto saved=hub.matrix->saveState();MatrixWorkbench restored;QString error;check(restored.restoreState(saved,&error),"matrix draft restores");check(restored.saveState()==saved,"matrix hidden draft fields round-trip unchanged");
 WorkbenchData::DataRequest dataRequest;hub.data->onCompute=[&](const auto&r){dataRequest=r;};auto *prediction=hub.data->findChild<QLineEdit*>("dataPrediction");check(prediction,"prediction draft exists");prediction->setText("invalid(");hub.open("data");button(*hub.data,"dataCompute")->click();check(dataRequest.model=="summary"&&dataRequest.prediction.isEmpty(),"summary omits hidden prediction");hub.data->cancel();
 auto *table=hub.data->findChild<QTableWidget*>("datasetGrid");check(table,"dataset native table exists");hub.data->findChild<QTabWidget*>("dataPages")->setCurrentIndex(0);settle();table->setCurrentCell(0,0);
 // Native delegate changes commit before Calculate reads the grid.
 if(!table->item(0,0))table->setItem(0,0,new QTableWidgetItem);
 table->editItem(table->item(0,0));settle();if(auto *edit=table->findChild<QLineEdit*>()){edit->setText("123.25");QTest::keyClick(edit,Qt::Key_Return);settle();}
 button(*hub.data,"dataCompute")->click();check(dataRequest.rows[0][0]=="123.25","dataset calculation includes active cell edit");hub.data->cancel();
 auto *guidedOperation=choice(*hub.guided,"guidedOperation");auto *expression=hub.guided->findChild<QLineEdit*>("guidedExpression");auto *variable=hub.guided->findChild<QLineEdit*>("guidedVariable");QString inspected;hub.guided->onInspect=[&](QString e,std::shared_ptr<const CalcContextSnapshot>,ResultCallback cb){inspected=e;CalcResult r;r.ok=true;r.result="2";r.serialized="2";cb(r);};
 variable->setText("invalid variable!");expression->setText("1+1");guidedOperation->setCurrentIndex(guidedOperation->findData("simplify"));button(*hub.guided,"guidedCompute")->click();check(inspected=="simplify(1+1)","variable-free simplify ignores hidden variable");
 hub.definitions->findChild<QLineEdit*>("definitionName")->setText("draft_test");hub.definitions->findChild<QPlainTextEdit*>("definitionBody")->setPlainText("incomplete(");hub.steps->setExpression("sin(");auto draftState=hub.saveState();WorkspaceHub draftCopy;check(draftCopy.restoreState(draftState,&error),"full workspace draft imports");check(draftCopy.definitions->saveDraft()==hub.definitions->saveDraft(),"unfinished definition is preserved");check(draftCopy.steps->saveDraft()==hub.steps->saveDraft(),"unfinished Steps expression is preserved");
 auto invalid=draftState;auto badProgrammer=invalid.value("programmer").toObject();badProgrammer.insert("width",99);invalid.insert("programmer",badProgrammer);auto badMatrix=invalid.value("matrix").toObject();badMatrix.insert("a",QJsonArray{});invalid.insert("matrix",badMatrix);auto before=draftCopy.saveState();check(!draftCopy.restoreState(invalid,&error),"invalid workspace rejected");check(draftCopy.saveState()==before,"invalid import leaves every live component unchanged");
 auto keypadDraft=hub.keypad->saveState();auto portrait=keypadDraft.value("portrait").toObject();auto keys=portrait.value("keys").toArray();keys.removeAt(0);portrait.insert("keys",keys);keypadDraft.insert("portrait",portrait);KeypadDesigner incomplete;check(incomplete.restoreState(keypadDraft,&error),"incomplete keypad draft restores without activating");check(!CustomKeypad::validate(incomplete.layoutDefinition()).isEmpty(),"incomplete keypad remains invalid for Apply");
 auto *matrixA=hub.matrix->findChild<QTableWidget*>("matrixGridA");auto *matrixRows=hub.matrix->findChild<QSpinBox*>("matrixRowsA");matrixA->setItem(1,1,new QTableWidgetItem("incomplete("));matrixRows->setValue(1);check(matrixA->rowCount()==2&&matrixRows->value()==2&&matrixA->item(1,1)->text()=="incomplete(","matrix shrink preserves nonempty draft cells");matrixA->item(1,1)->setText("");matrixRows->setValue(1);check(matrixA->rowCount()==1,"matrix shrink accepts empty removed rows");matrixRows->setValue(2);
 auto *dataRows=hub.data->findChild<QSpinBox*>("dataRows");table->setItem(2,2,new QTableWidgetItem("7"));dataRows->setValue(2);check(table->rowCount()==3&&dataRows->value()==3&&table->item(2,2)->text()=="7","dataset shrink preserves hidden and frequency cells");
 // Pure navigation changes do not rebuild an existing page; focus/drafts survive.
 auto *identity=hub.guided;QElapsedTimer elapsed;elapsed.start();for(int i=0;i<100;++i){hub.open("matrix");hub.open("data");hub.open("guided");}check(hub.guided==identity&&expression->text()=="1+1","navigation preserves page identity and draft");std::cout<<"300 workspace transitions: "<<elapsed.elapsed()<<" ms\n";
 hub.matrix->findChild<QTabWidget*>("matrixPages")->setCurrentIndex(0);hub.data->findChild<QTabWidget*>("dataPages")->setCurrentIndex(0);
 const QString captureDir=argc>1?QString::fromLocal8Bit(argv[1]):QString();if(!captureDir.isEmpty())QDir().mkpath(captureDir);
 for(const auto &size:{QSize(320,540),QSize(680,540),QSize(1200,700),QSize(420,900)}){
  hub.resize(size);for(const auto *page:{"home","matrix","data","guided","graphs","steps","definitions","programmer","keypad","workflows"}){hub.open(page);settle();check(hub.width()==size.width()&&hub.height()==size.height(),"workbench stays within requested window without global resizing");if(!captureDir.isEmpty()&&(size.width()==320||size.width()==680))hub.grab().save(captureDir+QString("/desktop-%1-%2x%3.png").arg(page).arg(size.width()).arg(size.height()));}
 }
 for(const auto &skin:calculatorThemeNames()){DesktopWorkbench::apply(&hub,calculatorWorkbenchPalette(skin));hub.open("matrix");settle();check(hub.palette().color(QPalette::Window)==calculatorWorkbenchPalette(skin).color(QPalette::Window),"workbench follows selected calculator skin palette");const auto palette=qvariant_cast<QPalette>(hub.property("twPalette"));check(EditCue::contrast(palette.color(QPalette::Link),palette.color(QPalette::Window))>=4.5,"workbench active tabs use readable theme accent on calculator body");const auto colors=DesktopWorkbench::colors(palette);check(EditCue::contrast(colors.surfaceLink,colors.surface)>=4.5,"accent mathematical captions remain readable on content surface");}
 for(qreal scale:{.9,1.,1.2}){QFont f=app.font();f.setPointSizeF(f.pointSizeF()*scale);hub.setFont(f);hub.resize(320,540);hub.open("matrix");settle();check(hub.size()==QSize(320,540),"font choice preserves compact local scrolling");}
 std::cout<<checks<<" checks passed\n";return 0;
 }catch(const std::exception&e){std::cerr<<"FAIL after "<<checks<<" checks: "<<e.what()<<'\n';return 1;}}
