// SPDX-License-Identifier: MIT
#include "financepanel.h"
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QTabWidget>
#include <QTextBrowser>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <algorithm>

namespace {
QString valueText(const QJsonValue &value) {
    if (value.isString()) return value.toString();
    if (value.isDouble()) return QString::number(value.toDouble(),'g',17);
    if (value.isBool()) return value.toBool()?QStringLiteral("true"):QStringLiteral("false");
    return {};
}
QJsonObject defaults(const Finance::Tool &tool) {
    QJsonObject input;
    for (const auto &field:tool.fields) {
        if (!field.initial.isUndefined()&&!field.initial.isNull()) input.insert(field.key,field.initial);
        else if (field.type=="table") input.insert(field.key,QJsonArray());
        else if (field.type=="bool") input.insert(field.key,false);
        else input.insert(field.key,QString());
    }
    return input;
}
void flexibleCombo(QComboBox *combo) {
    combo->setMinimumWidth(0);
    combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    combo->setMinimumContentsLength(1);
    combo->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
}
QLabel *wrapped(const QString &text) {
    auto *label=new QLabel(text); label->setWordWrap(true); label->setMinimumWidth(0);
    label->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
    label->setTextFormat(Qt::PlainText);
    return label;
}
void fitTableColumns(QTableWidget *table,bool includeRows) {
    table->ensurePolished();auto *header=table->horizontalHeader();header->ensurePolished();
    table->setWordWrap(false);table->setTextElideMode(Qt::ElideRight);
    // Sampling bounds the cost for long schedules. Every header is measured in
    // full; wide data remains exact in the model, tooltip and CSV export.
    header->setResizeContentsPrecision(80);
    if(includeRows)table->resizeColumnsToContents();
    const QFontMetrics metrics(header->font());
    for(int column=0;column<table->columnCount();++column) {
        const auto *item=table->horizontalHeaderItem(column);
        const int caption=item?metrics.horizontalAdvance(item->text())+36:110;
        table->setColumnWidth(column,std::max(caption,includeRows?std::clamp(table->columnWidth(column),110,420):150));
    }
}
QString csvCell(QString text) {
    text.replace('"',QStringLiteral("\"\""));
    return QStringLiteral("\"")+text+QStringLiteral("\"");
}
// Table values are stored as original strings, including all precision and
// expressions. Choice cells use a native editor without translating tokens.
class FieldDelegate final : public QStyledItemDelegate {
public:
    QVector<Finance::Field> columns;
    std::function<QString(const QString &)> translate;
    explicit FieldDelegate(QVector<Finance::Field> fields,QObject *parent):QStyledItemDelegate(parent),columns(std::move(fields)) {}
    void paint(QPainter *painter,const QStyleOptionViewItem &option,const QModelIndex &index) const override {
        const auto &field=columns.at(index.column());
        if(field.type!="choice"&&field.type!="bool"){QStyledItemDelegate::paint(painter,option,index);return;}
        QStyleOptionViewItem translated(option);initStyleOption(&translated,index);
        if(translate)translated.text=translate(index.data(Qt::DisplayRole).toString());
        const auto *widget=option.widget;(widget?widget->style():QApplication::style())->drawControl(QStyle::CE_ItemViewItem,&translated,painter,widget);
    }
    QWidget *createEditor(QWidget *parent,const QStyleOptionViewItem &option,const QModelIndex &index) const override {
        const auto &field=columns.at(index.column());
        if (field.type=="choice"||field.type=="bool") {
            auto *box=new QComboBox(parent);
            const auto choices=field.type=="bool"?QStringList{"false","true"}:field.choices;
            box->addItem(QString(),QString());
            for (const auto &choice:choices) box->addItem(translate?translate(choice):choice,choice);
            return box;
        }
        return QStyledItemDelegate::createEditor(parent,option,index);
    }
    void setEditorData(QWidget *editor,const QModelIndex &index) const override {
        if (auto *box=qobject_cast<QComboBox *>(editor)) box->setCurrentIndex(std::max(0,box->findData(index.data(Qt::EditRole))));
        else QStyledItemDelegate::setEditorData(editor,index);
    }
    void setModelData(QWidget *editor,QAbstractItemModel *model,const QModelIndex &index) const override {
        if (auto *box=qobject_cast<QComboBox *>(editor)) model->setData(index,box->currentData(),Qt::EditRole);
        else QStyledItemDelegate::setModelData(editor,model,index);
    }
};
}

