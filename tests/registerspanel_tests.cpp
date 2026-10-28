// SPDX-License-Identifier: MIT
#include "registerspanel.h"
#include "matheditor.h"
#include "i18n.h"
#include "calculatorthemes.h"
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QElapsedTimer>
#include <QDir>
#include <QMouseEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QStyle>
#include <QToolButton>
#include <QKeyEvent>
#include <iostream>
#include <cmath>

int main(int argc, char **argv) {
    QApplication app(argc, argv); app.setStyle("Fusion");
    int checks = 0, failures = 0;
    auto check = [&](bool ok, const QString &description) {
        ++checks; if (!ok) ++failures;
        std::cout << (ok ? "PASS " : "FAIL ") << description.toStdString() << '\n';
    };
    RegistersPanel panel; panel.resize(300, 430); panel.show(); app.processEvents();
    auto *clearSelected = panel.findChild<QPushButton *>("clearSelectedRegisters");
    auto *clearAll = panel.findChild<QPushButton *>("clearAllRegisters");
    auto *confirm = panel.findChild<QPushButton *>("confirmClearAllRegisters");
    auto *cancel = panel.findChild<QPushButton *>("cancelClearAllRegisters");
    auto *copy = panel.findChild<QPushButton *>("copyRegisterValue");
    auto *confirmation = panel.findChild<QFrame *>("registersClearConfirmation");
    auto *details = dynamic_cast<MathEditor *>(panel.findChild<QWidget *>("registerDetailsValue"));
    auto *status = panel.findChild<QLabel *>("registerDetailsStatus");
    auto *canonical = panel.findChild<QPlainTextEdit *>("registerCanonicalValue");
    auto *mode = panel.findChild<QLabel *>("registersMode");
    auto *bankTab = panel.findChild<QPushButton *>("registerBankTab");
    auto *valueTab = panel.findChild<QPushButton *>("registerValueTab");
    auto *tabs = panel.findChild<QWidget *>("registerPaneTabs");
    auto *detailScroll = panel.findChild<QScrollArea *>("registerDetailsScroll");
    auto *summary = panel.findChild<QLabel *>("registersSummary");
    auto *bank = panel.findChild<QWidget *>("registerBank");
    auto *inspector = panel.findChild<QWidget *>("registerInspector");
    check(clearSelected && clearAll && confirm && cancel && copy && confirmation && details && canonical && status && mode && bankTab && valueTab && tabs && detailScroll,
          "registers management exposes all controls");
    if (!clearSelected || !clearAll || !confirm || !cancel || !copy || !confirmation || !details || !canonical || !status || !mode || !bankTab || !valueTab || !tabs || !detailScroll) return 1;
    QList<QStringList> clearRequests; panel.onClear = [&](QStringList names) { clearRequests.append(names); };
    check(!clearSelected->isEnabled() && !clearAll->isEnabled() && !copy->isEnabled(), "empty register bank disables clearing and copying");
    check(summary && summary->text() == "0 / 10 stored" && detailScroll->isHidden() && canonical->isHidden(),
          "empty bank reports occupancy and avoids blank inspection canvases");
    for (int index = 0; index < 10; ++index) {
        auto *value = dynamic_cast<MathEditor *>(panel.findChild<QWidget *>(QStringLiteral("registerValue%1").arg(index)));
        check(value && value->isReadOnly() && value->resultPresentation() && value->text().isEmpty() && value->placeholderText() == "Empty",
              QStringLiteral("R%1 starts explicitly Empty in a mathematical read-only display").arg(index));
    }
    QMap<QString, CalcStoredValue> values{{"R0", {"0", false}}, {"R3", {"1/3", false}}, {"R7", {"1.234", true}}};
    CalcResult zero; zero.ok = true; zero.result = "0";
    CalcResult third; third.ok = true; third.result = "0.3"; third.resultRepeatStart = 2; third.resultRepeatLength = 1;
    CalcResult approx; approx.ok = true; approx.result = "1.234"; approx.resultApproximate = true; approx.valueApproximate = true;
    QMap<QString, CalcResult> formatted{{"R0", zero}, {"R3", third}, {"R7", approx}};
    panel.setValues(values, formatted);
    app.processEvents();
    check(summary->text() == "3 / 10 stored" && !detailScroll->isHidden() && !canonical->isHidden(),
          "stored zero restores both complete-value inspection views");
    check(bank && inspector && bank->isVisible() && !inspector->isVisible() && tabs->isVisible(),
          "narrow Desktop shows the bank with dedicated value navigation");
    valueTab->click(); app.processEvents();
    check(inspector->isVisible() && !bank->isVisible() && valueTab->isChecked(),
          "narrow Desktop value navigation reveals its locally scrolling inspector");
    bankTab->click(); app.processEvents();
    panel.resize(980, 620); app.processEvents();
    check(bank && inspector && inspector->mapTo(&panel, QPoint()).x() > bank->mapTo(&panel, QPoint()).x() + bank->width() &&
          std::abs(inspector->mapTo(&panel, QPoint()).y() - bank->mapTo(&panel, QPoint()).y()) < 4 &&
          !tabs->isVisible() && panel.findChild<QScrollArea *>("registersScroll") == nullptr,
          "wide page displays the register bank and complete-value inspector side by side without page overflow");
    panel.resize(300, 430); app.processEvents();
    const QString capturePath = qEnvironmentVariable("EMCICALC_REGISTER_PANEL_CAPTURE");
    if (!capturePath.isEmpty()) { app.processEvents(); panel.grab().save(capturePath); }
    auto *value0 = dynamic_cast<MathEditor *>(panel.findChild<QWidget *>("registerValue0"));
    auto *value3 = dynamic_cast<MathEditor *>(panel.findChild<QWidget *>("registerValue3"));
    auto *value7 = dynamic_cast<MathEditor *>(panel.findChild<QWidget *>("registerValue7"));
    auto *select0 = panel.findChild<QCheckBox *>("registerSelect0");
    auto *select3 = panel.findChild<QCheckBox *>("registerSelect3");
    auto *select7 = panel.findChild<QCheckBox *>("registerSelect7");
    auto *select9 = panel.findChild<QCheckBox *>("registerSelect9");
    check(value0->text() == "0" && value0->placeholderText().isEmpty() && value0->resultRelation() == "=", "stored zero is distinct from Empty");
    check(value3->text() == "0.3" && value3->toPlainText() == "1/3" && value3->recurringBarRects().size() == 1, "stored rational uses exact recurring rendering and complete canonical copy");
    check(value7->resultRelation() == QString::fromUtf8("≈"), "approximation metadata remains visible");
    check(clearRequests.isEmpty() && clearAll->isEnabled() && !clearSelected->isEnabled(), "publishing and inspecting snapshots never requests clearing");
    select9->setChecked(true);
    check(!clearSelected->isEnabled() && panel.selectedRegisters().isEmpty(), "selecting an Empty register does not enable inapplicable clear");
    select0->setChecked(true); select3->setChecked(true);
    check(clearSelected->isEnabled() && panel.selectedRegisters() == QStringList{"R0", "R3"}, "multiple occupied registers can be selected together");
    check(summary->text() == "3 / 10 stored" && panel.findChild<QLabel *>("registerSelectionSummary")->text() == "2 selected" &&
          panel.findChild<QFrame *>("registerRow0")->property("registerActive").toBool() && canonical->toPlainText() == "0",
          "checkbox marking is independent of the active inspector and occupancy count");
    auto *row7 = panel.findChild<QFrame *>("registerRow7");
    QMouseEvent inspectRow(QEvent::MouseButtonPress, QPointF(2, 2), QPointF(row7->mapToGlobal(QPoint(2, 2))),
                           Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(row7, &inspectRow);
    check(row7->property("registerActive").toBool() && canonical->toPlainText() == "1.234" &&
          panel.selectedRegisters() == QStringList{"R0", "R3"} && clearRequests.isEmpty(),
          "clicking the row outline inspects its value without changing clear selection or writing registers");
    check(inspector->isVisible() && !bank->isVisible() && panel.selectedRegisters() == QStringList{"R0", "R3"},
          "inspecting a row opens the narrow value pane without losing checkboxes");
    bankTab->click(); app.processEvents();
    clearSelected->click();
    check(clearRequests == QList<QStringList>{{"R0", "R3"}} && !confirmation->isVisible(), "selected clearing requests only selected occupied registers without a dialog");
    check(value0->text() == "0" && value3->text() == "0.3", "values are retained until owner confirms persistent clear");
    values.remove("R0"); values.remove("R3"); panel.setValues(values, formatted);
    check(!clearSelected->isEnabled() && value0->text().isEmpty() && value3->text().isEmpty(), "committed clear immediately refreshes values and disables empty selection");
    clearAll->click(); app.processEvents();
    check(confirmation->isVisible() && clearRequests.size() == 1, "Clear all first shows an in-window confirmation without mutating state");
    cancel->click();
    check(!confirmation->isVisible() && clearRequests.size() == 1 && value7->text() == "1.234", "cancelling Clear all preserves the bank");
    clearAll->click(); confirm->click();
    check(clearRequests.size() == 2 && clearRequests.last() == QStringList{"R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7", "R8", "R9"}, "confirmed Clear all addresses precisely the ten register identifiers");
    check(!confirmation->isVisible(), "confirmation closes after explicit approval");
    select7->setChecked(true); valueTab->click(); app.processEvents(); copy->click();
    check(QApplication::clipboard()->text() == "1.234" && status->text().contains("Approximate"), "details preserve approximation and canonical value copying");
    CalcOptions options; options.numberBase = 16; options.exact = false; options.notation = 2;
    panel.setDisplayOptions(options);
    check(mode->text().contains("HEX") && mode->text().contains("Decimal") && mode->text().contains("ENG"), "current radix, exact/decimal choice and notation are identifiable");
    check(status->text().contains("pending") && details->text().isEmpty() && details->resultRelation().isEmpty(),
          "changing display options invalidates stale formatting until a matching snapshot arrives");
    values["R7"] = {"1/" + QString(5000, '7'), false};
    formatted.remove("R7"); panel.setValues(values, formatted); copy->click(); app.processEvents();
    check(QApplication::clipboard()->text() == values["R7"].canonical && canonical->toPlainText() == values["R7"].canonical,
          "full canonical values beyond editable-input limits remain intact in details and copy");
    check(status->text().contains("pending") && details->text().isEmpty() && details->placeholderText() == "Formatting…", "pending format leaves mathematical display blank and labels canonical DEC area separately");
    check(canonical->horizontalScrollBar()->maximum() > 0,
          "long values are accessible through local details scrolling");
    CalcResult unsupported; unsupported.error = "Fixture unsupported format";
    formatted["R7"] = unsupported; panel.setValues(values, formatted);
    check(status->text().contains("unavailable") && status->text().contains(unsupported.error) && canonical->toPlainText() == values["R7"].canonical && details->text().isEmpty(),
          "format failure is explicit while complete stored value remains inspectable");
    values["R7"] = {"0." + QString(10000, '7'), true}; formatted.remove("R7");
    QElapsedTimer longValue; longValue.start(); panel.setValues(values, formatted);
    app.setStyleSheet("QWidget { font-family: sans-serif; }"); app.processEvents();
    copy->click();
    check(canonical->isReadOnly() && canonical->lineWrapMode() == QPlainTextEdit::NoWrap &&
          canonical->toPlainText() == values["R7"].canonical && QApplication::clipboard()->text() == values["R7"].canonical,
          "10002-character approximate canonical value remains complete and locally scrollable without math fallback");
    check(details->text().isEmpty() && value7->text().isEmpty() && status->text().contains("Approximate") && longValue.elapsed() < 5000,
          "pending long approximate value stays out of mathematical shaping during retheme");
    app.setStyleSheet({});
    values["R7"] = {"[1,2;3,4]", false}; CalcResult matrix; matrix.ok = true; matrix.result = "[1,2;3,4]";
    formatted["R7"] = matrix; panel.setValues(values, formatted);
    panel.setMathPresentation(1, ',');
    check(details->text() == "[1,2;3,4]" && details->structuredInput() && canonical->toPlainText() == "[1,2;3,4]",
          "matrix values retain mathematical presentation plus separate full canonical text");
    app.processEvents();
    check(value7->height() >= std::ceil(value7->naturalSize().height()),
          "register row height contains complete matrix typesetting");
    CalcResult fraction; fraction.ok = true; fraction.result = "1/3"; formatted["R7"] = fraction;
    values["R7"] = {"1/3", false}; panel.setValues(values, formatted); app.processEvents();
    check(value7->height() >= std::ceil(value7->naturalSize().height()),
          "register row height contains fraction denominator without clipping");
    for (const auto &language : languageCodes()) {
        panel.retranslate([language](const char *key) { return translateText(language, key); }); app.processEvents();
        check(clearSelected->text() == translateText(language, "Clear selected") && value0->placeholderText() == translateText(language, "Empty"), language + " translates clearing and Empty states");
        check(panel.width() == 300 && panel.findChild<QScrollArea *>("registersRows")->horizontalScrollBar()->maximum() == 0,
              language + " narrow management page retains local scrolling without horizontal page overflow");
    }
    panel.retranslate({}); panel.resize(980, 620); app.processEvents();
    for (const QString &theme : {QStringLiteral("Classic"), QStringLiteral("Modern Light"), QStringLiteral("AMOLED")}) {
        app.setPalette(calculatorThemePalette(theme)); app.setStyleSheet(calculatorThemeStyleSheet(theme)); app.processEvents();
        check(panel.findChild<QScrollArea *>("registersRows")->horizontalScrollBar()->maximum() == 0 && canonical->toPlainText() == "1/3" && copy->isEnabled(),
              theme + " palette transition preserves values, inspection and wide geometry");
    }
    app.setStyleSheet({}); app.setPalette(app.style()->standardPalette());
    auto contained = [&](QWidget *widget) { return panel.rect().contains(QRect(widget->mapTo(&panel, QPoint()), widget->size())); };
    panel.setCalculatorMode(true); panel.resize(300, 400); app.processEvents();
    check(bank->isVisible() && inspector->isVisible() && !tabs->isVisible() &&
          inspector->mapTo(&panel, QPoint()).y() >= bank->mapTo(&panel, QPoint()).y() + bank->height(),
          "Calculator keeps register bank above details at narrow width without navigation tabs");
    check(panel.size() == QSize(300, 400) && contained(clearSelected) && contained(clearAll) && contained(copy) && contained(canonical),
          "Calculator minimum-size management and copy controls remain within the panel");
    auto *canonicalTitle = panel.findChild<QLabel *>("registerCanonicalTitle");
    check(detailScroll->mapTo(&panel, QPoint()).y() + detailScroll->height() <= canonicalTitle->mapTo(&panel, QPoint()).y() &&
          canonical->mapTo(&panel, QPoint()).y() + canonical->height() <= copy->mapTo(&panel, QPoint()).y(),
          "Calculator mathematical canvas, canonical caption and pinned copy do not overlap");
    check(details->width() >= std::ceil(details->naturalSize().width()) && details->height() >= std::ceil(details->naturalSize().height()),
          "Calculator complete mathematical canvas contains its natural typesetting");
    panel.resize(980, 620); app.processEvents();
    check(inspector->mapTo(&panel, QPoint()).y() >= bank->mapTo(&panel, QPoint()).y() + bank->height() && !tabs->isVisible(),
          "Calculator retains stacked design even at wide width");
    panel.setCalculatorMode(false); panel.resize(300, 400); bankTab->click(); app.processEvents();
    check(contained(clearSelected) && contained(clearAll) && panel.size() == QSize(300, 400),
          "Desktop narrow bank actions stay pinned inside minimum-size panel");
    valueTab->click(); app.processEvents();
    check(contained(copy) && contained(canonical) && canonical->toPlainText() == "1/3",
          "Desktop narrow value pane retains complete canonical text and pinned copy");
    // Only Calculator's register list changes to adaptive pages.
    RegistersPanel paged; paged.setCalculatorMode(true); paged.resize(300, 400); paged.show();
    QMap<QString, CalcStoredValue> tenValues; QMap<QString, CalcResult> tenFormatted;
    for (int index = 0; index < 10; ++index) {
        const QString name = QString("R%1").arg(index); const QString value = QString::number(index);
        tenValues[name] = {value, false}; CalcResult result; result.ok = true; result.result = value; tenFormatted[name] = result;
    }
    paged.setValues(tenValues, tenFormatted);
    auto settle = [&] { for (int pass = 0; pass < 5; ++pass) app.processEvents(); };
    settle();
    auto *pager = paged.findChild<QWidget *>("registerListPager");
    auto *previousPage = paged.findChild<QToolButton *>("registerPreviousPage");
    auto *nextPage = paged.findChild<QToolButton *>("registerNextPage");
    auto *list = paged.findChild<QScrollArea *>("registersRows");
    auto *pagedCanonical = paged.findChild<QPlainTextEdit *>("registerCanonicalValue");
    auto visibleRows = [&] { QList<int> result; for (int i = 0; i < 10; ++i)
        if (!paged.findChild<QFrame *>(QString("registerRow%1").arg(i))->isHidden()) result.append(i); return result; };
    auto completeRows = [&] { for (int i : visibleRows()) { auto *row = paged.findChild<QFrame *>(QString("registerRow%1").arg(i));
        if (!list->viewport()->rect().contains(QRect(row->mapTo(list->viewport(), QPoint()), row->size()))) return false; } return true; };
    check(pager->isVisible() && !previousPage->isEnabled() && nextPage->isEnabled() &&
          list->verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOff && list->verticalScrollBar()->maximum() == 0 && completeRows(),
          "Calculator minimum-height list uses complete visible rows and arrows with no vertical scrolling");
    paged.findChild<QCheckBox *>("registerSelect0")->setChecked(true);
    paged.findChild<QPushButton *>("registerInspect0")->setFocus(); settle();
    QList<int> visited = visibleRows(); const QString inspectedBeforePage = pagedCanonical->toPlainText();
    for (int safety = 0; nextPage->isEnabled() && safety < 10; ++safety) {
        nextPage->click(); settle(); visited.append(visibleRows());
        check(completeRows() && list->verticalScrollBar()->maximum() == 0,
              "Calculator paged rows remain fully inside the non-scrolling list viewport");
    }
    check(visited == QList<int>{0,1,2,3,4,5,6,7,8,9} && !nextPage->isEnabled() && previousPage->isEnabled(),
          "left/right register pages cover all ten identifiers exactly once");
    check(pagedCanonical->toPlainText() == inspectedBeforePage && paged.selectedRegisters() == QStringList{"R0"},
          "paging away from a focused inspect button preserves the inspected value and cross-page clear marks");
    paged.findChild<QCheckBox *>("registerSelect9")->setChecked(true);
    paged.resize(400, 1200); settle();
    check(!pager->isVisible() && visibleRows().size() == 10 && completeRows() && list->verticalScrollBar()->maximum() == 0,
          "Calculator shows all ten natural-height rows and hides paging arrows when they fit");
    paged.resize(300, 400); settle();
    check(visibleRows().contains(9) && paged.selectedRegisters() == QStringList{"R0", "R9"} && pagedCanonical->toPlainText() == inspectedBeforePage,
          "resizing back to pages retains the register anchor, inspected value and all clear marks");
    auto *inspect9 = paged.findChild<QPushButton *>("registerInspect9"); inspect9->setFocus(); settle();
    paged.resize(400, 1200); settle(); paged.resize(300, 400); settle();
    check(visibleRows().contains(9) && pagedCanonical->toPlainText() == "9" && inspect9->hasFocus(),
          "resizing with a focused Inspect button preserves focus, page anchor and inspected value");
    QKeyEvent home(QEvent::KeyPress, Qt::Key_Home, Qt::NoModifier); QApplication::sendEvent(inspect9, &home); settle();
    check(visibleRows().contains(0) && pagedCanonical->toPlainText() == "0", "Home navigates across register pages before focusing R0");
    nextPage->setFocus(); settle();
    QKeyEvent pressPage(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier);
    QKeyEvent releasePage(QEvent::KeyRelease, Qt::Key_Space, Qt::NoModifier);
    QApplication::sendEvent(nextPage, &pressPage); QApplication::sendEvent(nextPage, &releasePage); settle();
    check(!visibleRows().contains(0) && pagedCanonical->toPlainText() == "0" && paged.selectedRegisters() == QStringList{"R0", "R9"},
          "Space on a focused paging arrow changes only the list page");
    previousPage->click(); settle();
    QKeyEvent end(QEvent::KeyPress, Qt::Key_End, Qt::NoModifier);
    QApplication::sendEvent(paged.findChild<QPushButton *>("registerInspect0"), &end); settle();
    check(visibleRows().contains(9) && pagedCanonical->toPlainText() == "9", "End navigates across register pages before focusing R9");
    const QString largeMatrix = "[1,2,3,4,5;6,7,8,9,10;11,12,13,14,15;16,17,18,19,20;21,22,23,24,25]";
    tenValues["R9"] = {largeMatrix, false}; CalcResult tall; tall.ok = true; tall.result = largeMatrix; tenFormatted["R9"] = tall;
    paged.setValues(tenValues, tenFormatted); settle();
    auto *compactValue = paged.findChild<QLabel *>("registerCompactValue9");
    check(visibleRows().contains(9) && compactValue->isVisible() && compactValue->text().contains(QChar(0x2026)) &&
          compactValue->text().startsWith("=") && completeRows() && pagedCanonical->toPlainText() == largeMatrix,
          "an oversized matrix has a readable elided list preview while the complete inspected value remains intact");
    paged.findChild<QPushButton *>("copyRegisterValue")->click();
    check(QApplication::clipboard()->text() == largeMatrix, "paged matrix fallback never changes complete canonical copying");
    paged.findChild<QPushButton *>("clearAllRegisters")->click(); settle();
    check(completeRows() && list->verticalScrollBar()->maximum() == 0 &&
          paged.rect().contains(QRect(paged.findChild<QPushButton *>("confirmClearAllRegisters")->mapTo(&paged, QPoint()),
                                     paged.findChild<QPushButton *>("confirmClearAllRegisters")->size())),
          "minimum-size Calculator confirmation keeps its page rows and clear controls within the panel");
    paged.findChild<QPushButton *>("cancelClearAllRegisters")->click(); settle();
    check(visibleRows().contains(9) && paged.selectedRegisters() == QStringList{"R0", "R9"} && pagedCanonical->toPlainText() == largeMatrix,
          "cancelling Clear all preserves paged register anchor, inspection and selections");
    paged.setValues(tenValues, {}); settle();
    check(visibleRows().contains(9) && pagedCanonical->toPlainText() == largeMatrix && completeRows(),
          "pending asynchronous formatting preserves the anchored register and complete canonical value");
    paged.setValues(tenValues, tenFormatted); settle();
    check(visibleRows().contains(9) && pagedCanonical->toPlainText() == largeMatrix && completeRows(),
          "formatted snapshot restores natural-height pages without losing their register anchor");
    auto withoutNine = tenValues; withoutNine.remove("R9"); paged.setValues(withoutNine, tenFormatted); settle();
    check(visibleRows().contains(9) && pagedCanonical->toPlainText().isEmpty() && completeRows(),
          "empty inspected register expands the available list while keeping its anchored row reachable");
    paged.setValues(tenValues, tenFormatted); paged.findChild<QCheckBox *>("registerSelect9")->setChecked(true); settle();
    check(visibleRows().contains(9) && pagedCanonical->toPlainText() == largeMatrix && completeRows(),
          "empty-to-occupied inspector transition preserves page reachability without clipping");
    QStringList pageClear; paged.onClear = [&](QStringList names) { pageClear = names; };
    paged.findChild<QPushButton *>("clearSelectedRegisters")->click();
    check(pageClear == QStringList{"R0", "R9"}, "clear selected includes checked registers from different pages");
    paged.setCalculatorMode(false); paged.resize(980, 620); settle();
    check(!pager->isVisible() && visibleRows().size() == 10 && list->verticalScrollBarPolicy() == Qt::ScrollBarAsNeeded &&
          compactValue->isHidden() && !paged.findChild<QWidget *>("registerValue9")->isHidden(),
          "Desktop restores all ten original mathematical rows and its existing scrollable bank without pagination");
    panel.setValues({}, {}); check(!clearSelected->isEnabled() && !clearAll->isEnabled() && !copy->isEnabled(), "clearing the bank refreshes all actions");
    check(detailScroll->isHidden() && canonical->isHidden() && summary->text() == "0 / 10 stored",
          "committed empty bank returns to compact inspection state");
    const QString captureDirectory = qEnvironmentVariable("EMCICALC_REGISTER_PANEL_CAPTURE_DIR");
    if (!captureDirectory.isEmpty()) {
        QDir().mkpath(captureDirectory);
        RegistersPanel capture; capture.setValues({{"R0", {"1/3", false}}, {"R1", {"sqrt(2)", false}},
            {"R2", {"0", false}}, {"R3", {"1.234", true}}, {"R4", {"[1,2;3,4]", false}}},
            {{"R0", fraction}, {"R1", [] { CalcResult r; r.ok = true; r.result = "sqrt(2)"; return r; }()},
             {"R2", zero}, {"R3", approx}, {"R4", matrix}});
        capture.show();
        capture.findChild<QCheckBox *>("registerSelect1")->setChecked(true);
        capture.findChild<QCheckBox *>("registerSelect3")->setChecked(true);
        capture.findChild<QPushButton *>("registerInspect0")->setFocus();
        for (const QString &theme : {QStringLiteral("Classic"), QStringLiteral("Modern Light"), QStringLiteral("AMOLED")}) {
            app.setPalette(calculatorThemePalette(theme)); app.setStyleSheet(calculatorThemeStyleSheet(theme));
            for (const QString &presentation : {QStringLiteral("desktop-wide"), QStringLiteral("desktop-narrow-bank"), QStringLiteral("desktop-narrow-value"), QStringLiteral("calculator"), QStringLiteral("calculator-all-fit")}) {
                capture.setCalculatorMode(presentation.startsWith("calculator"));
                capture.resize(presentation == "desktop-wide" ? QSize(980, 620) : presentation == "calculator-all-fit" ? QSize(400, 1200) : QSize(300, 400));
                capture.findChild<QPushButton *>(presentation == "desktop-narrow-value" ? "registerValueTab" : "registerBankTab")->click();
                for (int step = 0; step < 4; ++step) app.processEvents();
                capture.grab().save(QDir(captureDirectory).filePath(QString("registers-%1-%2.png").arg(theme.toLower().replace(' ', '-'), presentation)));
            }
        }
    }
    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
