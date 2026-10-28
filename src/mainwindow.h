// SPDX-License-Identifier: MIT
#pragma once
#include <QtWidgets>
#include <functional>
#include "engine.h"
#include "graphpanel.h"
#include "calculatorpanel.h"
#include "calculatorkeyboard.h"
#include "calculatormenu.h"
#include "historyarchive.h"
#include "registerarchive.h"
#include "registerspanel.h"
#include "matheditor.h"
#include "desktopsurface.h"
#include "calculatortools.h"
#include "currencypanel.h"
#include "exchangerates.h"
class WorkspaceHub;
class MainWindow : public QMainWindow {
 friend class DesktopWorkspaceIntegrationTests;
 friend class DesktopAcceptanceTests;
 friend class DesktopStorageTests;
 friend class DesktopSettingsPersistenceTests;
 friend class CalculatorFeaturesIntegrationTests;
 friend class ConverterIntegrationTests;
 friend class ConversionLayoutIntegrationTests;
 friend class RegisterHistoryIntegrationTests;
 friend class InputPresentationTests;
 friend class FinanceKeypadIntegrationTests;
 friend class ResultSettingsTests;
 friend class SettingsDialogTests;
public:
 explicit MainWindow(bool testMode=false);
 ~MainWindow() override;
 bool smokeTest(const QString& output);
 bool verifyLoadedSettings();
protected:
 void resizeEvent(QResizeEvent*) override;
 void closeEvent(QCloseEvent*) override;
private:
 void setupWorkspace(); void showWorkspace(QString id="home"); void inspectWorkspace(QString,std::shared_ptr<const CalcContextSnapshot>,std::function<void(CalcResult)>,QString group=QString()); void exportWorkspace(); void importWorkspace(); bool commitWorkbench(const CalcResult&,const CalcSessionState&);
 void refreshFunctionMetadata(); void applyFunctionMetadata(const QVector<FunctionMetadata>&,const QVector<CatalogEntry>&);
 QWidget *workspaceHost_=nullptr;QVBoxLayout *workspaceHostLayout_=nullptr;
 WorkspaceHub *workbenchHub_=nullptr; CalcResult selectedResult_; QMap<QString,std::shared_ptr<std::atomic_bool>> workspaceJobs_,workspaceCancelFlags_; QVector<CalcDefinition> definitions_;
 CalcStoredValue answerState_{QStringLiteral("0"),false};
 QString undefinedDisplayError_,fontSizeMode_="System";
 QComboBox *fontSizeBox_=nullptr,*percentModeBox_=nullptr,*implicitModeBox_=nullptr; QSpinBox *decimalPlacesBox_=nullptr;
 int decimalPlaces_=6,percentMode_=0,implicitMode_=0;
 std::weak_ptr<QLockFile> profileTransaction_; QByteArray settingsFingerprint_; QString settingsConflictPath_;
 std::shared_ptr<QLockFile> lockProfile();
 QString trc(const char*) const; QString calculationError(const QString&) const;
 QWidget *makeCalculator(), *makeHistory(), *makeToolbox(), *makeSettings(), *makeGuide(), *makeMenu();
 void refreshRegisters(); void clearRegisters(const QStringList&); void persistRegisters();
 void formatConversionResult(); void syncCurrencyRates(); void convertCurrency(QString,QString,QString);
 void setupWorker(); void setupTools(); void startTemplate(QString); void openMatrixTools(); void convertUnits(QString,QString,QString); void evaluate(); void reformatResult(); void setBusy(bool); void invalidateResult();
 void insert(QString); void handleAction(QString); void applyDesign(); void stepHistory(int); void copyResult(); void refreshKeys(); void memory(QString);
 void applyMathPreferences(); void setApproximation(const QString&,bool approximate=true,int repeatStart=-1,int repeatLength=0,const QString& copyValue={}); void refreshApproximation();
 void schedulePreview(); void startPreview(); void cancelPreview(); void clearResultDisplay(); void showCommittedResult(); void commitDisplay(const CalcResult&); void commitDisplay(const QString&,const QString&);
 void changeBase(int); void recallHistoryEntry(const QJsonObject&); void applyTheme(); void watchSystemTheme(); void updateTranslations(); void updateLayout();
 void loadSettings(); bool saveSettings(); void fillHistory(); void sizeHistoryRows();
 void beginSettingsDraft(); void discardSettingsDraft(); void applySettingsDraft();
 bool deferSettingsChange(); void syncSettingsControls(); void updateSettingsUi(); void updateSettingsPresentation(); QJsonObject settingsControlValues() const;
 void historyExport(); void setStatus(QString); void showPage(int,bool remember=true); void showGraph(); void goBack(); void menuAction(QString); void updateAuxiliaryLayouts(); void rememberWindowGeometry(); void restoreGeometrySafe();
 CalcOptions options() const;
 void dispatch(std::function<void(CalcEngine*)>);
 QStringList unitTokens_,constantTokens_; bool catalogReady_=false; int conversionGeneration_=0;
 int financeGeneration_=0;
 bool converterExact_=true; CalcResult lastConversion_; QString lastConversionTarget_;
 CalculatorTools *catalogTools_=nullptr,*converterTools_=nullptr;
 CurrencyPanel *currencyPanel_=nullptr; ExchangeRateService *exchangeRates_=nullptr;
 ExchangeRateSnapshot activeRatesSnapshot_,requestedRatesSnapshot_;
 int currencyConversionGeneration_=0,currencyRatesGeneration_=0; bool currencyRatesApplying_=false;
 QMap<QString,QString> unitLabels_;
 QString configPath_, language_="en", theme_="Graphite", appearance_="System", layout_="Adaptive",design_="Desktop",calculatorTheme_="Classic",calculatorLayout_="Compact",lastDir_, memoryValue_, lastSerialized_;
 bool lastValueApproximate_=false,memoryValueApproximate_=false;
 std::shared_ptr<std::atomic_bool> evaluationCommitAllowed_;
 int evaluationGeneration_=0,renderGeneration_=0; bool formatPending_=false; CalcOptions historyDraftOptions_; int historyCursor_=-1,historyDraftBase_=10; QString historyDraft_;
 bool continuousEvaluation_=false,previewRunning_=false,previewPending_=false,previewDisplayed_=false,previewDraft_=false; int previewGeneration_=0;
 bool committedResultApproximate_=false,committedApproximationApproximate_=true;
 CalcResult committedDisplayData_; QString committedResult_,committedApproximation_,displayCopyValue_; QTimer* previewTimer_=nullptr;
 bool mixedFractions_=false,keyTooltipsEnabled_=true,screenTooltipsEnabled_=true;
 bool structuredInput_=true; int digitGrouping_=0,desktopEffects_=1; QChar decimalSeparator_='.';
 int precision_=30,angle_=0,numberBase_=10,notation_=0; bool complexEnabled_=true; QMap<int,QString> baseDrafts_; QMap<int,QPair<int,int>> baseCursors_; bool restoringDraft_=false,baseResultDetached_=false; bool exact_=true,busy_=false,shift_=false,alpha_=false,testMode_=false,closing_=false;
 bool narrow_=false,layoutReady_=false,handheld_=false,rearranging_=false; QString arrangement_; QList<int> pageBackStack_; int graphGeneration_=0; bool graphRunning_=false,graphPending_=false;
 QJsonObject loadedSettings_; bool configSaveBlocked_=false;
 RegisterArchive registerArchive_; QMap<QString,CalcStoredValue> registerValues_; int registersGeneration_=0; bool registersReady_=false;
 RegistersPanel *registersPanel_=nullptr; QTabWidget *toolboxTabs_=nullptr;
 HistoryArchive historyArchive_; int historyPage_=0; static constexpr int HistoryPageSize=100; QList<int> historyMatches_; QLabel* historyCount_=nullptr; QPushButton *historyPrevious_=nullptr,*historyNext_=nullptr;
 QByteArray savedGeometry_,desktopGeometry_,calculatorGeometry_; bool savedMaximized_=false; QJsonArray history_; QString restoreNotice_;
 bool systemDark_=false; QProcess* systemThemeProcess_=nullptr;
 QThread workerThread_; QObject* worker_=nullptr; CalcEngine* engine_=nullptr;
 QVBoxLayout *outer_=nullptr,*handheldLayout_=nullptr,*graphLayout_=nullptr;
 DesktopSurface* desktopSurface_=nullptr;
 QWidget *desktopHeader_=nullptr,*auxHeader_=nullptr,*handheldHost_=nullptr,*graphHost_=nullptr;
 QLabel* auxTitle_=nullptr; QPushButton *gear_=nullptr,*auxGear_=nullptr,*back_=nullptr,*auxMenu_=nullptr;
 CalculatorMenu* modeMenu_=nullptr;
 bool settingsDraftActive_=false; int settingsReturnPage_=0; QJsonObject settingsDraftBaseline_;
 QPushButton *settingsBack_=nullptr,*settingsCancel_=nullptr,*settingsApply_=nullptr; QWidget *settingsActions_=nullptr;
 QLabel *settingsSummary_=nullptr,*settingsTitle_=nullptr;
 QList<QPair<QLabel*,QWidget*>> settingsRows_; QList<QFrame*> settingsSections_; QFrame *settingsAbout_=nullptr; QLabel *settingsAboutTitle_=nullptr;
 QFormLayout* settingsForm_=nullptr;QWidget* settingsPage_=nullptr;QLabel* settingsVersion_=nullptr;QGridLayout* toolboxGrid_=nullptr;QList<QPushButton*> toolboxButtons_;
 QGridLayout* historyControls_=nullptr;QPushButton* historyExportButton_=nullptr;QPushButton* historyClearButton_=nullptr;
 QStackedWidget *pages_=nullptr; QTabBar *navigation_=nullptr; QLabel *status_=nullptr,*brandSub_=nullptr;
 QWidget *calc_=nullptr,*workspace_=nullptr; QSplitter* split_=nullptr; QTabWidget *compact_=nullptr;
 CalculatorPanel* calculator_=nullptr; CalculatorKeyboard* keyboard_=nullptr;
 GraphPanel *graph_=nullptr; MathEditor *input_=nullptr,*result_=nullptr; QLabel *approx_=nullptr;
 QComboBox *continuousEvaluationBox_=nullptr,*mixedFractionsBox_=nullptr,*keyTooltipsBox_=nullptr,*screenTooltipsBox_=nullptr;
 QComboBox *inputStyleBox_=nullptr,*groupingBox_=nullptr,*decimalSeparatorBox_=nullptr,*desktopEffectsBox_=nullptr;
 QComboBox *baseBox_=nullptr,*notationBox_=nullptr,*calculatorThemeBox_=nullptr,*calculatorLayoutBox_=nullptr,*complexBox_=nullptr;
 QComboBox *angleBox_=nullptr,*exactBox_=nullptr,*themeBox_=nullptr,*appearanceBox_=nullptr,*layoutBox_=nullptr,*designBox_=nullptr,*languageBox_=nullptr;
 QSpinBox *precisionBox_=nullptr; QList<CalculatorKey*> keys_; QList<QPushButton*> memoryButtons_;
 QPushButton *copy_=nullptr,*full_=nullptr; QTableWidget *historyTable_=nullptr; QLineEdit* historySearch_=nullptr;
 QList<QPair<QWidget*,QString>> labels_; QTextBrowser* guide_=nullptr; QTimer* saveTimer_=nullptr;
 void addText(QWidget*,const char*); void queueGraph(QStringList,double,double,int);
};
