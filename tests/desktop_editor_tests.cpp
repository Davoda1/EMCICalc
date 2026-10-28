// SPDX-License-Identifier: MIT
// Native desktop component tests. Synthetic input is not a clean-machine gate.
#include "engine_metadata_fixture.h"
#include "calculatorpanel.h"
#include "calculatortools.h"
#include "financepanel.h"
#include "fontscale.h"
#include "keyartwork.h"
#include "mathcaption.h"
#include "inwindowdialog.h"
#include <QtWidgets>
#include <iostream>

namespace {
void press(QWidget *w,int key,Qt::KeyboardModifiers mods=Qt::NoModifier,QString text={}){
 QKeyEvent down(QEvent::KeyPress,key,mods,text);QApplication::sendEvent(w,&down);
 QKeyEvent up(QEvent::KeyRelease,key,mods,text);QApplication::sendEvent(w,&up);
}
QJsonObject sampleDraft(const Finance::Tool &tool,int variant){
 QJsonObject result;for(const auto &field:tool.fields){
  if(field.type=="table"){QJsonObject row;for(const auto &column:field.columns)row[column.key]=column.type=="bool"?QJsonValue(false):QJsonValue(variant==0?QString():QString(200,'9')+"+");result[field.key]=QJsonArray{row};}
  else if(field.type=="bool")result[field.key]=false;
  else if(field.type=="choice")result[field.key]=field.choices.value(0);
  else result[field.key]=variant==0?QString():variant==1?QStringLiteral("1+"):QString(256,'9');
 }return result;
}
}
int main(int argc,char **argv){
 QApplication app(argc,argv);app.setStyle("Fusion");DesktopFont::initialize(app);installEngineMetadataFixture();
 QString output=argc>1?QString::fromLocal8Bit(argv[1]):QStringLiteral("desktop-editor-evidence");QDir().mkpath(output);
 int failures=0;QJsonArray checks;auto check=[&](bool pass,const QString &name){checks.append(QJsonObject{{"name",name},{"passed",pass}});if(!pass)++failures;std::cout<<(pass?"PASS ":"FAIL ")<<name.toStdString()<<'\n';};
 MathEditor editor;editor.resize(420,220);editor.show();editor.setFocus();app.processEvents();
 editor.insertTemplate("rand(");
 check(editor.text()=="rand()"&&editor.slotRects().isEmpty()&&editor.isComplete(),"Nullary engine function has no mandatory input slot");
 editor.clear();editor.insertTemplate("unregistered_function(");check(editor.text()=="unregistered_function()"&&editor.slotRects().isEmpty()&&editor.functionArgumentHelp().contains("unavailable"),"Unknown metadata stays explicit and never guessed unary");
 editor.clear();editor.insertTemplate("loanpayment(");check(editor.slotRects().size()==3,"Finance function required arity comes from shipped engine");
  QKeyEvent argumentShortcut(QEvent::ShortcutOverride,Qt::Key_F1,Qt::NoModifier);argumentShortcut.ignore();QApplication::sendEvent(&editor,&argumentShortcut);check(argumentShortcut.isAccepted(),"Function F1 guidance takes precedence over the global manual shortcut");
 editor.insert("1200");editor.focusArgument(1);editor.insert("0.01");editor.focusArgument(2);editor.insert("12");
 check(editor.addFunctionArgument()&&editor.text().endsWith(QString::fromUtf8(",□)")),"Later optional Finance parameter is explicit");editor.insert("1");
 check(editor.removeLastFunctionArgument()&&editor.text()=="loanpayment(1200,0.01,12)"&&!editor.removeLastFunctionArgument(),"Optional argument deletion preserves exact true minimum");
 editor.setText("f(\"a,b\",[1,2;3,4],g(4,5))");editor.setCursorPosition(3);check(editor.argumentContext().ranges.size()==3,"Quoted, matrix and nested separators preserve outer argument identity");
 editor.setText("pi+sqrt(4)");editor.setCursorPosition(2);press(&editor,Qt::Key_Left,Qt::ShiftModifier);check(editor.selectedText()=="pi","Keyboard range extension respects atomic constant boundary");
 press(&editor,Qt::Key_C,Qt::ControlModifier);check(app.clipboard()->text()=="pi","Native keyboard selection copies exact canonical source");
 press(&editor,Qt::Key_X,Qt::ControlModifier);editor.undo();check(editor.text()=="pi+sqrt(4)","Cut and Undo preserve the structured source");
 const auto menuLabels=[&](){QContextMenuEvent e(QContextMenuEvent::Keyboard,editor.rect().center(),editor.mapToGlobal(editor.rect().center()));QApplication::sendEvent(&editor,&e);app.processEvents();QStringList labels;for(auto*m:editor.findChildren<QMenu*>())if(m->isVisible()){for(auto*a:m->actions())labels<<a->text();m->close();}return labels;};
 auto labels=menuLabels();check(labels.contains("Copy")&&labels.contains("Cut")&&labels.contains("Paste")&&labels.contains("Undo")&&labels.contains("Redo"),"Editable source retains native QMenu editing commands");
 editor.setReadOnly(true);labels=menuLabels();check(labels.contains("Copy")&&!labels.contains("Cut")&&!labels.contains("Paste")&&!labels.contains("Undo"),"Read-only result QMenu offers copy and omits mutations");editor.setReadOnly(false);
 for(const QString &call:QStringList{"log(□,□)","diff(□,□,□,□)","integrate(□,□,□,□)","sum(□,□,□,□)","product(□,□,□,□)"}){
  editor.setText(call);editor.home();app.processEvents();const auto rectangles=editor.slotRects();bool all=rectangles.size()==call.count(QChar(0x25a1));int last=-1;for(const auto&r:rectangles){last=call.indexOf(QChar(0x25a1),last+1);all&=editor.sourceOffsetAtPoint(r.center())==last;}
  check(all,"Natural notation slot hit testing preserves canonical order: "+call);
  editor.setStructuredInput(false);check(editor.text()==call,"Linear view preserves identical canonical source: "+call);editor.setStructuredInput(true);
  QString name=call.left(call.indexOf('('));check(editor.grab().save(output+"/notation-"+name+".png"),"Native notation capture: "+name);
 }
 {
  MathEditor themed;themed.resize(600,220);themed.setText("integrate(x^2,0,1,x)+log(8,2)");themed.setSelection(10,3);
  const auto bounds=themed.naturalSize();const auto selection=themed.selectedText();const auto caret=themed.cursorRect();
  auto colors=themed.palette();colors.setColor(QPalette::Text,Qt::white);colors.setColor(QPalette::Base,Qt::black);themed.setPalette(colors);
  themed.setStyleSheet("background-color: #202020; color: #eeeeee;");
  check(themed.naturalSize()==bounds&&themed.cursorRect()==caret&&themed.selectedText()==selection,"Palette and color-only stylesheet preserve math geometry and selected source");
  auto larger=themed.font();larger.setPixelSize(35);themed.setFont(larger);
  check(themed.naturalSize().height()>bounds.height()&&themed.selectedText()==selection,"Actual font change rebuilds natural notation while retaining selection");
  themed.setStyleSheet("font-size: 45px; color: #eeeeee;");themed.ensurePolished();
  check(themed.font().pixelSize()==45&&themed.naturalSize().height()>bounds.height(),"Stylesheet font changes still rebuild mathematical layout");
 }
 editor.clear();editor.setStructuredInput(false);editor.insertTemplate("sin(");check(editor.text()=="sin("&&editor.slotRects().isEmpty(),"Desktop Linear entry retains raw function opening workflow");editor.setStructuredInput(true);
 const QFont normal=editor.font();DesktopFont::setSizeMode("Large");editor.setFont(DesktopFont::font(24,normal));editor.setProperty("preserveMathScale",true);editor.setReadOnly(true);editor.setResultPresentation(true);editor.resize(160,90);editor.setText(QString(80,'9'));app.processEvents();const auto start=editor.sourceCharacterRect(0);
 QWheelEvent wheel(QPointF(80,40),editor.mapToGlobal(QPoint(80,40)),QPoint(),QPoint(0,-120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);QApplication::sendEvent(&editor,&wheel);
 check(editor.sourceCharacterRect(0).x()<start.x()&&editor.text()==QString(80,'9'),"Enlarged result pans locally without shrinking or modifying value");DesktopFont::setSizeMode("Medium");
 CalculatorPanel panel;panel.resize(320,540);panel.setDesign("Calculator");panel.show();app.processEvents();
 const auto key=[&](const QString &token){for(auto*k:panel.keyButtons())if(k->primaryToken()==token)return k;return static_cast<CalculatorKey*>(nullptr);};
 for(const QString &theme:calculatorThemeNames()){
  panel.setCalculatorTheme(theme);app.processEvents();auto *left=key("@left"),*right=key("@right"),*back=key("@del"),*root=key("sqrt(");
  check(left&&right&&back&&root&&!KeyArtwork::path("arrow-left").isEmpty()&&!KeyArtwork::path("backspace").isEmpty(),"Shared vector navigation artwork exists: "+theme);
  if(left&&right&&root){check(left->toolTip().count("data-key-artwork=\"arrow-left\"")==1&&right->toolTip().count("data-key-artwork=\"arrow-right\"")==1,"HOME and END help stay plain text: "+theme);check(root->toolTip().contains("data-key-artwork=\"cube-root\""),"Cube-root help uses font-independent vector: "+theme);}
 }
 for(qreal ratio:{1.,1.25,1.5,2.})for(const QString&token:QStringList{"integrate(","sum(","product(","log(","diff("})check(!MathCaption::imageHtml(token,token,Qt::black,14,ratio).isEmpty(),"Vector math caption rasterizes at fractional DPR "+QString::number(ratio)+" "+token);
 if(auto *helpKey=key("sqrt(")){QKeyEvent shortcut(QEvent::ShortcutOverride,Qt::Key_F1,Qt::NoModifier);shortcut.ignore();QApplication::sendEvent(helpKey,&shortcut);check(shortcut.isAccepted(),"Focused calculator keys expose bounded F1 tooltip help");}
 CalculatorTools catalog;auto *catalogList=catalog.findChild<QListWidget*>();if(catalogList){QKeyEvent shortcut(QEvent::ShortcutOverride,Qt::Key_F1,Qt::NoModifier);shortcut.ignore();QApplication::sendEvent(catalogList,&shortcut);check(shortcut.isAccepted(),"Function catalog F1 guidance takes precedence over the global manual shortcut");}
 auto custom=CustomKeypad::editableCopy(CustomKeypad::recoveryLayout());QString error;check(panel.setCustomLayout(custom,&error),"Validated semantic custom keypad applies: "+error);
 for(const QSize size:{QSize(320,540),QSize(900,540)}){panel.resize(size);app.processEvents();bool inside=true,overlap=false;const auto keys=panel.keyButtons();for(int i=0;i<keys.size();++i){inside&=panel.rect().contains(keys[i]->geometry());for(int j=0;j<i;++j)overlap|=keys[i]->geometry().intersects(keys[j]->geometry());}check(inside&&!overlap,"Custom keypad profile geometry is bounded without overlaps "+QString::number(size.width()));}
 panel.setShiftState(true,false);auto *escape=key("@modifierExit");if(escape)escape->click();check(escape&&!panel.effectiveShift()&&!panel.effectiveAlpha(),"Custom modifier escape remains reachable and clears layer");panel.clearCustomLayout();
 panel.setDesign("Calculator");panel.setCalculatorTheme("Classic");panel.resize(320,540);panel.editor()->setText("log(□,□)");panel.editor()->setCursorPosition(4);panel.editor()->focusArgument(0);app.processEvents();check(panel.grab().save(output+"/calculator-natural-320.png"),"Native Calculator minimum-size argument-guide capture");
 panel.setDesign("Desktop");panel.resize(680,600);app.processEvents();check(panel.grab().save(output+"/desktop-natural.png"),"Native Desktop argument-guide capture");
 FinancePanel finance;finance.resize(680,540);finance.show();app.processEvents();const auto tools=Finance::tools();auto*selector=finance.findChild<QComboBox*>("financeTool");auto*category=finance.findChild<QComboBox*>("financeCategory");check(tools.size()==29,"All 29 Finance worksheets are available");
 qint64 maximum=0,total=0;int transitions=0;QHash<QString,QJsonObject> drafts;
 for(int variant=0;variant<4;++variant)for(const auto &tool:tools){finance.setTool(tool.id);auto draft=sampleDraft(tool,variant);finance.setInput(draft);drafts[tool.id]=finance.inputSnapshot();
  if(variant==1)selector->setFocus();else if(variant>=2){auto inputs=finance.findChildren<QLineEdit*>();if(!inputs.isEmpty())inputs.first()->setFocus();}else finance.setFocus();
  QElapsedTimer timer;timer.start();finance.setTool(tools.first().id);app.processEvents();finance.setTool(tool.id);app.processEvents();const auto elapsed=timer.elapsed();maximum=qMax(maximum,elapsed);total+=elapsed;++transitions;
  check(finance.inputSnapshot()==drafts.value(tool.id),"Finance forward/reverse switch preserves draft and focus case "+QString::number(variant)+": "+tool.id);
 }
 const auto state=finance.saveState();FinancePanel restored;restored.restoreState(state);check(restored.saveState()==state,"Finance worksheet identity and every draft survive serialized restoration");
 for(int i=category->count()-1;i>=0;--i){category->setCurrentIndex(i);app.processEvents();check(!finance.toolId().isEmpty()&&selector->count()>0,"Finance reverse category selector transition "+QString::number(i));}
 check(finance.grab().save(output+"/finance-native.png"),"Native Finance layout capture");
 finance.setTool("loansummary");if(auto *load=finance.findChild<QPushButton*>("financeLoadExample"))load->click();app.processEvents();
 check(finance.grab().save(output+"/finance-manual-example.png"),"Native Finance manual example explicitly loaded by documentation fixture");
 panel.setDesign("Desktop");panel.resize(680,600);panel.editor()->setText("integrate(x^2,0,1,x)");panel.editor()->home();app.processEvents();
 check(panel.grab().save(output+"/desktop-filled-integral.png"),"Native filled integral editing capture without automatic evaluation");
 panel.activateWindow();panel.editor()->setFocus();panel.editor()->setCursorPosition(10);panel.editor()->focusArgument(0);app.processEvents();
 bool helpCaptured=false;QTimer::singleShot(20,[&]{if(auto *dialog=InWindowDialog::active()){
  check(!dialog->isWindow()&&dialog->window()==panel.window()&&dialog->parentWidget()->rect().contains(dialog->geometry()),"Function help remains a bounded child inside the application window");
  auto *heading=dialog->findChild<QLabel*>("inWindowDialogTitle");check(heading&&heading->isVisible()&&!heading->text().isEmpty(),"In-window tool title remains visible and accessible");
  const QString source=panel.editor()->text();press(panel.editor(),Qt::Key_9,Qt::NoModifier,"9");check(panel.editor()->text()==source,"In-window help blocks underlying editor shortcuts and input");
  helpCaptured=panel.grab().save(output+"/function-arguments-manual.png");
  panel.resize(320,540);app.processEvents();check(dialog->parentWidget()->rect().contains(dialog->geometry()),"In-window help follows a narrowed parent without leaving client area");
  press(QApplication::focusWidget(),Qt::Key_Tab);app.processEvents();check(dialog->isAncestorOf(QApplication::focusWidget()),"In-window help contains native Tab navigation");
  QKeyEvent escapeOverride(QEvent::ShortcutOverride,Qt::Key_Escape,Qt::NoModifier);escapeOverride.ignore();QApplication::sendEvent(QApplication::focusWidget(),&escapeOverride);check(escapeOverride.isAccepted(),"In-window Escape takes precedence over the main-window navigation shortcut");
  press(QApplication::focusWidget(),Qt::Key_Escape);check(!dialog->isVisible(),"Escape closes the in-window help card");if(dialog->isVisible())dialog->reject();
 }});
 if(auto *help=panel.findChild<QPushButton*>("functionArgumentHelp"))help->click();check(helpCaptured,"Native function argument help dialog manual capture");
 QFile report(output+"/desktop-editor-results.json");if(report.open(QIODevice::WriteOnly))report.write(QJsonDocument(QJsonObject{{"classification","Native desktop component tests and synthetic desktop input; not clean-machine evidence"},{"checks",checks},{"failures",failures},{"financeTransitionPairs",transitions},{"financeMaximumPairMs",maximum},{"financeMeanPairMs",transitions?double(total)/transitions:0}}).toJson());
 std::cout<<checks.size()<<" checks, "<<failures<<" failures; maximum Finance forward/reverse pair "<<maximum<<" ms\n";return failures?1:0;
}
