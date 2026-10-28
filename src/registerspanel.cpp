// SPDX-License-Identifier: MIT
#include "registerspanel.h"
#include "matheditor.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QEvent>
#include <QFrame>
#include <QFontInfo>
#include <QFontMetricsF>
#include <QFontDatabase>
#include <QPlainTextEdit>
#include <QPainter>
#include <QHBoxLayout>
#include <QLabel>
#include <QKeyEvent>
#include <QStyle>
#include <QScrollBar>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QStyleOptionButton>
#include <QVBoxLayout>
#include <QTimer>
#include <QToolButton>
#include <cmath>

namespace {
QString registerName(int index) { return QStringLiteral("R%1").arg(index); }
// QBoxLayout's height-for-width pass can allocate equal slots to the two
// wrapping panes even when the register list has a larger explicit budget.
// Calculator pagination needs that budget exactly; Desktop keeps the original
// QBoxLayout geometry path, including its width-dependent two-pane behavior.
class RegisterSectionsLayout final : public QBoxLayout {
public:
    RegisterSectionsLayout() : QBoxLayout(QBoxLayout::LeftToRight) {}
    void setCalculatorBankHeight(int height) { bankHeight_ = height; invalidate(); }
    void setGeometry(const QRect &rectangle) override {
        QBoxLayout::setGeometry(rectangle);
        if (bankHeight_ < 0 || count() != 2) return;
        const int bankHeight = qMin(bankHeight_, qMax(0, rectangle.height() - spacing() - itemAt(1)->minimumSize().height()));
        itemAt(0)->setGeometry(QRect(rectangle.x(), rectangle.y(), rectangle.width(), bankHeight));
        itemAt(1)->setGeometry(QRect(rectangle.x(), rectangle.y() + bankHeight + spacing(), rectangle.width(),
                                   qMax(0, rectangle.height() - bankHeight - spacing())));
    }
private:
    int bankHeight_ = -1;
};
// Use the current native palette on every paint: changing skins never leaves
// fixed light/dark colours behind, and the math canvas keeps its own palette.
class RegisterCard final : public QFrame {
public:
    RegisterCard() { setAttribute(Qt::WA_Hover); }
protected:
    void paintEvent(QPaintEvent *) override {
        const bool active = property("registerActive").toBool();
        QPainter painter(this); painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(palette().color(QPalette::Base));
        painter.setPen(QPen(palette().color(active || underMouse() ? QPalette::Highlight : QPalette::Mid), active ? 2 : 1));
        painter.drawRoundedRect(QRectF(rect()).adjusted(1, 1, -1, -1), 6, 6);
    }
    bool event(QEvent *event) override {
        if (event->type() == QEvent::HoverEnter || event->type() == QEvent::HoverLeave) update();
        return QFrame::event(event);
    }
};
class RegisterSelection final : public QCheckBox {
public:
    explicit RegisterSelection(const QString &name) : QCheckBox(name) {}
protected:
    void paintEvent(QPaintEvent *event) override {
        QCheckBox::paintEvent(event);
        // Some native styles derive an unchecked border from the dark button
        // shade, making it disappear on AMOLED. Keep the native tick and focus
        // treatment, with a visible outline from the value surface's text ink.
        QStyleOptionButton option; initStyleOption(&option);
        const QRect indicator = style()->subElementRect(QStyle::SE_CheckBoxIndicator, &option, this);
        QPainter painter(this); QColor ink = palette().color(QPalette::Text); ink.setAlpha(160);
        painter.setPen(ink); painter.setBrush(Qt::NoBrush);
        painter.drawRect(indicator.adjusted(0, 0, -1, -1));
    }
};
QLabel *label(const QString &name = {}) {
    auto *result = new QLabel;
    result->setObjectName(name); result->setTextFormat(Qt::PlainText);
    result->setWordWrap(true); result->setMinimumWidth(0);
    result->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    return result;
}
}

