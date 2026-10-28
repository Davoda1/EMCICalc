// SPDX-License-Identifier: MIT
#pragma once
#include "finance.h"
#include <QWidget>
#include <QHash>
#include <functional>

class QComboBox;
class QLabel;
class QPushButton;
class QScrollArea;
class QSplitter;
class QTabWidget;
class QTextBrowser;
class QVBoxLayout;
class QTableWidget;

// A native, engine-independent worksheet. Its owner serializes calculation and
// discards asynchronous results after onDraftChanged. User-entered numeric text
// stays intact until the calculation worker resolves expressions and registers.
class FinancePanel final : public QWidget {
public:
    explicit FinancePanel(QWidget *parent=nullptr);
    ~FinancePanel() override;
    QJsonObject saveState() const;
    void restoreState(const QJsonObject &state);
    QString toolId() const;
    void setTool(const QString &id);
    QJsonObject inputSnapshot() const;
    void setInput(const QJsonObject &input);
    void setResult(const Finance::Result &result);
    void setBusy(bool busy);
    void setCachedRate(const QString &rate,const QString &timestamp,const QString &source=QStringLiteral("cached"));
    void retranslate(std::function<QString(const char *)> translate);
    std::function<void(QString,QJsonObject)> onCalculate;
    std::function<void()> onDraftChanged;
    std::function<void(QString)> onInsert;
    std::function<void(QString,QString)> onUseCachedRate;
    QSize minimumSizeHint() const override { return QSize(240,300); }
protected:
    bool eventFilter(QObject *watched,QEvent *event) override;
private:
    friend class FinanceKeypadIntegrationTests;
    struct Editor { Finance::Field field; QWidget *widget=nullptr; };
    QString trc(const QString &text) const;
    const Finance::Tool *currentTool() const;
    void populateTools(const QString &selected={});
    void buildForm(const QJsonObject &input);
    void chooseTool();
    void draftChanged();
    void fieldChanged(const QString &key);
    void clearResult();
    void renderResult();
    void calculate();
    void updateActions();
    QJsonValue editorValue(const Editor &editor) const;
    void setEditorValue(const Editor &editor,const QJsonValue &value);
    void appendRow(QTableWidget *table,const QVector<Finance::Field> &columns,const QJsonObject &row={});
    QString tableCsv(int tab) const;
    QVector<Finance::Tool> tools_;
    QVector<Editor> editors_;
    QHash<QString,QJsonObject> drafts_;
    std::function<QString(const char *)> translate_;
    QString selectedTool_;
    bool changing_=false,busy_=false,hasResult_=false;
    Finance::Result result_;
    QComboBox *category_=nullptr,*tool_=nullptr;
    QScrollArea *fields_=nullptr;
    QSplitter *splitter_=nullptr;
    QTabWidget *results_=nullptr;
    QTextBrowser *summary_=nullptr;
    QLabel *status_=nullptr;
    QPushButton *example_=nullptr,*insert_=nullptr,*calculate_=nullptr,*cachedRate_=nullptr,*copySummary_=nullptr,*copyTable_=nullptr;
};