FinancePanel::FinancePanel(QWidget *parent):QWidget(parent),tools_(Finance::tools()) {
    setObjectName("financePanel"); setMinimumWidth(0);
    auto *layout=new QVBoxLayout(this);layout->setContentsMargins(0,0,0,0);layout->setSpacing(6);
    category_=new QComboBox;category_->setObjectName("financeCategory");flexibleCombo(category_);
    tool_=new QComboBox;tool_->setObjectName("financeTool");flexibleCombo(tool_);
    layout->addWidget(category_);layout->addWidget(tool_);
    splitter_=new QSplitter(Qt::Vertical);splitter_->setObjectName("financeSplitter");
    splitter_->setChildrenCollapsible(false);splitter_->setHandleWidth(5);layout->addWidget(splitter_,1);
    auto *inputPane=new QWidget;inputPane->setMinimumHeight(100);auto *inputLayout=new QVBoxLayout(inputPane);
    inputLayout->setContentsMargins(0,0,0,0);inputLayout->setSpacing(5);
    fields_=new QScrollArea;fields_->setObjectName("financeFieldsScroll");fields_->setWidgetResizable(true);
    fields_->setFrameShape(QFrame::NoFrame);fields_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    fields_->setMinimumSize(0,40);inputLayout->addWidget(fields_,1);
    auto *actions=new QHBoxLayout;actions->setSpacing(5);
    example_=new QPushButton;example_->setObjectName("financeLoadExample");
    insert_=new QPushButton;insert_->setObjectName("financeInsertFunction");
    for (auto *button:{example_,insert_}) { button->setMinimumWidth(0);button->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);actions->addWidget(button); }
    inputLayout->addLayout(actions);
    cachedRate_=new QPushButton;cachedRate_->setObjectName("financeUseCachedRate");inputLayout->addWidget(cachedRate_);
    calculate_=new QPushButton;calculate_->setObjectName("financeCalculate");inputLayout->addWidget(calculate_);
    splitter_->addWidget(inputPane);
    auto *outputPane=new QWidget;outputPane->setMinimumHeight(100);auto *outputLayout=new QVBoxLayout(outputPane);
    outputLayout->setContentsMargins(0,0,0,0);outputLayout->setSpacing(5);
    results_=new QTabWidget;results_->setObjectName("financeResults");results_->setMinimumSize(0,40);
    results_->setUsesScrollButtons(true);results_->setElideMode(Qt::ElideRight);
    summary_=new QTextBrowser;summary_->setObjectName("financeSummary");summary_->setOpenExternalLinks(false);
    summary_->setMinimumSize(0,0);results_->addTab(summary_,QString());outputLayout->addWidget(results_,1);
    auto *copyActions=new QHBoxLayout;copySummary_=new QPushButton;copySummary_->setObjectName("financeCopySummary");
    copyTable_=new QPushButton;copyTable_->setObjectName("financeCopyTable");
    for (auto *button:{copySummary_,copyTable_}) {button->setMinimumWidth(0);button->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);copyActions->addWidget(button);}
    outputLayout->addLayout(copyActions);splitter_->addWidget(outputPane);splitter_->setStretchFactor(0,3);splitter_->setStretchFactor(1,2);
    status_=wrapped(QString());status_->setObjectName("financeStatus");status_->setAccessibleName("Calculation status");layout->addWidget(status_);
    connect(category_,&QComboBox::currentIndexChanged,this,[this]{if(!changing_)populateTools(selectedTool_);});
    connect(tool_,&QComboBox::currentIndexChanged,this,[this]{if(!changing_)chooseTool();});
    connect(example_,&QPushButton::clicked,this,[this]{if(const auto *item=currentTool())setInput(item->example);});
    connect(insert_,&QPushButton::clicked,this,[this]{if(const auto *item=currentTool())if(onInsert&&!item->expression.isEmpty())onInsert(item->expression.contains('(')?item->expression.left(item->expression.indexOf('(')+1):item->expression);});
    connect(calculate_,&QPushButton::clicked,this,[this]{calculate();});
    connect(cachedRate_,&QPushButton::clicked,this,[this]{
        if(!onUseCachedRate)return;const auto input=inputSnapshot();
        onUseCachedRate(input.value("source_currency").toString(),input.value("destination_currency").toString());
    });
    connect(copySummary_,&QPushButton::clicked,this,[this]{if(hasResult_)QApplication::clipboard()->setText(summary_->toPlainText());});
    connect(copyTable_,&QPushButton::clicked,this,[this]{const QString csv=tableCsv(results_->currentIndex());if(!csv.isEmpty())QApplication::clipboard()->setText(csv);});
    connect(results_,&QTabWidget::currentChanged,this,[this]{updateActions();});
    retranslate({});
}
FinancePanel::~FinancePanel() {
    // A live delegate editor can commit while QWidget destroys child widgets.
    // Stop those signals before this class's draft/result members are destroyed.
    changing_=true;
    for(auto *object:findChildren<QObject *>())QObject::disconnect(object,nullptr,this,nullptr);
}
QString FinancePanel::trc(const QString &text) const {
    if(!translate_)return text;const QByteArray bytes=text.toUtf8();const QString translated=translate_(bytes.constData());
    if(translated!=text)return translated;
    // Refinancing schemas compose labels from a stable loan-side prefix and a
    // shared field caption. Translate each part without duplicating every key.
    for(const auto &prefix:QStringList{"Existing — ","Replacement — ","Keep existing — ","Refinance — "})
        if(text.startsWith(prefix))return translate_(prefix.toUtf8().constData())+trc(text.mid(prefix.size()));
    const int separator=text.indexOf(QStringLiteral(" — "));
    if(separator>0)return trc(text.left(separator))+QStringLiteral(" — ")+trc(text.mid(separator+3));
    static const QRegularExpression currencySuffix(QStringLiteral("^(.*) \\(([A-Z]{3})(?: per ([A-Z]{3}))?\\)$"));
    const auto currency=currencySuffix.match(text);
    if(currency.hasMatch())return currency.captured(3).isEmpty()?trc("%1 (%2)").arg(trc(currency.captured(1)),currency.captured(2)):trc("%1 (%2 per %3)").arg(trc(currency.captured(1)),currency.captured(2),currency.captured(3));
    const QString fractionSuffix=QStringLiteral(" (fraction)");
    if(text.endsWith(fractionSuffix))return trc(text.chopped(fractionSuffix.size()))+translate_(fractionSuffix.toUtf8().constData());
    return text;
}
QString FinancePanel::toolId() const {return selectedTool_;}
const Finance::Tool *FinancePanel::currentTool() const {for(const auto &item:tools_)if(item.id==selectedTool_)return &item;return nullptr;}
void FinancePanel::populateTools(const QString &selected) {
    changing_=true;tool_->clear();const QString category=category_->currentData().toString();
    for(const auto &item:tools_)if(category.isEmpty()||item.category==category)tool_->addItem(trc(item.title),item.id);
    tool_->setCurrentIndex(std::max(0,tool_->findData(selected)));changing_=false;chooseTool();
}
void FinancePanel::setTool(const QString &id) {
    for(const auto &item:tools_)if(item.id==id) {
        changing_=true;category_->setCurrentIndex(0);changing_=false;populateTools(id);return;
    }
}
void FinancePanel::chooseTool() {
    const QString next=tool_->currentData().toString();if(next==selectedTool_)return;
    if(!selectedTool_.isEmpty())drafts_[selectedTool_]=inputSnapshot();
    selectedTool_=next;
    if(const auto *item=currentTool())buildForm(drafts_.value(next,defaults(*item)));
    draftChanged();
}
void FinancePanel::buildForm(const QJsonObject &input) {
    const auto *item=currentTool();if(!item)return;
    changing_=true;editors_.clear();
    if(auto *old=fields_->widget())old->hide();
    delete fields_->takeWidget();auto *container=new QWidget;container->setMinimumWidth(0);
    auto *form=new QVBoxLayout(container);form->setContentsMargins(2,2,8,4);form->setSpacing(5);
    auto *description=wrapped(trc(item->description));description->setObjectName("financeDescription");form->addWidget(description);
    {
        auto *toggle=new QToolButton;toggle->setText(trc("Assumptions and conventions"));toggle->setCheckable(true);toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);toggle->setArrowType(Qt::RightArrow);
        auto *assumptions=wrapped(QString());assumptions->setObjectName("financeExpressionHelp");QStringList translated;
        translated<<trc("Numeric fields accept expressions and R0–R9. Rates are fractional unless the field explicitly says percent; 0.20 and 20% both mean twenty percent.");
        for(const auto &line:item->assumptions)translated<<QStringLiteral("• ")+trc(line);
        assumptions->setText(translated.join('\n'));assumptions->hide();form->addWidget(toggle);form->addWidget(assumptions);
        connect(toggle,&QToolButton::toggled,this,[toggle,assumptions](bool show){assumptions->setVisible(show);toggle->setArrowType(show?Qt::DownArrow:Qt::RightArrow);});
    }
    for(const auto &field:item->fields) {
        auto *label=wrapped(trc(field.label));QFont font=label->font();font.setBold(true);label->setFont(font);form->addSpacing(5);form->addWidget(label);
        QWidget *editor=nullptr;
        if(field.type=="table") {
            auto *table=new QTableWidget(0,field.columns.size());table->setObjectName("financeInput_"+field.key);
            QStringList headers;for(const auto &column:field.columns)headers<<trc(column.label);table->setHorizontalHeaderLabels(headers);
            table->setMinimumWidth(0);table->setMinimumHeight(128);table->setMaximumHeight(210);table->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
            table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);table->horizontalHeader()->setDefaultSectionSize(150);table->horizontalHeader()->setMinimumSectionSize(80);
            table->horizontalHeader()->setStretchLastSection(true);table->setAlternatingRowColors(true);table->setSelectionBehavior(QAbstractItemView::SelectRows);
            table->setSelectionMode(QAbstractItemView::ExtendedSelection);
            auto *delegate=new FieldDelegate(field.columns,table);delegate->translate=[this](const QString &text){return trc(text);};table->setItemDelegate(delegate);
            for(int column=0;column<field.columns.size();++column)table->horizontalHeaderItem(column)->setToolTip(trc(field.columns[column].label)+(field.columns[column].help.isEmpty()?QString():QStringLiteral("\n")+trc(field.columns[column].help)));
            form->addWidget(table);editor=table;
            auto *rowActions=new QHBoxLayout;auto *add=new QPushButton(trc("Add row"));auto *remove=new QPushButton(trc("Remove selected"));
            add->setObjectName("financeAddRow_"+field.key);remove->setObjectName("financeRemoveRow_"+field.key);
            for(auto *button:{add,remove}){button->setMinimumWidth(0);button->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);rowActions->addWidget(button);}form->addLayout(rowActions);
            connect(add,&QPushButton::clicked,this,[this,table,columns=field.columns]{changing_=true;appendRow(table,columns);changing_=false;draftChanged();table->setCurrentCell(table->rowCount()-1,0);table->editItem(table->item(table->rowCount()-1,0));});
            connect(remove,&QPushButton::clicked,this,[this,table]{QSet<int> rows;for(const auto *cell:table->selectedItems())rows.insert(cell->row());auto ordered=rows.values();std::sort(ordered.begin(),ordered.end(),std::greater<int>());if(ordered.isEmpty())return;changing_=true;for(int row:ordered)table->removeRow(row);changing_=false;draftChanged();});
            connect(table,&QTableWidget::itemChanged,this,[this]{draftChanged();});
        } else if(field.type=="choice") {
            auto *box=new QComboBox;flexibleCombo(box);box->addItem(trc("Choose…"),QString());
            for(const auto &choice:field.choices)box->addItem(trc(choice),choice);
            box->installEventFilter(this);form->addWidget(box);editor=box;
            connect(box,&QComboBox::currentIndexChanged,this,[this,key=field.key]{fieldChanged(key);});
        } else if(field.type=="bool") {
            auto *check=new QCheckBox(trc("Enabled"));form->addWidget(check);editor=check;
            connect(check,&QCheckBox::toggled,this,[this]{draftChanged();});
        } else {
            auto *line=new QLineEdit;line->setClearButtonEnabled(true);line->setMinimumWidth(0);
            if(field.type=="date")line->setPlaceholderText(QStringLiteral("YYYY-MM-DD"));
            else if(field.type=="number"||field.type=="integer"||field.type=="int")line->setPlaceholderText(trc("Value or expression"));
            form->addWidget(line);editor=line;
            connect(line,&QLineEdit::textChanged,this,[this,key=field.key]{fieldChanged(key);});
            connect(line,&QLineEdit::returnPressed,this,[this]{calculate();});
        }
        editor->setObjectName("financeInput_"+field.key);editor->setAccessibleName(trc(field.label));editor->setToolTip(trc(field.help));label->setBuddy(editor);
        if(!field.help.isEmpty()){auto *help=wrapped(trc(field.help));form->addWidget(help);}
        editors_.append({field,editor});setEditorValue(editors_.last(),input.value(field.key));
    }
    form->addStretch();fields_->setWidget(container);
    for(const auto &editor:editors_)if(auto *table=qobject_cast<QTableWidget *>(editor.widget))fitTableColumns(table,false);
    changing_=false;updateActions();
}
void FinancePanel::appendRow(QTableWidget *table,const QVector<Finance::Field> &columns,const QJsonObject &row) {
    const int index=table->rowCount();table->insertRow(index);
    for(int column=0;column<columns.size();++column) {
        const auto &field=columns[column];auto *cell=new QTableWidgetItem(valueText(row.contains(field.key)?row.value(field.key):field.initial));
        cell->setToolTip(trc(field.help));table->setItem(index,column,cell);
    }
}
QJsonValue FinancePanel::editorValue(const Editor &editor) const {
    if(auto *line=qobject_cast<QLineEdit *>(editor.widget))return line->text();
    if(auto *box=qobject_cast<QComboBox *>(editor.widget))return box->currentData().toString();
    if(auto *check=qobject_cast<QCheckBox *>(editor.widget))return check->isChecked();
    if(auto *table=qobject_cast<QTableWidget *>(editor.widget)) {
        QJsonArray rows;for(int row=0;row<table->rowCount();++row) {QJsonObject value;for(int col=0;col<editor.field.columns.size();++col){const auto &field=editor.field.columns[col];const auto *cell=table->item(row,col);QString text=cell?cell->text():QString();
            // A calculation, save or worksheet switch can occur before the
            // current delegate commits. Snapshot that pending edit verbatim.
            if(table->currentRow()==row&&table->currentColumn()==col){
                if(auto *line=table->viewport()->findChild<QLineEdit*>();line&&!line->isHidden())text=line->text();
                else if(auto *box=table->viewport()->findChild<QComboBox*>();box&&!box->isHidden())text=box->currentData().toString();
            }
            value.insert(field.key,field.type=="bool"?QJsonValue(text=="true"):QJsonValue(text));}rows.append(value);}return rows;
    }
    return {};
}
void FinancePanel::setEditorValue(const Editor &editor,const QJsonValue &value) {
    if(auto *line=qobject_cast<QLineEdit *>(editor.widget))line->setText(valueText(value));
    else if(auto *box=qobject_cast<QComboBox *>(editor.widget))box->setCurrentIndex(std::max(0,box->findData(valueText(value))));
    else if(auto *check=qobject_cast<QCheckBox *>(editor.widget))check->setChecked(value.isBool()?value.toBool():valueText(value)=="true");
    else if(auto *table=qobject_cast<QTableWidget *>(editor.widget)){table->setRowCount(0);for(const auto &row:value.toArray())appendRow(table,editor.field.columns,row.toObject());}
}
QJsonObject FinancePanel::inputSnapshot() const {QJsonObject input;for(const auto &editor:editors_)input.insert(editor.field.key,editorValue(editor));return input;}
void FinancePanel::setInput(const QJsonObject &input) {
    const auto *item=currentTool();if(!item)return;auto combined=defaults(*item);for(auto it=input.begin();it!=input.end();++it)combined.insert(it.key(),it.value());
    changing_=true;for(const auto &editor:editors_)setEditorValue(editor,combined.value(editor.field.key));changing_=false;draftChanged();
}
QJsonObject FinancePanel::saveState() const {
    QJsonObject savedDrafts;
    for(auto it=drafts_.cbegin();it!=drafts_.cend();++it)savedDrafts.insert(it.key(),it.value());
    if(!selectedTool_.isEmpty())savedDrafts.insert(selectedTool_,inputSnapshot());
    return {{"schema",1},{"tool",selectedTool_},{"category",category_->currentData().toString()},{"drafts",savedDrafts}};
}
void FinancePanel::restoreState(const QJsonObject &state) {
    if(state.value("schema").toInt()!=1 || !state.value("drafts").isObject())return;
    QHash<QString,QJsonObject> restored;
    const auto saved=state.value("drafts").toObject();
    QString selected=selectedTool_;
    for(const auto &tool:tools_) {
        if(tool.id==state.value("tool").toString())selected=tool.id;
        if(!saved.value(tool.id).isObject())continue;
        const auto draft=saved.value(tool.id).toObject();QJsonObject knownFields;
        for(const auto &field:tool.fields)if(draft.contains(field.key)) {
            const auto value=draft.value(field.key);
            if(field.type=="table") {
                if(!value.isArray())continue;
                QJsonArray rows;
                for(const auto &row:value.toArray())if(row.isObject()) {
                    QJsonObject cells;const auto rowObject=row.toObject();
                    for(const auto &column:field.columns)if(rowObject.contains(column.key))cells.insert(column.key,rowObject.value(column.key));
                    rows.append(cells);
                }
                knownFields.insert(field.key,rows);
            } else if(value.isString() || value.isBool() || value.isDouble())knownFields.insert(field.key,value);
        }
        restored.insert(tool.id,knownFields);
    }
    drafts_=std::move(restored);changing_=true;
    int categoryIndex=category_->findData(state.value("category").toString());
    // A malformed category must not hide a valid selected worksheet.
    QString selectedCategory;for(const auto &tool:tools_)if(tool.id==selected)selectedCategory=tool.category;
    if(categoryIndex<0 || (categoryIndex>0 && category_->itemData(categoryIndex).toString()!=selectedCategory))categoryIndex=0;
    category_->setCurrentIndex(categoryIndex);tool_->clear();
    const QString category=category_->currentData().toString();
    for(const auto &tool:tools_)if(category.isEmpty() || tool.category==category)tool_->addItem(trc(tool.title),tool.id);
    tool_->setCurrentIndex(qMax(0,tool_->findData(selected)));selectedTool_=tool_->currentData().toString();
    if(const auto *tool=currentTool())buildForm(drafts_.value(selectedTool_,defaults(*tool)));
    changing_=false;draftChanged();
}
void FinancePanel::fieldChanged(const QString &key) {
    if(changing_)return;
    if(selectedTool_=="fxnet"&&(key=="rate"||key=="rate_direction"||key=="source_currency"||key=="destination_currency")) {
        const QString source=inputSnapshot().value("rate_source").toString();
        const bool cached=source!="user-entered"&&!source.isEmpty();changing_=true;
        for(const auto &editor:editors_) {
            if(editor.field.key=="rate_source")setEditorValue(editor,QStringLiteral("user-entered"));
            else if(editor.field.key=="rate_timestamp")setEditorValue(editor,QString());
            else if(cached&&(key=="source_currency"||key=="destination_currency")&&editor.field.key=="rate")setEditorValue(editor,QString());
        }
        changing_=false;
    }
    draftChanged();
}
void FinancePanel::draftChanged() {if(changing_)return;clearResult();setBusy(false);if(onDraftChanged)onDraftChanged();}
void FinancePanel::clearResult() {
    hasResult_=false;result_={};while(results_->count()>1){auto *widget=results_->widget(1);results_->removeTab(1);delete widget;}
    summary_->setPlainText(trc("Enter the worksheet inputs, then calculate. Examples are loaded only when you choose Load example."));status_->clear();updateActions();
}
void FinancePanel::calculate() {if(busy_||!currentTool())return;clearResult();if(onCalculate)onCalculate(selectedTool_,inputSnapshot());}
void FinancePanel::setBusy(bool busy) {busy_=busy;calculate_->setText(trc(busy?"Calculating…":"Calculate"));calculate_->setEnabled(!busy&&!selectedTool_.isEmpty());if(busy)status_->setText(trc("Calculating…"));updateActions();}
void FinancePanel::setResult(const Finance::Result &result) {setBusy(false);result_=result;hasResult_=true;renderResult();}
void FinancePanel::renderResult() {
    const QHash<QString,QString> statusLabels{{"horizon","Simulation horizon reached"},{"horizon_limit","Simulation horizon reached"},{"non_repaying","Non-repaying configuration"},{"remaining_balance","Remaining balance"},{"non_unique","Multiple or non-unique solutions"},{"no_exhaustion","No exhaustion"},{"shortfall","Cash shortfall"},{"unreachable","Target unreachable"},{"non_convergence","No converged solution"},{"insufficient_funds","Insufficient funds"},{"unsupported","Unsupported calculation"},{"invalid","Check the worksheet inputs"}};
    const bool regular=result_.status=="ok"||result_.status=="converged";
    const QString statusText=trc(statusLabels.value(result_.status,regular?(result_.ok?"Calculation complete":"Check the worksheet inputs"):result_.status));
    while(results_->count()>1){auto *widget=results_->widget(1);results_->removeTab(1);delete widget;}
    QString html=QStringLiteral("<h3>%1</h3>").arg(trc(currentTool()?currentTool()->title:QString("Result")).toHtmlEscaped());
    if(!result_.message.isEmpty())html+=QStringLiteral("<p>%1</p>").arg(trc(result_.message).toHtmlEscaped());
    if(!result_.ok||!regular)html+=QStringLiteral("<p><b>%1:</b> %2</p>").arg(trc("Status").toHtmlEscaped(),statusText.toHtmlEscaped());
    html+=QStringLiteral("<table cellspacing=\"6\">");
    for(const auto &item:result_.summary)html+=QStringLiteral("<tr><td>%1</td><td><b>%2</b></td></tr>").arg(trc(item.first).toHtmlEscaped(),item.second.toHtmlEscaped());
    html+=QStringLiteral("</table>");
    if(result_.summary.isEmpty()&&!result_.scalar.isEmpty())html+=QStringLiteral("<p><b>%1</b></p>").arg(result_.scalar.toHtmlEscaped());
    if(!result_.warnings.isEmpty()){html+=QStringLiteral("<h4>%1</h4><ul>").arg(trc("Notes and warnings").toHtmlEscaped());for(const auto &warning:result_.warnings)html+=QStringLiteral("<li>%1</li>").arg(trc(warning).toHtmlEscaped());html+=QStringLiteral("</ul>");}
    summary_->setHtml(html);
    for(const auto &data:result_.tables) {
        auto *table=new QTableWidget(data.rows.size(),data.headers.size());table->setObjectName("financeResultTable");table->setMinimumSize(0,0);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);table->setAlternatingRowColors(true);table->setSelectionBehavior(QAbstractItemView::SelectItems);
        QStringList headers;for(const auto &header:data.headers)headers<<trc(header);table->setHorizontalHeaderLabels(headers);
        for(int row=0;row<data.rows.size();++row)for(int column=0;column<data.headers.size();++column){auto *cell=new QTableWidgetItem(data.rows[row].value(column));cell->setToolTip(cell->text());cell->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);table->setItem(row,column,cell);}
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);table->horizontalHeader()->setDefaultSectionSize(145);table->horizontalHeader()->setMinimumSectionSize(75);
        results_->addTab(table,trc(data.title));
        for(int column=0;column<headers.size();++column)table->horizontalHeaderItem(column)->setToolTip(headers[column]);
        fitTableColumns(table,true);
    }
    results_->setCurrentIndex(0);status_->setText(statusText);status_->setToolTip(trc(result_.status));updateActions();
}
QString FinancePanel::tableCsv(int tab) const {
    if(!hasResult_||tab<1||tab>result_.tables.size())return {};const auto &table=result_.tables[tab-1];QStringList lines,headers;
    for(const auto &header:table.headers)headers<<csvCell(trc(header));lines<<headers.join(',');
    for(const auto &row:table.rows){QStringList cells;for(const auto &cell:row)cells<<csvCell(cell);lines<<cells.join(',');}
    return lines.join(QStringLiteral("\r\n"))+QStringLiteral("\r\n");
}
void FinancePanel::setCachedRate(const QString &rate,const QString &timestamp,const QString &source) {
    if(selectedTool_!="fxnet")return;
    changing_=true;for(const auto &editor:editors_)if(editor.field.key=="rate")setEditorValue(editor,rate);else if(editor.field.key=="rate_timestamp")setEditorValue(editor,timestamp);else if(editor.field.key=="rate_source")setEditorValue(editor,source);else if(editor.field.key=="rate_direction")setEditorValue(editor,QStringLiteral("destination-per-source"));
    changing_=false;draftChanged();status_->setText(trc("Cached rate loaded")+(timestamp.isEmpty()?QString():QStringLiteral(" · ")+timestamp));
}
void FinancePanel::updateActions() {
    const auto *item=currentTool();example_->setEnabled(item&&!item->example.isEmpty());insert_->setVisible(item&&!item->expression.isEmpty());
    cachedRate_->setVisible(item&&item->id=="fxnet");copySummary_->setEnabled(hasResult_);copyTable_->setEnabled(hasResult_&&results_->currentIndex()>0);
}
void FinancePanel::retranslate(std::function<QString(const char *)> translate) {
    const auto input=inputSnapshot();translate_=std::move(translate);const QString previous=category_->currentData().toString();changing_=true;
    category_->clear();category_->addItem(trc("All finance categories"),QString());QStringList categories;
    for(const auto &item:tools_)if(!categories.contains(item.category))categories<<item.category;
    for(const auto &category:categories)category_->addItem(trc(category),category);
    category_->setCurrentIndex(std::max(0,category_->findData(previous)));changing_=false;
    category_->setAccessibleName(trc("Finance category"));tool_->setAccessibleName(trc("Finance worksheet"));
    example_->setText(trc("Load example"));insert_->setText(trc("Insert function"));cachedRate_->setText(trc("Use cached exchange rate"));
    copySummary_->setText(trc("Copy summary"));copyTable_->setText(trc("Copy table CSV"));results_->setTabText(0,trc("Summary"));
    const bool hadTool=!selectedTool_.isEmpty();populateTools(selectedTool_);if(hadTool)buildForm(input);
    setBusy(busy_);if(hasResult_)renderResult();else clearResult();
}
bool FinancePanel::eventFilter(QObject *watched,QEvent *event) {
    if(event->type()==QEvent::Wheel) {
        auto *wheel=static_cast<QWheelEvent *>(event);auto *bar=fields_->verticalScrollBar();
        const int delta=!wheel->pixelDelta().isNull()?wheel->pixelDelta().y():wheel->angleDelta().y()/120*bar->singleStep()*3;
        bar->setValue(bar->value()-delta);event->accept();return true;
    }
    return QWidget::eventFilter(watched,event);
}