RegistersPanel::RegistersPanel(QWidget *parent) : QWidget(parent) {
    setObjectName("registersPanel");
    auto *layout = new QVBoxLayout(this); layout->setContentsMargins(10, 8, 10, 8); layout->setSpacing(7);
    layout->setSizeConstraint(QLayout::SetNoConstraint);
    auto *headingLayout = new QHBoxLayout;
    title_ = label("registersTitle");
    QFont heading = title_->font(); heading.setBold(true); heading.setPointSizeF(heading.pointSizeF() + 2);
    title_->setFont(heading); headingLayout->addWidget(title_, 1);
    mode_ = label("registersMode"); mode_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    headingLayout->addWidget(mode_, 1); layout->addLayout(headingLayout);
    note_ = label("registersNote"); layout->addWidget(note_);
    paneTabs_ = new QWidget; paneTabs_->setObjectName("registerPaneTabs");
    auto *tabs = new QHBoxLayout(paneTabs_); tabs->setContentsMargins(0, 0, 0, 0); tabs->setSpacing(4);
    bankTab_ = new QPushButton; bankTab_->setObjectName("registerBankTab"); bankTab_->setCheckable(true);
    valueTab_ = new QPushButton; valueTab_->setObjectName("registerValueTab"); valueTab_->setCheckable(true);
    for (auto *tab : {bankTab_, valueTab_})
        tab->setStyleSheet("QPushButton:checked { background: palette(highlight); color: palette(highlighted-text); border: 1px solid palette(highlight); }");
    tabs->addWidget(bankTab_, 1); tabs->addWidget(valueTab_, 1); layout->addWidget(paneTabs_);
    // There is deliberately no page scroll area: each value/list owns its
    // overflow, while management actions remain pinned inside the visible pane.
    sections_ = new RegisterSectionsLayout; sections_->setSpacing(12);
    layout->addLayout(sections_, 1);
    bank_ = new QWidget; bank_->setObjectName("registerBank"); bank_->setMinimumWidth(0);
    bank_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto *bankLayout = new QVBoxLayout(bank_); bankLayout->setContentsMargins(0, 0, 0, 0); bankLayout->setSpacing(6);
    auto *bankHeading = new QHBoxLayout;
    bankTitle_ = label("registerBankTitle"); QFont bold = bankTitle_->font(); bold.setBold(true); bankTitle_->setFont(bold);
    summary_ = label("registersSummary"); summary_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    registerPager_ = new QWidget; registerPager_->setObjectName("registerListPager");
    auto *pagerLayout = new QHBoxLayout(registerPager_); pagerLayout->setContentsMargins(0, 0, 0, 0); pagerLayout->setSpacing(3);
    previousPage_ = new QToolButton; previousPage_->setObjectName("registerPreviousPage"); previousPage_->setArrowType(Qt::LeftArrow);
    nextPage_ = new QToolButton; nextPage_->setObjectName("registerNextPage"); nextPage_->setArrowType(Qt::RightArrow);
    previousPage_->setFixedSize(24, 24); nextPage_->setFixedSize(24, 24);
    pageRange_ = label("registerPageRange"); pageRange_->setWordWrap(false); pageRange_->setAlignment(Qt::AlignCenter);
    pagerLayout->addWidget(previousPage_); pagerLayout->addWidget(pageRange_, 1); pagerLayout->addWidget(nextPage_);
    bankHeading->addWidget(bankTitle_, 1); bankHeading->addWidget(registerPager_, 1); bankHeading->addWidget(summary_, 1);
    registerPager_->hide(); bankLayout->addLayout(bankHeading);
    rowsScroll_ = new QScrollArea; rowsScroll_->setObjectName("registersRows");
    rowsScroll_->setWidgetResizable(true); rowsScroll_->setFrameShape(QFrame::NoFrame);
    rowsScroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); rowsScroll_->setMinimumHeight(40);
    rowsScroll_->viewport()->installEventFilter(this);
    auto *rowsContents = new QWidget; auto *rowsLayout = new QVBoxLayout(rowsContents);
    rowsLayout->setContentsMargins(0, 0, 3, 0); rowsLayout->setSpacing(5);
    for (int index = 0; index < 10; ++index) {
        auto *row = new RegisterCard; row->setObjectName(QStringLiteral("registerRow%1").arg(index));
        row->installEventFilter(this);
        auto *rowLayout = new QHBoxLayout(row); rowLayout->setContentsMargins(8, 3, 8, 3); rowLayout->setSpacing(7);
        auto &widgets = rows_[index]; widgets.card = row;
        widgets.selection = new RegisterSelection({});
        widgets.selection->setObjectName(QStringLiteral("registerSelect%1").arg(index));
        widgets.selection->setStyleSheet("QCheckBox { color: palette(text); }");
        rowLayout->addWidget(widgets.selection);
        widgets.inspect = new QPushButton(registerName(index));
        widgets.inspect->setObjectName(QStringLiteral("registerInspect%1").arg(index));
        widgets.inspect->setFlat(true); widgets.inspect->setFont(bold);
        widgets.inspect->setStyleSheet("QPushButton { background: transparent; color: palette(text); border: none; padding: 3px; } QPushButton:focus { border: 1px solid palette(highlight); }");
        widgets.inspect->setFixedWidth(34); widgets.inspect->setCursor(Qt::PointingHandCursor);
        widgets.inspect->installEventFilter(this); rowLayout->addWidget(widgets.inspect);
        widgets.value = new MathEditor;
        widgets.value->setObjectName(QStringLiteral("registerValue%1").arg(index));
        widgets.value->setReadOnly(true); widgets.value->setResultPresentation(true);
        QFont valueFont = widgets.value->font(); valueFont.setPixelSize(17); widgets.value->setFont(valueFont);
        widgets.value->setMinimumHeight(44); widgets.value->setFocusPolicy(Qt::NoFocus);
        widgets.value->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        widgets.value->setCursor(Qt::PointingHandCursor);
        widgets.value->installEventFilter(this); rowLayout->addWidget(widgets.value, 1);
        // A single unusually tall matrix may exceed even a one-row page.
        // Keep a readable, explicitly elided list preview in that case; the
        // complete mathematical inspector and canonical copy are unchanged.
        widgets.preview = label(QStringLiteral("registerCompactValue%1").arg(index));
        widgets.preview->setWordWrap(false); widgets.preview->setFont(valueFont);
        widgets.preview->setFixedHeight(44); widgets.preview->setCursor(Qt::PointingHandCursor);
        widgets.preview->installEventFilter(this); rowLayout->addWidget(widgets.preview, 1); widgets.preview->hide();
        rowsLayout->addWidget(row);
        // Checking a register marks it for clearing, independently of which
        // value is being inspected. Navigation never changes these checkboxes.
        connect(widgets.selection, &QCheckBox::toggled, this, [this] { updateControls(); if (calculatorMode_) updatePaneLayout(); });
        connect(widgets.inspect, &QPushButton::clicked, this, [this, index] { if (calculatorMode_) pageAnchor_ = index; showDetails(index, true); });
    }
    rowsLayout->addStretch(); rowsScroll_->setWidget(rowsContents); bankLayout->addWidget(rowsScroll_, 1);
    normalActions_ = new QWidget; auto *normalLayout = new QVBoxLayout(normalActions_);
    normalLayout->setContentsMargins(0, 0, 0, 0); normalLayout->setSpacing(5);
    selectionSummary_ = label("registerSelectionSummary"); normalLayout->addWidget(selectionSummary_);
    clearSelected_ = new QPushButton; clearSelected_->setObjectName("clearSelectedRegisters");
    clearAll_ = new QPushButton; clearAll_->setObjectName("clearAllRegisters");
    actions_ = new QBoxLayout(QBoxLayout::LeftToRight); actions_->setSpacing(5);
    actions_->addWidget(clearSelected_, 1); actions_->addWidget(clearAll_, 1); normalLayout->addLayout(actions_);
    bankLayout->addWidget(normalActions_);
    confirmation_ = new QFrame; confirmation_->setObjectName("registersClearConfirmation");
    confirmation_->setProperty("converterCard", true);
    auto *confirmLayout = new QVBoxLayout(confirmation_); confirmLayout->setContentsMargins(4, 4, 4, 4); confirmLayout->setSpacing(5);
    confirmText_ = label(); confirmLayout->addWidget(confirmText_);
    confirmClear_ = new QPushButton; confirmClear_->setObjectName("confirmClearAllRegisters");
    cancelClear_ = new QPushButton; cancelClear_->setObjectName("cancelClearAllRegisters");
    confirmationActions_ = new QBoxLayout(QBoxLayout::LeftToRight); confirmationActions_->setSpacing(5);
    confirmationActions_->addWidget(cancelClear_, 1); confirmationActions_->addWidget(confirmClear_, 1);
    confirmLayout->addLayout(confirmationActions_); bankLayout->addWidget(confirmation_); confirmation_->hide();
    sections_->addWidget(bank_, 1);
    inspector_ = new QWidget; inspector_->setObjectName("registerInspector"); inspector_->setMinimumWidth(0);
    inspector_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto *detailsLayout = new QVBoxLayout(inspector_); detailsLayout->setContentsMargins(0, 0, 0, 0); detailsLayout->setSpacing(6);
    inspectorTitle_ = label("registerInspectorTitle"); inspectorTitle_->setFont(bold); detailsLayout->addWidget(inspectorTitle_);
    detailTitle_ = label("registerDetailsTitle"); detailTitle_->setFont(heading); detailsLayout->addWidget(detailTitle_);
    detailStatus_ = label("registerDetailsStatus");
    detailStatus_->setTextInteractionFlags(Qt::TextSelectableByMouse); detailsLayout->addWidget(detailStatus_);
    detailsScroll_ = new QScrollArea; detailsScroll_->setObjectName("registerDetailsScroll");
    detailsScroll_->setFrameShape(QFrame::StyledPanel); detailsScroll_->setWidgetResizable(false);
    detailsScroll_->setMinimumHeight(56); detailsScroll_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    detailsScroll_->viewport()->installEventFilter(this);
    detail_ = new MathEditor; detail_->setObjectName("registerDetailsValue");
    detail_->setReadOnly(true); detail_->setResultPresentation(true);
    QFont detailFace = detail_->font(); detailFace.setPixelSize(23); detail_->setFont(detailFace);
    detailsScroll_->setWidget(detail_); detail_->installEventFilter(this); detailsLayout->addWidget(detailsScroll_, 2);
    canonicalTitle_ = label("registerCanonicalTitle"); canonicalTitle_->setFont(bold); detailsLayout->addWidget(canonicalTitle_);
    canonical_ = new QPlainTextEdit; canonical_->setObjectName("registerCanonicalValue");
    canonical_->setReadOnly(true); canonical_->setLineWrapMode(QPlainTextEdit::NoWrap);
    canonical_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    canonical_->setMinimumHeight(48); canonical_->setMinimumWidth(0); detailsLayout->addWidget(canonical_, 1);
    empty_ = label("registerEmptyMessage"); empty_->setAlignment(Qt::AlignCenter); detailsLayout->addWidget(empty_, 1);
    copy_ = new QPushButton; copy_->setObjectName("copyRegisterValue"); detailsLayout->addWidget(copy_);
    sections_->addWidget(inspector_, 1);

    connect(previousPage_, &QToolButton::clicked, this, [this] { moveRegisterPage(-1); });
    connect(nextPage_, &QToolButton::clicked, this, [this] { moveRegisterPage(1); });
    connect(bankTab_, &QPushButton::clicked, this, [this] { setInspectorVisible(false); });
    connect(valueTab_, &QPushButton::clicked, this, [this] { setInspectorVisible(true); });
    connect(clearSelected_, &QPushButton::clicked, this, [this] { requestClear(false); });
    connect(clearAll_, &QPushButton::clicked, this, [this] {
        normalActions_->hide(); confirmation_->show(); updatePaneLayout(); cancelClear_->setFocus(Qt::OtherFocusReason);
    });
    connect(cancelClear_, &QPushButton::clicked, this, [this] {
        confirmation_->hide(); normalActions_->show(); updatePaneLayout(); clearAll_->setFocus(Qt::OtherFocusReason);
    });
    connect(confirmClear_, &QPushButton::clicked, this, [this] { requestClear(true); });
    connect(copy_, &QPushButton::clicked, this, [this] {
        const auto found = values_.constFind(registerName(current_));
        if (found != values_.cend() && !found->canonical.isEmpty())
            QApplication::clipboard()->setText(found->canonical);
    });
    retranslate({}); updatePaneLayout();
}

