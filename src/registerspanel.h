// SPDX-License-Identifier: MIT
#pragma once

#include "engine.h"
#include <QWidget>
#include <array>
#include <functional>

class QBoxLayout;
class QCheckBox;
class QFrame;
class QLabel;
class QPushButton;
class QPlainTextEdit;
class QScrollArea;
class MathEditor;
class QToolButton;

// Registers is an inspection/management surface. The owner obtains immutable
// stored values and formatted results on the engine worker; this widget never
// parses or evaluates canonical values and cannot insert or assign registers.
class RegistersPanel final : public QWidget {
public:
    explicit RegistersPanel(QWidget *parent = nullptr);
    void setValues(const QMap<QString, CalcStoredValue> &values,
                   const QMap<QString, CalcResult> &formatted);
    void setDisplayOptions(const CalcOptions &options);
    void setMathPresentation(int grouping, QChar decimalSeparator);
    void setCatalog(const QVector<CatalogEntry> &catalog);
    void retranslate(std::function<QString(const char *)> translate);
    void setHeaderVisible(bool visible);
    void setCalculatorMode(bool calculator);
    QStringList selectedRegisters() const;
    std::function<void(QStringList)> onClear;
    QSize minimumSizeHint() const override { return QSize(240, 280); }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QString trc(const char *text) const;
    void refreshRows();
    void updateControls();
    void showDetails(int index, bool reveal = false);
    void setInspectorVisible(bool visible);
    void updatePaneLayout();
    void presentValue(MathEditor *editor, const QString &name);
    void updateDetailsGeometry();
    void updateRowGeometry();
    void updateRegisterPages(int viewportHeight);
    void moveRegisterPage(int delta);
    void requestClear(bool all);
    struct Row { QFrame *card = nullptr; QCheckBox *selection = nullptr; QPushButton *inspect = nullptr; MathEditor *value = nullptr; QLabel *preview = nullptr; };
    std::array<Row, 10> rows_{};
    QVector<int> pageStarts_;
    int pageAnchor_ = 0;
    bool pagingLayout_ = false;
    QMap<QString, CalcStoredValue> values_;
    QMap<QString, CalcResult> formatted_;
    CalcOptions options_;
    std::function<QString(const char *)> translate_;
    int current_ = 0, grouping_ = 0;
    bool rowGeometryPending_ = false, inspectorVisible_ = false, calculatorMode_ = false, headerVisible_ = true;
    QChar decimalSeparator_ = '.';
    QLabel *title_ = nullptr, *note_ = nullptr, *mode_ = nullptr, *summary_ = nullptr;
    QLabel *bankTitle_ = nullptr, *inspectorTitle_ = nullptr, *selectionSummary_ = nullptr;
    QLabel *detailTitle_ = nullptr, *detailStatus_ = nullptr, *confirmText_ = nullptr;
    QLabel *canonicalTitle_ = nullptr, *empty_ = nullptr;
    QPlainTextEdit *canonical_ = nullptr;
    QPushButton *clearSelected_ = nullptr, *clearAll_ = nullptr, *copy_ = nullptr;
    QPushButton *confirmClear_ = nullptr, *cancelClear_ = nullptr;
    QPushButton *bankTab_ = nullptr, *valueTab_ = nullptr;
    QWidget *bank_ = nullptr, *inspector_ = nullptr, *paneTabs_ = nullptr, *normalActions_ = nullptr;
    QFrame *confirmation_ = nullptr;
    QWidget *registerPager_ = nullptr;
    QToolButton *previousPage_ = nullptr, *nextPage_ = nullptr;
    QLabel *pageRange_ = nullptr;
    QBoxLayout *sections_ = nullptr, *actions_ = nullptr, *confirmationActions_ = nullptr;
    QScrollArea *rowsScroll_ = nullptr, *detailsScroll_ = nullptr;
    MathEditor *detail_ = nullptr;
};