QString RegistersPanel::trc(const char *text) const {
    return translate_ ? translate_(text) : QString::fromUtf8(text);
}
void RegistersPanel::setHeaderVisible(bool visible) { headerVisible_ = visible; title_->setVisible(visible && !calculatorMode_); }
void RegistersPanel::setCalculatorMode(bool calculator) {
    if (calculatorMode_ == calculator) return;
    calculatorMode_ = calculator;
    if (calculator) pageAnchor_ = current_;
    QFont detailFace = detail_->font(); detailFace.setPixelSize(calculator ? 20 : 23); detail_->setFont(detailFace);
    updatePaneLayout(); showDetails(current_); updateControls();
}
void RegistersPanel::setValues(const QMap<QString, CalcStoredValue> &values,
                               const QMap<QString, CalcResult> &formatted) {
    values_ = values; formatted_ = formatted; refreshRows();
}
void RegistersPanel::setDisplayOptions(const CalcOptions &options) {
    if (options_.numberBase != options.numberBase || options_.exact != options.exact ||
        options_.notation != options.notation || options_.precision != options.precision ||
        options_.mixedFractions != options.mixedFractions || options_.complexEnabled != options.complexEnabled)
        formatted_.clear();
    options_ = options; refreshRows();
}
void RegistersPanel::setMathPresentation(int grouping, QChar decimalSeparator) {
    grouping_ = grouping; decimalSeparator_ = decimalSeparator;
    for (auto &row : rows_) row.value->setNumberFormat(grouping_, decimalSeparator_);
    detail_->setNumberFormat(grouping_, decimalSeparator_); refreshRows();
}
void RegistersPanel::setCatalog(const QVector<CatalogEntry> &catalog) {
    QStringList units, constants; QMap<QString, QString> labels;
    for (const auto &entry : catalog) {
        if (entry.unit) { units.append(entry.token); units.append(entry.aliases); }
        else { constants.append(entry.token); constants.append(entry.aliases); }
        if (entry.unit && !entry.displayToken.isEmpty()) labels.insert(entry.token, entry.displayToken);
    }
    for (auto &row : rows_) {
        row.value->setUnitTokens(units); row.value->setUnitLabels(labels); row.value->setConstantNames(constants);
    }
    detail_->setUnitTokens(units); detail_->setUnitLabels(labels); detail_->setConstantNames(constants);
}
void RegistersPanel::retranslate(std::function<QString(const char *)> translate) {
    translate_ = std::move(translate);
    title_->setText(trc("Registers"));
    note_->setText(trc("Click a value to inspect it. Use checkboxes to select."));
    bankTitle_->setText(trc("Register bank")); inspectorTitle_->setText(trc("Value inspector"));
    bankTab_->setText(trc("Register bank"));
    previousPage_->setToolTip(trc("Previous")); previousPage_->setAccessibleName(trc("Previous"));
    nextPage_->setToolTip(trc("Next")); nextPage_->setAccessibleName(trc("Next"));
    clearSelected_->setText(trc("Clear selected")); clearAll_->setText(trc("Clear all"));
    confirmText_->setText(trc("Clear all ten registers? M, Ans and History will be kept."));
    confirmClear_->setText(trc("Clear all")); cancelClear_->setText(trc("Cancel"));
    copy_->setText(trc("Copy complete value"));
    canonicalTitle_->setText(trc("Canonical value (DEC)"));
    canonical_->setAccessibleName(trc("Canonical value (DEC)"));
    canonical_->setPlaceholderText(trc("Empty"));
    for (int index = 0; index < 10; ++index) {
        rows_[index].selection->setAccessibleName(trc("Select %1").arg(registerName(index)));
        rows_[index].selection->setToolTip(trc("Select %1").arg(registerName(index)));
        rows_[index].inspect->setAccessibleName(trc("Value of %1").arg(registerName(index)));
        rows_[index].value->setAccessibleName(trc("Value of %1").arg(registerName(index)));
        rows_[index].preview->setAccessibleName(trc("Value of %1").arg(registerName(index)));
    }
    refreshRows(); updatePaneLayout();
}
QStringList RegistersPanel::selectedRegisters() const {
    QStringList selected;
    for (int index = 0; index < 10; ++index)
        if (rows_[index].selection->isChecked() && values_.contains(registerName(index)) &&
            !values_.value(registerName(index)).canonical.isEmpty()) selected << registerName(index);
    return selected;
}
void RegistersPanel::presentValue(MathEditor *editor, const QString &name) {
    const auto value = values_.constFind(name);
    if (value == values_.cend() || value->canonical.isEmpty()) {
        editor->setText({}); editor->setResultRelation({}); editor->setPlaceholderText(trc("Empty")); return;
    }
    const auto result = formatted_.constFind(name);
    // Pending/unsupported values stay in the plain canonical inspection area.
    // Formatting can expose 10,000-digit approximate canonical payloads that
    // should never be parsed into a hidden mathematical widget as a fallback.
    const bool formatted = result != formatted_.cend() && result->ok;
    if (!formatted) {
        editor->setText({}); editor->setResultRelation({});
        editor->setPlaceholderText(result == formatted_.cend() ? trc("Formatting…") : trc("Format unavailable"));
        return;
    }
    editor->setPlaceholderText({});
    const QString display = result->result;
    if (editor == detail_) {
        // MathEditor deliberately fits unbreakable rows to its own width.
        // Give the local inspection canvas its full text width first so a
        // thousand-digit number remains readable instead of being miniaturized.
        QFont face = editor->font();
        if (face.pixelSize() < 0) face.setPixelSize(qMax(16, QFontInfo(face).pixelSize()));
        const qreal measured = QFontMetricsF(face).horizontalAdvance(display) + 64;
        editor->resize(qMax(detailsScroll_->viewport()->width(),
                            static_cast<int>(qMin(qreal(QWIDGETSIZE_MAX), std::ceil(measured)))),
                       qMax(90, detailsScroll_->viewport()->height()));
    }
    editor->setNumberBase(options_.numberBase);
    editor->setText(display);
    editor->setResultRelation(result->resultApproximate
                              ? QString::fromUtf8("≈") : QStringLiteral("="));
    editor->setRecurringDigits(result->resultRepeatStart, result->resultRepeatLength, value->canonical);
    editor->setCursorPosition(0); editor->deselect();
}
void RegistersPanel::refreshRows() {
    const QString base = options_.numberBase == 2 ? "BIN" : options_.numberBase == 8 ? "OCT" :
                         options_.numberBase == 16 ? "HEX" : "DEC";
    const QString notation = options_.notation == 1 ? "SCI" : options_.notation == 2 ? "ENG" : trc("Normal");
    mode_->setText(base + QStringLiteral(" · ") + (options_.exact ? trc("Exact") : trc("Decimal")) +
                   QStringLiteral(" · ") + notation);
    for (int index = 0; index < 10; ++index) presentValue(rows_[index].value, registerName(index));
    updateRowGeometry(); showDetails(current_); updateControls();
}
void RegistersPanel::showDetails(int index, bool reveal) {
    current_ = qBound(0, index, 9); const QString name = registerName(current_);
    for (int row = 0; row < 10; ++row) {
        rows_[row].card->setProperty("registerActive", row == current_); rows_[row].card->update();
    }
    detailTitle_->setText(trc("%1 — complete value").arg(name));
    detail_->setAccessibleName(trc("Value of %1").arg(name)); presentValue(detail_, name);
    const auto value = values_.constFind(name);
    const QString canonical = value == values_.cend() ? QString() : value->canonical;
    if (canonical_->toPlainText() != canonical) canonical_->setPlainText(canonical);
    // An empty register needs a clear state, not two large blank value boxes.
    detailsScroll_->setVisible(!canonical.isEmpty()); canonicalTitle_->setVisible(!canonical.isEmpty());
    canonical_->setVisible(!canonical.isEmpty()); copy_->setVisible(!canonical.isEmpty());
    empty_->setVisible(canonical.isEmpty());
    if (value == values_.cend() || value->canonical.isEmpty()) detailStatus_->setText(trc("Empty"));
    else {
        const auto result = formatted_.constFind(name);
        const QString accuracy = value->approximate ? trc("Approximate stored value") : trc("Exact stored value");
        if (result == formatted_.cend()) detailStatus_->setText(accuracy + "\n" + trc("Canonical value; display formatting is pending."));
        else if (!result->ok) detailStatus_->setText(accuracy + "\n" + trc("This display format is unavailable. The canonical value is shown.") +
                                                    (result->error.isEmpty() ? QString() : "\n" + result->error));
        else detailStatus_->setText(accuracy + QStringLiteral(" · ") + mode_->text());
    }
    if (calculatorMode_ && !canonical.isEmpty()) {
        const auto result = formatted_.constFind(name);
        const QString fullStatus = detailStatus_->text();
        detailStatus_->setText(value->approximate ? trc("Approximate stored value") : trc("Exact stored value"));
        if (result == formatted_.cend()) detailStatus_->setText(detailStatus_->text() + " · " + trc("Formatting…"));
        else if (!result->ok) detailStatus_->setText(detailStatus_->text() + " · " + trc("Format unavailable"));
        detailStatus_->setToolTip(fullStatus);
    } else detailStatus_->setToolTip({});
    updateDetailsGeometry(); updateControls();
    valueTab_->setText(trc("Value") + QStringLiteral(" · ") + name);
    empty_->setText(trc("No value is stored here.") + QStringLiteral("\n\n") + name + QStringLiteral(" := …"));
    if (reveal && !calculatorMode_ && width() < 620) inspectorVisible_ = true;
    updatePaneLayout();
}
void RegistersPanel::updateControls() {
    const int selected = selectedRegisters().size();
    clearSelected_->setEnabled(selected > 0);
    int occupiedCount = 0; for (int index = 0; index < 10; ++index)
        if (!values_.value(registerName(index)).canonical.isEmpty()) ++occupiedCount;
    const bool occupied = occupiedCount > 0;
    summary_->setText(trc("%1 / 10 stored").arg(occupiedCount) +
        (calculatorMode_ && selected ? QStringLiteral(" · ") + trc("%1 selected").arg(selected) : QString()));
    selectionSummary_->setText(selected ? trc("%1 selected").arg(selected) : trc("No registers selected"));
    for (int index = 0; index < 10; ++index) {
        const bool occupiedRow = !values_.value(registerName(index)).canonical.isEmpty();
        rows_[index].selection->setEnabled(occupiedRow);
        if (!occupiedRow && rows_[index].selection->isChecked()) {
            const QSignalBlocker blocker(rows_[index].selection); rows_[index].selection->setChecked(false);
        }
    }
    clearAll_->setEnabled(occupied); confirmClear_->setEnabled(occupied);
    copy_->setEnabled(values_.contains(registerName(current_)) && !values_.value(registerName(current_)).canonical.isEmpty());
    if (!occupied) { confirmation_->hide(); normalActions_->show(); }
}
void RegistersPanel::requestClear(bool all) {
    QStringList names;
    if (all) for (int index = 0; index < 10; ++index) names << registerName(index);
    else names = selectedRegisters();
    confirmation_->hide(); normalActions_->show(); updatePaneLayout();
    // The owner must publish the committed snapshot. Do not optimistically erase
    // UI state: a failed persistent write must retain the still-stored values.
    if (!names.isEmpty() && onClear) onClear(names);
}
void RegistersPanel::updateDetailsGeometry() {
    if (!detail_ || !detailsScroll_) return;
    const QSizeF size = detail_->naturalSize();
    detail_->resize(qMax(detailsScroll_->viewport()->width(), static_cast<int>(std::ceil(size.width()))),
                    qMax(detailsScroll_->viewport()->height(), static_cast<int>(std::ceil(size.height()))));
}
void RegistersPanel::updateRowGeometry() {
    rowGeometryPending_ = false;
    for (auto &row : rows_) if (row.value) {
        const int height = qMax(44, static_cast<int>(std::ceil(row.value->naturalSize().height())) + 8);
        if (row.value->minimumHeight() != height || row.value->maximumHeight() != height)
            row.value->setFixedHeight(height);
    }
    if (calculatorMode_ && !pagingLayout_) updatePaneLayout();
}
void RegistersPanel::moveRegisterPage(int delta) {
    if (!calculatorMode_ || pageStarts_.size() < 2) return;
    int page = 0;
    while (page + 1 < pageStarts_.size() && pageStarts_[page + 1] <= pageAnchor_) ++page;
    page = qBound(0, page + delta, static_cast<int>(pageStarts_.size()) - 1);
    pageAnchor_ = pageStarts_[page]; updatePaneLayout();
}
void RegistersPanel::updateRegisterPages(int viewportHeight) {
    pageStarts_.clear(); pageStarts_.append(0);
    const int spacing = rowsScroll_->widget()->layout()->spacing();
    int used = 0;
    for (int index = 0; index < 10; ++index) {
        auto &row = rows_[index];
        const auto margins = row.card->layout()->contentsMargins();
        const int naturalHeight = row.value->height() + margins.top() + margins.bottom();
        const bool compact = naturalHeight > viewportHeight;
        row.value->setVisible(!compact); row.preview->setVisible(compact);
        if (compact) {
            const int lineHeight = QFontMetrics(row.preview->font()).height() + 4;
            row.preview->setFixedHeight(qMax(lineHeight, qMin(44, viewportHeight - margins.top() - margins.bottom())));
            QString preview = row.value->text();
            if (preview.isEmpty()) preview = row.value->placeholderText();
            else preview.prepend(row.value->resultRelation() + QStringLiteral(" "));
            const int previewWidth = qMax(20, rowsScroll_->viewport()->width() - 86);
            row.preview->setText(QFontMetrics(row.preview->font()).elidedText(preview, Qt::ElideRight, previewWidth));
            row.preview->setToolTip(trc("%1 — complete value").arg(registerName(index)));
        }
        const int rowHeight = compact ? row.preview->height() + margins.top() + margins.bottom() : naturalHeight;
        const int required = rowHeight + (used ? spacing : 0);
        if (used && used + required > viewportHeight) { pageStarts_.append(index); used = rowHeight; }
        else used += required;
    }
    int page = 0;
    while (page + 1 < pageStarts_.size() && pageStarts_[page + 1] <= pageAnchor_) ++page;
    const int first = pageStarts_[page];
    const int last = page + 1 < pageStarts_.size() ? pageStarts_[page + 1] - 1 : 9;
    for (int index = 0; index < 10; ++index) rows_[index].card->setVisible(index >= first && index <= last);
    pageRange_->setText(registerName(first) + QString::fromUtf8("–") + registerName(last));
    previousPage_->setEnabled(page > 0); nextPage_->setEnabled(page + 1 < pageStarts_.size());
    rowsScroll_->verticalScrollBar()->setValue(0);
}
bool RegistersPanel::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::Resize || event->type() == QEvent::FontChange || event->type() == QEvent::StyleChange)
        for (const auto &row : rows_) if (watched == row.value && !rowGeometryPending_) {
            rowGeometryPending_ = true;
            QTimer::singleShot(0, this, [this] { updateRowGeometry(); });
            break;
        }
    if (detail_ && detailsScroll_ && watched == detail_ && event->type() == QEvent::Wheel) {
        // The inspection canvas is larger than the viewport. Let its scroll
        // area own wheels/touchpads rather than MathEditor's internal scroller.
        QApplication::sendEvent(detailsScroll_->viewport(), event); return true;
    }
    if (detailsScroll_ && watched == detailsScroll_->viewport() && event->type() == QEvent::Resize) updateDetailsGeometry();
    if (calculatorMode_ && watched == rowsScroll_->viewport() && event->type() == QEvent::Wheel) return true;
    for (int index = 0; index < 10; ++index) {
        const auto &row = rows_[index];
        if (watched != row.value && watched != row.card && watched != row.inspect && watched != row.preview) continue;
        if (calculatorMode_ && event->type() == QEvent::Wheel) return true;
        if (event->type() == QEvent::MouseButtonPress && watched != row.inspect) {
            if (calculatorMode_) pageAnchor_ = index;
            showDetails(index, true); return true;
        }
        if (event->type() == QEvent::FocusIn && (!calculatorMode_ || !pagingLayout_)) { if (calculatorMode_) pageAnchor_ = index; showDetails(index); }
        if (event->type() == QEvent::KeyPress) {
            auto *key = static_cast<QKeyEvent *>(event);
            int next = index;
            if (key->key() == Qt::Key_Down) next = qMin(9, index + 1);
            else if (key->key() == Qt::Key_Up) next = qMax(0, index - 1);
            else if (key->key() == Qt::Key_Home) next = 0;
            else if (key->key() == Qt::Key_End) next = 9;
            else if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) { if (calculatorMode_) pageAnchor_ = index; showDetails(index, true); return true; }
            else break;
            if (calculatorMode_) pageAnchor_ = next;
            showDetails(next); rows_[next].inspect->setFocus(Qt::OtherFocusReason);
            if (!calculatorMode_) rowsScroll_->ensureWidgetVisible(rows_[next].card); return true;
        }
        break;
    }
    return QWidget::eventFilter(watched, event);
}
void RegistersPanel::setInspectorVisible(bool visible) {
    inspectorVisible_ = visible; updatePaneLayout();
}
void RegistersPanel::updatePaneLayout() {
    if (pagingLayout_) return;
    pagingLayout_ = true;
    const bool wide = width() >= 620;
    title_->setVisible(headerVisible_ && !calculatorMode_); note_->setVisible(!calculatorMode_);
    inspectorTitle_->setVisible(!calculatorMode_); selectionSummary_->setVisible(!calculatorMode_);
    paneTabs_->setVisible(!calculatorMode_ && !wide);
    sections_->setDirection(calculatorMode_ ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
    bank_->setVisible(calculatorMode_ || wide || !inspectorVisible_);
    inspector_->setVisible(calculatorMode_ || wide || inspectorVisible_);
    bankTab_->setChecked(!inspectorVisible_); valueTab_->setChecked(inspectorVisible_);
    sections_->setStretch(0, calculatorMode_ ? 2 : 1); sections_->setStretch(1, calculatorMode_ ? 3 : 1);
    // Calculator retains the familiar bank-above-details arrangement.
    // Only its list pages; Desktop keeps its existing scrollable bank.
    const int available = calculatorMode_ || !wide ? width() - 20 : (width() - 32) / 2;
    const bool stackActions = clearSelected_->sizeHint().width() + clearAll_->sizeHint().width() + 5 > available;
    actions_->setDirection(stackActions ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
    confirmationActions_->setDirection(cancelClear_->sizeHint().width() + confirmClear_->sizeHint().width() + 5 > available
                                       ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
    // Word-wrapping labels give these panes height-for-width hints. Qt can
    // otherwise retain the prior horizontal layout's equal-height allocation
    // after switching modes. Explicit vertical minima keep the local scroll
    // surfaces and their captions disjoint at Calculator's minimum size.
    bank_->layout()->invalidate(); inspector_->layout()->invalidate();
    if (calculatorMode_) {
        rowsScroll_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        const int inspectorMinimum = inspector_->layout()->minimumSize().height();
        const auto margins = layout()->contentsMargins();
        const int available = qMax(0, height() - margins.top() - margins.bottom() -
            layout()->itemAt(0)->sizeHint().height() - layout()->spacing() - sections_->spacing());
        // First test all ten natural rows without paging controls. The two
        // header variants share the existing bank area and management footer.
        registerPager_->hide(); bankTitle_->show();
        const int rowSpacing = rowsScroll_->widget()->layout()->spacing();
        int allRowsHeight = rowSpacing * 9;
        for (const auto &row : rows_) {
            const auto rowMargins = row.card->layout()->contentsMargins();
            allRowsHeight += row.value->height() + rowMargins.top() + rowMargins.bottom();
        }
        bank_->layout()->invalidate();
        auto bankOverhead = [this] {
            auto *bankLayout = bank_->layout();
            int result = bankLayout->minimumSize().height() - rowsScroll_->minimumHeight();
            // The stored/selected summary and confirmation text may wrap.
            // Their actual width-dependent heights are part of the page budget.
            for (int index = 0; index < bankLayout->count(); ++index) {
                auto *item = bankLayout->itemAt(index);
                if (item->isEmpty() || item->widget() == rowsScroll_ || !item->hasHeightForWidth()) continue;
                result += qMax(0, item->heightForWidth(qMax(1, width() - 20)) - item->minimumSize().height());
            }
            return result;
        };
        int overhead = bankOverhead();
        const int maximumBank = qMax(0, available - inspectorMinimum);
        const bool pagesNeeded = allRowsHeight + overhead > maximumBank;
        registerPager_->setVisible(pagesNeeded); bankTitle_->setVisible(!pagesNeeded);
        bank_->layout()->invalidate();
        overhead = bankOverhead();
        const int bankHeight = qMax(overhead + 40, qMin(maximumBank, allRowsHeight + overhead));
        bank_->setMinimumHeight(0); bank_->setMaximumHeight(QWIDGETSIZE_MAX);
        inspector_->setMinimumHeight(inspectorMinimum);
        static_cast<RegisterSectionsLayout *>(sections_)->setCalculatorBankHeight(bankHeight);
        updateRegisterPages(qMax(40, bankHeight - overhead));
    } else {
        static_cast<RegisterSectionsLayout *>(sections_)->setCalculatorBankHeight(-1);
        registerPager_->hide(); bankTitle_->show(); pageStarts_.clear();
        rowsScroll_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        for (auto &row : rows_) { row.card->show(); row.value->show(); row.preview->hide(); }
        bank_->setMinimumHeight(0); bank_->setMaximumHeight(QWIDGETSIZE_MAX); inspector_->setMinimumHeight(0);
    }
    sections_->invalidate(); layout()->invalidate(); layout()->activate();
    if (calculatorMode_) {
        bank_->layout()->activate(); rowsScroll_->widget()->layout()->activate();
        updateRegisterPages(rowsScroll_->viewport()->height());
        // Resolve any one-pixel / style-dependent difference using the final
        // viewport before returning to Qt's paint loop.
        const bool paged = pageStarts_.size() > 1;
        if (registerPager_->isHidden() == paged) {
            registerPager_->setVisible(paged); bankTitle_->setVisible(!paged);
            bank_->layout()->activate();
            updateRegisterPages(rowsScroll_->viewport()->height());
        }
        rowsScroll_->widget()->layout()->activate();
        rowsScroll_->verticalScrollBar()->setValue(0);
    }
    updateDetailsGeometry();
    pagingLayout_ = false;
}

void RegistersPanel::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event); updatePaneLayout();
}
