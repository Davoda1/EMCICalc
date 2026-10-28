// SPDX-License-Identifier: MIT
#include "calculatorpanel.h"
#include "recurringlabel.h"
#include "argumentguide.h"
#include "fontscale.h"
#include "keyartwork.h"
#include "mathcaption.h"
#include <algorithm>
#include <cmath>

namespace {
// These established key labels are composites, not aliases for a lone math
// symbol. Measure the Latin ink and vector marks separately so fallback glyph
// bearings cannot remove dx or make the two S/D gaps unequal.
struct KeyCaption {
    MathCaption::Layout layout;
    QList<QRectF> parts;
};
bool hasKeyCaption(const QString &label) { return label=="∫ dx" || label=="S⇔D"; }
KeyCaption keyCaption(const QString &label,const QFont &font) {
    KeyCaption result;if(!hasKeyCaption(label))return result;
    const QFontMetricsF metrics(font);const qreal cap=metrics.capHeight();
    const qreal gap=qMax(1.5,cap*.23);qreal x=0;
    const auto addText=[&](const QString &text) {
        const QRectF ink=metrics.tightBoundingRect(text);
        const QPointF baseline(x-ink.left(),-ink.center().y());
        const QRectF placed=ink.translated(baseline);
        result.layout.text.append({baseline,font,text});result.parts.append(placed);
        result.layout.bounds=result.layout.bounds.united(placed);x=placed.right()+gap;
    };
    const auto addMark=[&](QPainterPath mark) {
        const QRectF ink=mark.boundingRect();mark.translate(x-ink.left(),-ink.center().y());
        const QRectF placed=mark.boundingRect();result.layout.marks.addPath(mark);
        result.parts.append(placed);result.layout.bounds=result.layout.bounds.united(placed);x=placed.right()+gap;
    };
    if(label=="∫ dx") {addMark(MathCaption::path("integrate(",font));addText("dx");}
    else {
        addText("S");
        // U+21D4's two arrowheads AND paired horizontal strokes. The shafts
        // join each head geometrically; no arrow glyph or font is required.
        const qreal width=cap*1.35,height=cap*.64,head=height*.48,shaft=height*.17;
        const qreal join=head*(1-shaft/(height*.5));QPainterPath arrow;
        arrow.moveTo(head,-height/2);arrow.lineTo(0,0);arrow.lineTo(head,height/2);
        arrow.moveTo(width-head,-height/2);arrow.lineTo(width,0);arrow.lineTo(width-head,height/2);
        arrow.moveTo(join,-shaft);arrow.lineTo(width-join,-shaft);
        arrow.moveTo(join,shaft);arrow.lineTo(width-join,shaft);
        addMark(MathCaption::stroked(arrow,qMax(.65,cap*.075)));addText("D");
    }
    return result;
}
qreal keyCaptionWidth(const QString &label,const QFont &font) {
    return hasKeyCaption(label)?keyCaption(label,font).layout.bounds.width():QFontMetricsF(font).horizontalAdvance(label);
}
bool paintKeyCaption(QPainter &p,const QRectF &area,const QFont &font,const QString &label,const QColor &ink,QObject *owner=nullptr,const QByteArray &property={}) {
    const auto caption=keyCaption(label,font);const auto bounds=caption.layout.bounds;
    if(bounds.isEmpty()||area.isEmpty())return false;
    const qreal scale=qMin(1.,qMin(area.width()/bounds.width(),area.height()/bounds.height()));
    QTransform transform;transform.translate(area.center().x(),area.center().y());transform.scale(scale,scale);transform.translate(-bounds.center().x(),-bounds.center().y());
    p.save();p.setRenderHint(QPainter::Antialiasing);p.setTransform(transform,true);MathCaption::draw(p,caption.layout,ink);p.restore();
    if(owner&&!property.isEmpty()) {
        QVariantList parts;for(const auto &part:caption.parts)parts.append(transform.mapRect(part));
        owner->setProperty(property.constData(),parts);
    }
    return true;
}
QString keyCaptionImageHtml(const QString &label,const QString &description,const QColor &ink,int pixels,qreal ratio) {
    if(!hasKeyCaption(label))return {};
    QFont font=qApp->font();font.setPixelSize(pixels);font.setBold(true);
    const auto bounds=keyCaption(label,font).layout.bounds;
    const int height=qMax(3,pixels),width=qCeil((height-2)*bounds.width()/bounds.height())+4;ratio=qMax(1.,ratio);
    QImage image(QSize(qCeil(width*ratio),qCeil(height*ratio)),QImage::Format_ARGB32_Premultiplied);image.setDevicePixelRatio(ratio);image.fill(Qt::transparent);
    {QPainter p(&image);paintKeyCaption(p,QRectF(2,1,width-4,height-2),font,label,ink);}
    QByteArray png;QBuffer buffer(&png);buffer.open(QIODevice::WriteOnly);image.save(&buffer,"PNG");
    return QString("<img data-key-caption=\"%1\" alt=\"%2\" width=\"%3\" height=\"%4\" valign=\"middle\" src=\"data:image/png;base64,%5\">")
        .arg(label,description.toHtmlEscaped()).arg(width).arg(height).arg(QString::fromLatin1(png.toBase64()));
}
bool paintArrowSvg(QPainter &p,const QRectF &area,bool right,const QColor &color) {
    const qreal height=qMin(area.height()*.60,20.0);
    return KeyArtwork::paint(p,QRectF(area.left(),area.center().y()-height/2,area.width(),height),
                             right?"arrow-right":"arrow-left",color);
}
bool paintScriptCaption(QPainter &p,const QRectF &area,const QFont &font,const QString &text,const QColor &color) {
    struct Caption {const char *label,*base,*script;bool below;};
    static const Caption captions[]={{"10ˣ","10","x",false},{"2ˣ","2","x",false},{"eˣ","e","x",false},{"xʸ","x","y",false},
        {"x²","x","2",false},{"x³","x","3",false},{"x⁻¹","x","-1",false},
        {"sin⁻¹","sin","-1",false},{"cos⁻¹","cos","-1",false},{"tan⁻¹","tan","-1",false},
        {"log₁₀","log","10",true},{"logₙ","log","n",true}};
    for(const auto &caption:captions)if(text==QString::fromUtf8(caption.label)) {
        QFont base=font;QFont script=font;script.setPixelSize(qMax(5,int(font.pixelSize()*.68)));
        const QFontMetricsF bm(base),sm(script);const QString b=QString::fromLatin1(caption.base),s=QString::fromLatin1(caption.script);
        const qreal width=bm.horizontalAdvance(b)+sm.horizontalAdvance(s);
        const qreal baseline=area.center().y()+(bm.ascent()-bm.descent())/2;
        p.save();p.setPen(color);p.translate(area.center().x(),0);
        p.scale(qMin(1.0,area.width()/qMax(1.0,width)),1);
        p.setFont(base);p.drawText(QPointF(-width/2,baseline),b);
        p.setFont(script);p.drawText(QPointF(-width/2+bm.horizontalAdvance(b),baseline+(caption.below?bm.height()*.20:-bm.height()*.36)),s);
        p.restore();return true;
    }
    return false;
}
bool paintRadicalCaption(QPainter &p,const QRectF &area,const QFont &font,const QString &text,const QColor &color) {
    if(text!="∛" && text!="ⁿ√")return false;
    const qreal h=qMin(area.height()*.8,qreal(font.pixelSize()));
    const qreal x=area.center().x(),y=area.center().y();
    p.save();p.setPen(QPen(color,qMax(.8,h*.08),Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
    QPainterPath radical;radical.moveTo(x-h*.30,y);radical.lineTo(x-h*.18,y-h*.03);radical.lineTo(x-h*.04,y+h*.36);radical.lineTo(x+h*.15,y-h*.4);radical.lineTo(x+h*.54,y-h*.4);p.drawPath(radical);
    QFont degree=font;degree.setPixelSize(qMax(5,int(h*.6)));p.setFont(degree);p.setPen(color);
    p.drawText(QRectF(x-h*.65,y-h*.64,h*.52,h*.68),Qt::AlignCenter,text=="∛"?"3":"n");p.restore();return true;
}
class DisplayModeBox final : public QComboBox {
public:
    using QComboBox::QComboBox;
protected:
    void paintEvent(QPaintEvent *event) override {
        QComboBox::paintEvent(event);
        QPainter painter(this); painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen); painter.setBrush(palette().color(QPalette::Text));
        const int x=width()-9, y=height()/2;
        painter.drawPolygon(QPolygonF{QPointF(x-3,y-1),QPointF(x+3,y-1),QPointF(x,y+2)});
    }
};
QColor blend(const QColor &a, const QColor &b, qreal amount) {
    return QColor::fromRgbF(a.redF() * (1 - amount) + b.redF() * amount,
                           a.greenF() * (1 - amount) + b.greenF() * amount,
                           a.blueF() * (1 - amount) + b.blueF() * amount);
}
const QList<KeyDefinition> desktopDefinitions = {
    {"SHIFT","@shift","","",4},{"ALPHA","@alpha","","",5},{"◀","@left","HOME","@home",0,"<","<"},{"▶","@right","END","@end",0,">",">"},{"⌫","@del","DEL","@forwardDelete",3,"UNDO","@undo"},{"AC","@clear","","",3},
    {"x²","^2","x³","^3",0,"≤","<="},{"xʸ","^","ⁿ√","root(",0,"≠","!="},{"√","sqrt(","∛","cbrt(",0,"≥",">="},{"log","log10(","10ˣ","10^(",0,"2ˣ","2^("},{"ln","ln(","eˣ","exp(",0,"round","round("},{"|x|","abs(","1/x","1/(",0,"mean","mean("},
    {"sin","sin(","asin","asin(",0,"sinh","sinh("},{"cos","cos(","acos","acos(",0,"cosh","cosh("},{"tan","tan(","atan","atan(",0,"tanh","tanh("},{"(","(","%","%",0,"[","["},{")",")","mod"," mod ",0,":=",":="},{",",",",";",";"},
    {"7","7","CPLX","@complex",1,"A","@hexA"},{"8","8","∞","infinity",1,"B","@hexB"},{"9","9","1̅2̅3̅","@recurring",1,"C","@hexC"},{"÷","/","OCT","@base8",0,"im","im("},{"n!","!","nPr","perm(",0,"gcd","gcd("},{"π","pi","τ","(2*pi)",0,"D°M′S","@dms"},
    {"4","4","floor","floor(",1,"b","@binaryPrefix"},{"5","5","ceil","ceil(",1,"0x","@hexPrefix"},{"6","6","max","max(",1,"z","z"},{"×","*","BIN","@base2",0,"re","re("},{"nCr","binomial(","min","min(",0,"lcm","lcm("},{"e","e","i","i",0,"conj","conj("},
    {"1","1","CNST","@constants",1,"D","@hexD"},{"2","2","CONV","@units",1,"E","@hexE"},{"3","3","arg","arg(",1,"F","@hexF"},{"−","-","HEX","@base16",0,"z","z"},{"x","x","y","y",0,"t","t"},{"Ans","Ans","a/b","@fraction",0,"a b/c","@mixedFraction"},
    {"0","0","OTHER","@toolbox",1,"M","M"},{".",".","REG","@reg",0,"ran#","rand()"},{"EXP","*10^","FSE","@notation",0,"stdev","stdev("},{"+","+","DEC","@base10",0,"logₙ","log("},{"to"," to ","median","median(",0,"deg"," deg"},{"=","@eval","≈","@approx",2,"=","="}
};
// Seven control keys; each following scientific row has six keys.
const QList<KeyDefinition> lcdDefinitions = {
    {"SHIFT","@shift","","",4},{"ALPHA","@alpha","","",5},{"MENU","@menu","STAT","@statistics",0,"S⇔D","@format"},{"◀","@left","HOME","@home",0,"<","<"},{"▶","@right","END","@end",0,">",">"},{"⌫","@del","DEL","@forwardDelete",3,"UNDO","@undo"},{"AC","@clear","","",3},
    {"DRG","@angle","rad"," rad",0,"deg"," deg"},{"S⇔D","@format","floor","floor(",0,"ceil","ceil("},{"MTRX","[","det","det(",0,"inv","inverse("},{"Σ","sum(","Π","product(",0,"min","min("},{"∫ dx","integrate(","d/dx","diff(",0,"max","max("},{"GRAPH","@graph","SOLVE","solve("},
    {"π","pi","τ","(2*pi)",0,"D°M′S","@dms"},{"sin","sin(","sin⁻¹","asin(",0,"sinh","sinh("},{"cos","cos(","cos⁻¹","acos(",0,"cosh","cosh("},{"tan","tan(","tan⁻¹","atan(",0,"tanh","tanh("},{"i","i","∠","arg(",0,"conj","conj("},{"e","e","logₙ","log(",0,"re","re("},
    {"x⁻¹","1/(","lim","limit(",0,"im","im("},{"x²","^2","x³","^3",0,"≤","<="},{"√","sqrt(","∛","cbrt(",0,"≥",">="},{"xʸ","^","ⁿ√","root(",0,"≠","!="},{"log₁₀","log10(","10ˣ","10^(",0,"2ˣ","2^("},{"ln","ln(","eˣ","exp("},
    {"|x|","abs(","mean","mean(",0,"median","median("},{",",",",";",";"},{"x","x","y","y",0,"z","z"},{"HIST","@history","stdev","stdev(",0,"gcd","gcd("},{"MR","@mr","M+","@mplus",0,"M","M"},{"MS","@ms","MC","@mc",0,"lcm","lcm("},
    {"7","7","CPLX","@complex",1,"A","@hexA"},{"8","8","∞","infinity",1,"B","@hexB"},{"9","9","1̅2̅3̅","@recurring",1,"C","@hexC"},{"(","(","%","%",0,"[","["},{")",")","mod"," mod ",0,":=",":="},
    {"4","4","n!","!",1,"b","@binaryPrefix"},{"5","5","nCr","binomial(",1,"0x","@hexPrefix"},{"6","6","nPr","perm(",1,"z","z"},{"×","*","BIN","@base2"},{"÷","/","OCT","@base8"},
    {"1","1","CNST","@constants",1,"D","@hexD"},{"2","2","CONV","@units",1,"E","@hexE"},{"3","3","round","round(",1,"F","@hexF"},{"+","+","DEC","@base10"},{"−","-","HEX","@base16"},
    {"0","0","OTHER","@toolbox",1,"M","M"},{".",".","REG","@reg",0,"ran#","rand()"},{"Ans","Ans","a/b","@fraction",0,"a b/c","@mixedFraction"},{"EXP","*10^","FSE","@notation",0,"t","t"},{"=","@eval","≈","@approx",2,"=","="}
};
const QList<KeyDefinition> expandedDefinitions = {
    {"TRANS","transpose(","",""},{"NORM","norm(","",""},{"SORT","sort(","",""},
    {"TRUNC","trunc(","",""},{"FRAC","frac(","",""},{"VAR","var(","",""}
};
QString actionDescription(const QString &token, const std::function<QString(const char *)> &tr) {
    static const QMap<QString, const char *> descriptions {
        {"@shift","Shift functions"},{"@alpha","Alpha functions"},
        {"@left","Move cursor left"},{"@right","Move cursor right"},
        {"@home","Move cursor to start"},{"@end","Move cursor to end"},
        {"@del","Delete previous character"},{"@forwardDelete","Delete next character"},{"@undo","Undo"},
        {"@clear","Clear expression"},{"@eval","Evaluate expression"},
        {"@approx","Evaluate expression as decimal"},{"@angle","Angle unit"},
        {"@format","Switch exact and decimal result"},{"@toolbox","Toolbox"},{"@menu","Menu"},{"@settings","Settings"},
        {"@history","History"},{"@graph","Graph"},{"@statistics","Statistics"},
        {"@recurring","Repeating decimal template"},{"@dms","Degrees, minutes, seconds"},
        {"@ms","Store result in memory"},{"@mr","Recall memory"},
        {"@mplus","Add result to memory"},{"@mc","Clear memory"},
        {"@fraction","Fraction template"},{"@mixedFraction","Mixed fraction template"},{"@base2","BIN"},{"@base8","OCT"},{"@base10","DEC"},{"@base16","HEX"},
        {"@constants","Constants"},{"@units","Unit conversion"},{"@complex","Complex numbers"},{"@notation","Number notation"},
        {"=","Insert equation equals sign"},{":=","Assign a value to a register"},
        {"@reg","Register selection"},{"@regExit","Exit register selection"},
        {"@binaryPrefix","Insert binary prefix"},{"@hexPrefix","Insert hexadecimal prefix"},
        {" rad","Radian unit"},{" deg","Degree unit"},{" to ","Convert to unit"},
        {",","Separate function arguments"},{";","Separate matrix rows"},
        {"Ans","Insert previous result"},{"M","Insert memory variable"},
        {"/","Division"},{"abs(","Absolute value"}
    };
    if(token.startsWith("@hex")&&token.size()==5)return tr("Insert hexadecimal digit")+" "+token.right(1);
    if(token.size()==2&&token[0]=='R'&&token[1].isDigit())return tr("Insert register")+" "+token;
    return descriptions.contains(token) ? tr(descriptions.value(token)) : token;
}
}

CalculatorKey::CalculatorKey(KeyDefinition definition, QWidget *parent)
    : QPushButton(parent), definition_(std::move(definition)), animation_(this) {
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(22, 20);
    setText(definition_.label);
    setAccessibleName(definition_.label);
    animation_.setEasingCurve(QEasingCurve::OutCubic);
    connect(&animation_, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        depression_ = v.toReal(); update();
    });
    connect(this, &QPushButton::pressed, this, [this] { animate(1, 55); });
    connect(this, &QPushButton::released, this, [this] {
        // Keep a quick click visibly depressed for a frame before its spring return.
        animation_.stop(); depression_ = 1; update();
        QTimer::singleShot(65, this, [this] { if (!isDown()) animate(0, 145); });
    });
}
bool CalculatorKey::event(QEvent *event) {
    if((event->type()==QEvent::ShortcutOverride||event->type()==QEvent::KeyPress)&&!toolTip().isEmpty()){
        const auto *key=static_cast<QKeyEvent*>(event);if(key->key()==Qt::Key_F1&&key->modifiers()==Qt::NoModifier){
            if(event->type()==QEvent::KeyPress)QToolTip::showText(mapToGlobal(QPoint(width()/2,height())),toolTip(),this,rect());
            event->accept();return true;
        }
    }
    if(event->type()==QEvent::Hide || event->type()==QEvent::WindowDeactivate || event->type()==QEvent::UngrabMouse || (event->type()==QEvent::EnabledChange&&!isEnabled())){setDown(false);animation_.stop();depression_=0;update();}
    return QPushButton::event(event);
}
void CalculatorKey::animate(qreal target, int duration) {
    animation_.stop(); animation_.setStartValue(depression_); animation_.setEndValue(target);
    animation_.setDuration(duration); animation_.start();
}
void CalculatorKey::flash() {
    if (!isEnabled()) return;
    animation_.stop(); depression_ = 1; update();
    QTimer::singleShot(75, this, [this] { if (!isDown()) animate(0, 155); });
}
QString CalculatorKey::token() const {
    if(alphaSelected_&&!definition_.alphaToken.isEmpty())return definition_.alphaToken;
    if(shifted_&&!definition_.alternate.isEmpty())return definition_.alternate;
    return definition_.primaryAvailable ? definition_.token : QString();
}
void CalculatorKey::setDefinition(KeyDefinition definition) {
    definition_ = std::move(definition);
    translatedLabel_.clear();
    setText(definition_.primaryAvailable ? definition_.label : QString());
    setAccessibleName(text());
    update();
}
void CalculatorKey::setModes(bool shift, bool alpha) {
    alphaSelected_=alpha&&!definition_.alphaToken.isEmpty();
    shifted_ = shift && !definition_.alternate.isEmpty();
    active_ = (definition_.token == "@shift" && shift) ||
              ((definition_.token == "@alpha" || definition_.alternate == "@alpha") && alpha);
    setEnabled(definition_.primaryAvailable||shifted_||alphaSelected_);
    setAccessibleName(alphaSelected_?definition_.alphaLabel:shifted_ ? definition_.secondary : !definition_.primaryAvailable?QString():definition_.token=="."?QString(decimalSeparator_):(translatedLabel_.isEmpty() ? definition_.label : translatedLabel_));
    update();
}
void CalculatorKey::setLook(const QString &design) { design_ = design; update(); }
void CalculatorKey::setCalculatorTheme(const QString &theme) { theme_=calculatorThemeDefinition(theme).name; update(); }
void CalculatorKey::setDecimalSeparator(QChar separator){decimalSeparator_=separator;update();}
void CalculatorKey::setDesktopEffects(int effects){desktopEffects_=qBound(0,effects,3);update();}
void CalculatorKey::setTranslatedLabel(const QString &label) { translatedLabel_ = label; setModes(false, false); }

void CalculatorKey::paintEvent(QPaintEvent *) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    const bool lcd = design_ == "Calculator";
    const auto theme=calculatorThemeDefinition(theme_);
    // Desktop effect QSS can provide gradient brushes; use the actual theme
    // palette so reading Button as a QColor never turns those faces black.
    const QPalette keyPalette = lcd ? palette() : qApp->palette();
    const bool classic = lcd && theme.beveled, modern = lcd && !theme.beveled;
    const QColor accent = lcd ? theme.accent : keyPalette.color(QPalette::Highlight);
    QColor face = keyPalette.color(QPalette::Button), ink = keyPalette.color(QPalette::ButtonText);
    QColor legend = accent;
    const QColor alphaInk=lcd?(theme.body.lightnessF()<.5?QColor("#9cd9ee"):QColor("#175e80")):accent;
    if (lcd) {
        face=theme.key;ink=theme.keyText;legend=theme.legend;
        if(definition_.kind==1){face=theme.number;ink=theme.numberText;}
        if(definition_.kind==4){face=theme.shift;ink=theme.shiftText;}
        if(definition_.kind==5){face=theme.name=="Classic"?QColor("#b5dce9"):blend(theme.accent,QColor("#a2d4ed"),.55);ink=QColor("#142f3b");}
        if(definition_.kind==3){face=theme.clear;ink=theme.clearText;}
        if(definition_.kind==2){face=theme.accent;ink=calculatorThemePalette(theme_).color(QPalette::HighlightedText);}
    } else {
        if (definition_.kind == 1) face = keyPalette.color(QPalette::Base);
        if (definition_.kind == 3) face = keyPalette.color(QPalette::AlternateBase);
        if (definition_.kind == 5) {face=blend(face,accent,.25);}
        if (definition_.kind == 2
            || active_
        ) { face = accent; ink = keyPalette.color(QPalette::HighlightedText); }
    }
    if (active_) { face = blend(face, accent, .6); ink = lcd?calculatorThemePalette(theme_).color(QPalette::HighlightedText):keyPalette.color(QPalette::HighlightedText); }
    if (!isEnabled()) { face = blend(face, keyPalette.color(QPalette::Window), .45); ink.setAlpha(125); }
    const qreal depression = isDown() ? 1.0 : depression_;
    if (underMouse()) face = face.lighter(110);
    face = blend(face, accent, depression * .26);
    const QString baseLabel = definition_.primaryAvailable ? definition_.label : QString();
    const QString primary = alphaSelected_?definition_.alphaLabel:shifted_ ? definition_.secondary : !definition_.primaryAvailable?QString():definition_.token=="."?QString(decimalSeparator_):(translatedLabel_.isEmpty() ? baseLabel : translatedLabel_);
    const QString secondary = shifted_ ? baseLabel : definition_.secondary;
    const QString tertiary = alphaSelected_?baseLabel:definition_.alphaLabel;
    // With enlarged display text or the new guide a short row cannot hold both
    // inactive legends and a readable active face. Preserve the active layer;
    // SHIFT/ALPHA, native descriptions and held Help still expose every action.
    // Ordinary-font artwork without an active guide keeps its original strip.
    const bool compactFace=lcd&&property("compactKeyFaces").toBool()&&height()<42;
    const bool showSecondary=!compactFace&&(!secondary.isEmpty()||!tertiary.isEmpty());
    setProperty("compactKeyFace",compactFace);
    // The scientific bank is one aligned physical keyboard: even SHIFT and AC
    // reserve the legend strip, so no key face becomes taller than its neighbours.
    // The legend strip is a physical part of the Calculator keyboard. Match
    // the physical face boundary rather than spending extra button height on
    // the system font's line spacing (notably larger with Windows fonts).
    const int calculatorLegendStrip=qBound(11,height()/4,20);
    QFont legendFont=font();legendFont.setPixelSize(DesktopFont::pixels(lcd?qBound(8,calculatorLegendStrip-1,15):qBound(9,height()/5,12)));
    const int legendHeight = compactFace ? 0 : lcd ? calculatorLegendStrip : (showSecondary ? QFontMetrics(legendFont).height()+1 : 0);
    QRectF faceRect = QRectF(rect()).adjusted(1.5, lcd ? legendHeight + 1.5 : 1.5, -1.5, -3.5);
    setProperty("keyFace",faceRect);
    const qreal radius = classic ? 1.5 : modern ? qMin(10., faceRect.height() / 3) : 7;
    if (faceRect.height() < 7) return;
    p.setPen(Qt::NoPen); p.setBrush(classic ? QColor("#191c1a") : QColor(0, 0, 0, modern ? 65 : 45));
    if(lcd||desktopEffects_)p.drawRoundedRect(faceRect.translated(0, desktopEffects_==2?3.5:2.5), radius, radius);
    faceRect.translate(0, 1.7 * depression);
    if(!lcd&&desktopEffects_==1)face.setAlpha(195);
    if(!lcd&&desktopEffects_==3)face.setAlpha(180);
    QColor gradientTop=face.lighter(classic||(!lcd&&desktopEffects_==2)?124:!lcd&&desktopEffects_==0?100:110);
    QColor gradientBottom=face.darker(classic||(!lcd&&desktopEffects_==2)?114:!lcd&&desktopEffects_==0?100:104);
    QColor primaryInk=alphaSelected_?(face.lightnessF()<.5?QColor("#9cd9ee"):QColor("#175e80")):ink;
    QColor secondaryInk=shifted_?ink:legend,tertiaryInk=alphaInk;
    if(!lcd&&isEnabled()&&(definition_.kind==2||active_)) {
        // An accent-filled action key cannot use that same accent as legend
        // ink. Evaluate both gradient ends over the theme's possible card
        // backgrounds, including the Glass/Frosted face alpha, then retain
        // tinted secondary ink wherever it has readable small-text contrast.
        const auto luminance=[](const QColor &c) {
            const auto linear=[](qreal v){return v<=.04045?v/12.92:std::pow((v+.055)/1.055,2.4);};
            return .2126*linear(c.redF())+.7152*linear(c.greenF())+.0722*linear(c.blueF());
        };
        const auto minimumContrast=[&](const QColor &foreground) {
            qreal minimum=21.0;
            for(const QColor &endpoint:{gradientTop,gradientBottom})for(const auto role:{QPalette::Base,QPalette::Window,QPalette::Highlight}) {
                const QColor background=blend(keyPalette.color(role),endpoint,endpoint.alphaF());
                const qreal a=luminance(foreground),b=luminance(background);
                minimum=qMin(minimum,(qMax(a,b)+.05)/(qMin(a,b)+.05));
            }
            return minimum;
        };
        const QColor strong=minimumContrast(Qt::black)>=minimumContrast(Qt::white)?QColor(Qt::black):QColor(Qt::white);
        const QColor surfaceTarget=strong==QColor(Qt::black)?QColor(Qt::white):QColor(Qt::black);
        // Some raised gradients straddle the dark/light text crossover.
        // Slightly move only these action faces away from the chosen ink,
        // preserving their opacity, hue family and raised-edge treatment.
        for(int step=0;step<40&&minimumContrast(strong)<4.6;++step) {
            const int topAlpha=gradientTop.alpha(),bottomAlpha=gradientBottom.alpha();
            gradientTop=blend(gradientTop,surfaceTarget,.04);gradientTop.setAlpha(topAlpha);
            gradientBottom=blend(gradientBottom,surfaceTarget,.04);gradientBottom.setAlpha(bottomAlpha);
        }
        const auto readable=[&](const QColor &preferred) {
            if(minimumContrast(preferred)>=4.5)return preferred;
            for(int step=1;step<=100;++step) {
                const QColor candidate=blend(preferred,strong,step/100.0);
                if(minimumContrast(candidate)>=4.5)return candidate;
            }
            return strong;
        };
        ink=readable(ink);primaryInk=readable(primaryInk);
        secondaryInk=readable(secondaryInk);tertiaryInk=readable(tertiaryInk);
        setProperty("primaryInkContrast",minimumContrast(primaryInk));
        setProperty("secondaryInkContrast",minimumContrast(secondaryInk));
        setProperty("tertiaryInkContrast",minimumContrast(tertiaryInk));
    }
    QLinearGradient gradient(faceRect.topLeft(), faceRect.bottomRight());
    gradient.setColorAt(0,gradientTop);gradient.setColorAt(1,gradientBottom);
    p.setBrush(gradient);
    p.setPen(QPen(hasFocus() ? accent.lighter(135) : lcd ? theme.border : blend(face, ink, .20), hasFocus() ? 2 : 1));
    p.drawRoundedRect(faceRect, radius, radius);
    if (classic||(!lcd&&desktopEffects_==2)) {
        p.setPen(QPen(QColor(255,255,240,100), 1));
        p.drawLine(faceRect.topLeft() + QPointF(2,2), faceRect.topRight() + QPointF(-2,2));
        p.drawLine(faceRect.topLeft() + QPointF(2,2), faceRect.bottomLeft() + QPointF(2,-2));
        p.setPen(QPen(QColor(0,0,0,150), 2));
        p.drawLine(faceRect.bottomLeft() + QPointF(2,-1), faceRect.bottomRight() + QPointF(-1,-1));
    }
    QFont labelFont = font();
    // Both Desktop label bands live inside the inset face, below the top
    // bevel and above the bottom raised edge. They move with the pressed face.
    const qreal bevel=classic||(!lcd&&desktopEffects_==2)?3.0:2.0;
    const QRectF usableFace=faceRect.adjusted(3,lcd?0:bevel,-3,lcd?0:-bevel);
    const QRectF legendArea=lcd?QRectF(2,0,width()-4,legendHeight+1):
        QRectF(usableFace.x(),usableFace.y(),usableFace.width(),legendHeight);
    const bool horizontalLegends=!lcd&&showSecondary&&usableFace.height()<25&&usableFace.width()>=72;
    const QFontMetricsF legendMetrics(legendFont);
    const qreal leftLegendWidth=secondary.isEmpty()?0:qMin(usableFace.width()*.43,qMax(usableFace.width()*.18,(hasKeyCaption(secondary)?keyCaptionWidth(secondary,legendFont):legendMetrics.horizontalAdvance(secondary))+8));
    const qreal rightLegendWidth=tertiary.isEmpty()?0:qMin(usableFace.width()*.43,qMax(usableFace.width()*.18,(hasKeyCaption(tertiary)?keyCaptionWidth(tertiary,legendFont):legendMetrics.horizontalAdvance(tertiary))+4));
    QRectF textRect=usableFace;
    if(horizontalLegends)textRect=usableFace.adjusted(leftLegendWidth+2,0,-rightLegendWidth-2,0);
    else if(!lcd&&showSecondary)textRect.setTop(legendArea.bottom());
    setProperty("usableFace",usableFace);setProperty("primaryLabelArea",textRect);
    setProperty("secondaryLabelArea",legendArea);
    int mainPixels = compactFace?qBound(12,int(faceRect.height()*.62),23):lcd?qBound(11,int(faceRect.height()*(definition_.kind==1||definition_.kind==2?.54:.43)),29):qBound(10,int(textRect.height()*.72),23);
    mainPixels=DesktopFont::pixels(mainPixels);
    labelFont.setPixelSize(mainPixels); labelFont.setBold(modern || (!lcd && definition_.kind == 1));
    QFontMetrics metrics(labelFont);
    const auto primaryWidth=[&](){return hasKeyCaption(primary)?keyCaptionWidth(primary,labelFont):qreal(metrics.horizontalAdvance(primary));};
    while (mainPixels > (compactFace?12:lcd?8:10) && (primaryWidth() > textRect.width() || metrics.height()>textRect.height())) {
        labelFont.setPixelSize(--mainPixels); metrics = QFontMetrics(labelFont);
    }
    if(compactFace&&primaryWidth()>textRect.width()){
        labelFont.setStretch(qBound(75,int(100*textRect.width()/qMax(1.,primaryWidth())),100));
        metrics=QFontMetrics(labelFont);
    }
    setProperty("primaryFontPixels",mainPixels);
    setProperty("primaryFontHeight",metrics.height());setProperty("primaryTextWidth",primaryWidth());
    setProperty("legendFontHeight",QFontMetrics(legendFont).height());
    p.save();p.setClipRect(usableFace);
    p.setFont(labelFont); p.setPen(primaryInk);
    const auto paintRecurring=[&](QRectF area,const QFont &baseFont,const QColor &color){
        QFont numberFont=baseFont;int pixels=qMax(6,baseFont.pixelSize());
        numberFont.setPixelSize(pixels);
        while(pixels>6&&(QFontMetricsF(numberFont).horizontalAdvance("123")>area.width()-2||QFontMetricsF(numberFont).height()+2>area.height()))numberFont.setPixelSize(--pixels);
        const QFontMetricsF fm(numberFont);const qreal w=fm.horizontalAdvance("123");
        const qreal x=area.center().x()-w/2, baseline=area.center().y()+(fm.ascent()-fm.descent())/2+1;
        p.setFont(numberFont);p.setPen(color);p.drawText(QPointF(x,baseline),"123");
        p.setPen(QPen(color,qMax(1.0,pixels*.06)));p.drawLine(QPointF(x,baseline-fm.ascent()),QPointF(x+w,baseline-fm.ascent()));
    };
    setProperty("primaryCaptionParts",QVariantList());setProperty("secondaryCaptionParts",QVariantList());setProperty("tertiaryCaptionParts",QVariantList());
    if(paintKeyCaption(p,textRect,labelFont,primary,primaryInk,this,"primaryCaptionParts")) {
        // Preserve the complete literal key label before the generic symbol.
    } else if(MathCaption::paint(p,textRect,labelFont,token(),primaryInk)) {
        // Display notation does not change the existing key's insertion token.
    } else if(token()=="@left" || token()=="@right") {
        setProperty("usesSvgArrow",paintArrowSvg(p,textRect,token()=="@right",primaryInk));
    }
    else if(token()=="@recurring")paintRecurring(textRect,labelFont,p.pen().color());
    else if(token()=="@del") {
        const qreal h=qMin(textRect.height()*.66,22.0);
        setProperty("usesSvgBackspace",KeyArtwork::paint(p,QRectF(textRect.center().x()-h*.77,textRect.center().y()-h*.42,h*1.54,h*.84),"backspace",primaryInk));
    }else if((primary=="a/b"||primary=="a b/c")&&(!lcd&&textRect.height()<13))p.drawText(textRect,Qt::AlignCenter,primary);
    else if(primary=="a/b"||primary=="a b/c") {
        const bool mixed=primary=="a b/c";
        const qreal h=qMin(textRect.height()*.7,18.0), x=textRect.center().x()+(mixed?h*.29:0), y=textRect.center().y();
        p.setPen(QPen(ink,1.3));p.setBrush(Qt::NoBrush);
        if(mixed)p.drawRect(QRectF(x-h*.94,y-h*.26,h*.34,h*.52));
        p.drawRect(QRectF(x-h*.20,y-h*.47,h*.40,h*.30));
        p.drawLine(QPointF(x-h*.43,y),QPointF(x+h*.43,y));
        p.drawRect(QRectF(x-h*.20,y+h*.16,h*.40,h*.30));
    } else if(primary=="xʸ") {
        const qreal h=qMin(textRect.height()*.72,20.0),x=textRect.center().x(),y=textRect.center().y();
        p.setPen(QPen(ink,1.2));p.setBrush(Qt::NoBrush);
        p.drawRect(QRectF(x-h*.38,y-h*.18,h*.50,h*.56));
        p.drawRect(QRectF(x+h*.23,y-h*.47,h*.31,h*.31));
    } else if(primary=="√"||primary=="ⁿ√"||primary=="∛") {
        const qreal h=qMin(textRect.height()*.73,21.0),x=textRect.center().x(),y=textRect.center().y();
        p.setPen(QPen(ink,1.25));p.setBrush(Qt::NoBrush);
        QPainterPath radical;radical.moveTo(x-h*.61,y);radical.lineTo(x-h*.46,y-h*.04);radical.lineTo(x-h*.28,y+h*.34);radical.lineTo(x-h*.06,y-h*.39);radical.lineTo(x+h*.55,y-h*.39);p.drawPath(radical);
        p.drawRect(QRectF(x+h*.10,y-h*.17,h*.34,h*.43));
        if(primary=="ⁿ√")p.drawRect(QRectF(x-h*.54,y-h*.47,h*.24,h*.24));
        if(primary=="∛"){QFont degree=labelFont;degree.setPixelSize(qMax(7,int(h*.40)));p.setFont(degree);p.drawText(QRectF(x-h*.7,y-h*.66,h*.45,h*.6),Qt::AlignCenter,"3");}
    } else if(!paintScriptCaption(p,textRect,labelFont,primary,primaryInk))p.drawText(textRect, Qt::AlignCenter, primary);
    p.restore();
    setProperty("legendsFit",true);
    // The native caption fixture can request exact fit measurements. Avoid
    // creating diagnostic records during normal application painting.
    const bool diagnoseLegends=property("diagnoseLegendFit").toBool();
    if(diagnoseLegends)setProperty("legendFitDiagnostics",QVariantList());
    setProperty("secondaryFontPixels",0);setProperty("tertiaryFontPixels",0);
    if (showSecondary) {
        const auto paintLegend=[&](QString text,const QString &action,QRectF area,QColor color,bool boxed,const char *fontProperty){
            if(text.isEmpty())return;
            p.save();p.setClipRect(area);
            if(text==QString::fromUtf8("1̅2̅3̅")){
                QFont small=font();small.setPixelSize(qBound(7,legendHeight-2,15));
                paintRecurring(area,small,color);p.restore();return;
            }
            if(text=="a/b"||text=="a b/c") {
                // The explicit fraction template is visually different from
                // ordinary division even before its SHIFT layer is selected.
                const bool mixed=text=="a b/c";
                const qreal h=qMin(qMin(qreal(area.height()-1),14.0),qreal(area.width())/(mixed?1.6:1.0)),
                    x=area.center().x()+(mixed?h*.29:0),y=area.center().y();
                p.setPen(QPen(color,1));p.setBrush(Qt::NoBrush);
                if(mixed)p.drawRect(QRectF(x-h*.94,y-h*.26,h*.34,h*.52));
                p.drawRect(QRectF(x-h*.19,y-h*.46,h*.38,h*.27));
                p.drawLine(QPointF(x-h*.43,y),QPointF(x+h*.43,y));
                p.drawRect(QRectF(x-h*.19,y+h*.19,h*.38,h*.27));
                p.restore();return;
            }
            QFont small=legendFont;small.setBold(boxed);
            int pixels=legendFont.pixelSize();small.setPixelSize(pixels);
            // Calculator legends are centered by the actual ink bounds. A
            // font's line height includes unused leading, which previously
            // shrank both the legends and the physical button faces.
            const auto textWidth=[&](){return hasKeyCaption(text)?keyCaptionWidth(text,small):qreal(QFontMetrics(small).horizontalAdvance(text));};
            const auto textHeight=[&](){return hasKeyCaption(text)?keyCaption(text,small).layout.bounds.height():lcd?QFontMetricsF(small).tightBoundingRect(text).height():qreal(QFontMetrics(small).height());};
            while(pixels>(lcd?7:9)&&(textWidth()+(boxed?6:0)>area.width()||textHeight()>area.height()-(lcd?0:1))){small.setPixelSize(--pixels);}
            // A mildly condensed system font preserves readable cap height on
            // narrow keys where both inactive legends share a row.
            const qreal textSpace=area.width()-(boxed?6:0);
            const qreal condensedWidth=hasKeyCaption(text)?keyCaptionWidth(text,small):QFontMetricsF(small).horizontalAdvance(text);
            if(condensedWidth>textSpace)
                small.setStretch(qBound(75,int(100*textSpace/qMax(1.0,condensedWidth)),100));
            p.setFont(small);
            setProperty(fontProperty,pixels);
            const auto recordFit=[&](const QRectF &target,bool fits,const char *method,const QRectF &paintedInk=QRectF(),qreal inkScale=1.){
                if(!diagnoseLegends)return;
                const QFontMetricsF fm(small);const QFontInfo resolved(small);
                QVariantMap diagnostic{{"label",text},{"method",method},{"fits",fits},{"target",target},
                    {"font",small.toString()},{"resolvedFamily",resolved.family()},{"pointSize",small.pointSizeF()},
                    {"pixelSize",small.pixelSize()},{"resolvedPixelSize",resolved.pixelSize()},{"stretch",small.stretch()},
                    {"measuredWidth",textWidth()},{"measuredHeight",textHeight()},{"horizontalAdvance",fm.horizontalAdvance(text)},
                    {"tightInk",fm.tightBoundingRect(text)},{"lineHeight",fm.height()},{"boxedPadding",boxed?6:0},
                    {"paintedInk",paintedInk},{"inkScale",inkScale}};
                if(hasKeyCaption(text))diagnostic.insert("compositeBounds",keyCaption(text,small).layout.bounds);
                auto records=property("legendFitDiagnostics").toList();records.append(diagnostic);setProperty("legendFitDiagnostics",records);
            };
            const auto validateNominalFit=[&](){
                const bool fits=!(textWidth()+(boxed?6:0)>area.width()+.5||textHeight()>area.height()+.5);
                if(!fits)setProperty("legendsFit",false);
                recordFit(area,fits,"plain");
            };
            const auto drawLegend=[&](const QRectF &target){
                const QByteArray property=QByteArray(fontProperty).startsWith("secondary")?"secondaryCaptionParts":"tertiaryCaptionParts";
                if(hasKeyCaption(text)) {
                    // Composite captions fit uniformly into their allocated
                    // legend target. Report the transformed ink actually drawn,
                    // not the wider nominal layout before that fitting step.
                    bool fits=paintKeyCaption(p,target,small,text,p.pen().color(),this,property);
                    for(const auto &part:this->property(property.constData()).toList())
                        fits&=target.adjusted(-.01,-.01,.01,.01).contains(part.toRectF());
                    if(!fits)setProperty("legendsFit",false);
                    recordFit(target,fits,"composite");
                    return;
                }
                if(MathCaption::paint(p,target,small,action,p.pen().color())){validateNominalFit();return;}
                if(text=="◀" || text=="▶"){paintArrowSvg(p,target,text=="▶",p.pen().color());validateNominalFit();return;}
                if(paintRadicalCaption(p,target,small,text,p.pen().color())){validateNominalFit();return;}
                if(paintScriptCaption(p,target,small,text,p.pen().color())){validateNominalFit();return;}
                const QRectF inkBounds=QFontMetricsF(small).tightBoundingRect(text);
                const QRectF inkTarget=boxed?target.adjusted(3,0,-3,0):target;
                if(lcd||inkBounds.width()>inkTarget.width()||inkBounds.height()>inkTarget.height()){
                    // At small pixel sizes, hinted ink can remain wider than
                    // horizontalAdvance after font stretching. Fit the actual
                    // ink we draw, keeping the established font-size limits.
                    // Desktop legends that already fit keep their alignment.
                    if(inkBounds.isEmpty()||inkTarget.isEmpty()){
                        setProperty("legendsFit",false);recordFit(inkTarget,false,"plain-ink");return;
                    }
                    const qreal scale=qMin(1.,qMin(inkTarget.width()/inkBounds.width(),inkTarget.height()/inkBounds.height()));
                    QTransform transform;transform.translate(inkTarget.center().x(),inkTarget.center().y());
                    transform.scale(scale,scale);transform.translate(-inkBounds.center().x(),-inkBounds.center().y());
                    p.save();p.setTransform(transform,true);p.drawText(QPointF(0,0),text);p.restore();
                    const QRectF paintedInk=transform.mapRect(inkBounds);
                    const bool fits=inkTarget.adjusted(-.01,-.01,.01,.01).contains(paintedInk);
                    if(!fits)setProperty("legendsFit",false);
                    recordFit(inkTarget,fits,"plain-ink",paintedInk,scale);
                }
                else {validateNominalFit();p.drawText(target,Qt::AlignCenter,text);}
            };
            if(boxed){const qreal bw=qMin(area.width(),qreal(textWidth()+6));QRectF box(area.center().x()-bw/2,area.y(),bw,area.height());p.setPen(Qt::NoPen);p.setBrush(color);p.drawRoundedRect(box,2,2);p.setPen(lcd?theme.body:face);drawLegend(box);}
            else{p.setPen(color);drawLegend(area);}
            p.restore();
        };
        QRectF legendRect=horizontalLegends?usableFace:legendArea;
        const bool split=!secondary.isEmpty()&&!tertiary.isEmpty();
        QRectF secondArea=legendRect,thirdArea=legendRect;
        const bool boxed=QStringList{"BIN","OCT","DEC","HEX","REG"}.contains(secondary);
        if(horizontalLegends) {
            secondArea.setWidth(leftLegendWidth);
            thirdArea.setLeft(legendRect.right()-rightLegendWidth);
        } else if(split){
            QFont secondFont=legendFont;secondFont.setBold(boxed);
            const qreal left=qMax(10.0,keyCaptionWidth(secondary,secondFont)+(boxed?6:0)),right=qMax(10.0,keyCaptionWidth(tertiary,legendFont));
            secondArea.setWidth((legendRect.width()-2)*left/(left+right));thirdArea.setLeft(secondArea.right()+2);
        }
        paintLegend(secondary,shifted_?definition_.token:definition_.alternate,secondArea,secondaryInk,boxed,"secondaryFontPixels");
        paintLegend(tertiary,alphaSelected_?definition_.token:definition_.alphaToken,thirdArea,tertiaryInk,false,"tertiaryFontPixels");
    }
}

CalculatorPanel::CalculatorPanel(QWidget *parent) : QWidget(parent) {
    setObjectName("calculatorPanel"); setMinimumWidth(330);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    translate_ = [](const char *s) { return QString::fromUtf8(s); };
    screen_ = new QFrame(this); screen_->setObjectName("calculatorScreen");
    scientific_ = new QLabel(screen_); scientific_->setObjectName("panelScientific");
    memoryIndicator_ = new QLabel("M", screen_); memoryIndicator_->setAlignment(Qt::AlignCenter);
    memoryIndicator_->setObjectName("panelMemoryIndicator");
    angle_ = new DisplayModeBox(screen_); angle_->setObjectName("panelAngle"); angle_->addItems({"RAD","DEG","GRAD"});
    exact_ = new DisplayModeBox(screen_); exact_->setObjectName("panelExact"); exact_->addItems({"Exact","Decimal"});
    base_=new DisplayModeBox(screen_);base_->setObjectName("panelBase");
    base_->addItem("BIN",2);base_->addItem("OCT",8);base_->addItem("DEC",10);base_->addItem("HEX",16);base_->setCurrentIndex(2);
    notation_=new DisplayModeBox(screen_);notation_->setObjectName("panelNotation");
    notation_->addItem("Normal",0);notation_->addItem("SCI",1);notation_->addItem("ENG",2);
    angleIndicator_=new QLabel(screen_);angleIndicator_->setObjectName("panelAngleIndicator");
    exactIndicator_=new QLabel(screen_);exactIndicator_->setObjectName("panelExactIndicator");
    baseIndicator_=new QLabel(screen_);baseIndicator_->setObjectName("panelBaseIndicator");baseIndicator_->setAlignment(Qt::AlignCenter);
    notationIndicator_=new QLabel(screen_);notationIndicator_->setObjectName("panelNotationIndicator");
    for(auto box:{angle_,exact_,base_,notation_})connect(box,&QComboBox::currentIndexChanged,this,[this]{syncIndicators();});
    numberKind_=new QLabel("CPLX",screen_);numberKind_->setObjectName("panelNumberKind");
    copy_ = new QPushButton(screen_); copy_->setObjectName("panelCopy");
    settings_ = new QPushButton(QString::fromUtf8("⚙"),screen_); settings_->setObjectName("panelSettings");
    settings_->setFocusPolicy(Qt::StrongFocus); settings_->setCursor(Qt::PointingHandCursor);
    connect(settings_,&QPushButton::clicked,this,[this]{ if(onAction)onAction("@settings"); });
    status_ = new QLabel(screen_); status_->setObjectName("panelStatus"); status_->setTextFormat(Qt::PlainText);
    status_->setWordWrap(true); status_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    status_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    editor_ = new MathEditor(screen_); editor_->setObjectName("expression"); editor_->setMaxLength(4096);
    result_ = new MathEditor(screen_); result_->setObjectName("result"); result_->setReadOnly(true);
    argumentGuide_=new ArgumentGuide(editor_,screen_);
    argumentGuide_->setObjectName("calculatorArgumentGuide");
    argumentGuide_->onGeometryChanged=[this]{arrange();};
    approximate_ = new RecurringLabel(screen_); approximate_->setObjectName("panelApproximate");
    approximate_->setTextFormat(Qt::PlainText);
    approximate_->setTextInteractionFlags(Qt::TextSelectableByMouse); approximate_->setMinimumWidth(0);
    approximate_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    for (const QString &label : {QString("MS"),QString("MR"),QString("M+"),QString("MC")}) {
        auto *button = new CalculatorKey({label,"@" + (label == "M+" ? QString("mplus") : label.toLower()),"",""}, this);
        button->setObjectName("memory" + label); memories_.append(button);
    }
    makeKeys(); retranslate(translate_); setDesign("Desktop");
    result_->onTextChanged=[this]{arrange();};
    // Catch display descendants, including dynamically created editor children.
    // Their tooltip text remains available when the preference is re-enabled.
    qApp->installEventFilter(this);
}
bool CalculatorPanel::eventFilter(QObject *watched, QEvent *event) {
    if (!screenTooltipsEnabled_ && event->type() == QEvent::ToolTip) {
        // QObject ancestry also includes the mode boxes' popup windows; the
        // QWidget ancestor check intentionally stops at a window boundary.
        for(auto object=watched;object;object=object->parent())
            if(object==screen_) {QToolTip::hideText();event->accept();return true;}
    }
    return QWidget::eventFilter(watched,event);
}
void CalculatorPanel::makeKeys() {
    auto make = [this](const QList<KeyDefinition> &definitions, QList<CalculatorKey *> &buttons, const QString &prefix) {
        for (int i = 0; i < definitions.size(); ++i) {
            auto *button = new CalculatorKey(definitions[i], this);
            button->setObjectName(prefix + QString::number(i)); buttons.append(button);
            connect(button, &QPushButton::clicked, this, [this,button] { dispatchKeyAction(button->token()); });
        }
    };
    make(desktopDefinitions, desktopKeys_, "key"); make(lcdDefinitions, lcdKeys_, "lcdKey");make(expandedDefinitions,expandedKeys_,"expandedKey");
    refreshDefinitions();
}
void CalculatorPanel::refreshDefinitions() {
    // Each visible layout has one assignment for each action. A normal key wins
    // over SHIFT, then ALPHA; equal-layer assignments prefer the lower bank.
    // Restore from the complete definitions before each pass so Pocket retains
    // functions whose dedicated scientific keys are absent in that layout.
    const auto canonical = [](const QString &token) {
        if(token=="@mr")return QString("M");
        return token;
    };
    const auto forNumberMode = [this](QList<KeyDefinition> definitions) {
        for(auto &key:definitions) {
            if(key.alternate=="@complex")key.secondary=complexEnabled_?"REAL":"CPLX";
            if(key.alphaToken=="@complex")key.alphaLabel=complexEnabled_?"REAL":"CPLX";
        }
        if(complexEnabled_)return definitions;
        const QSet<QString> complexOnly {"i","arg(","conj(","re(","im("};
        for(auto &key:definitions) {
            // Sign is useful for real values without duplicating another key.
            // Factorial, combinations and permutations stay on SHIFT+4/5/6,
            // so changing the number domain never changes those combinations.
            if(key.token=="i")key={"sgn","sgn(","",""};
            if(complexOnly.contains(key.alternate)) {key.alternate.clear();key.secondary.clear();}
            if(complexOnly.contains(key.alphaToken)) {key.alphaToken.clear();key.alphaLabel.clear();}
        }
        return definitions;
    };
    const auto unique = [&](QList<KeyDefinition> definitions, const QList<int> &visible, QSet<QString> seen) {
        for(int layer=0;layer<3;++layer)for(auto it=visible.crbegin();it!=visible.crend();++it) {
            auto &key=definitions[*it];
            QString &token=layer==0?key.token:layer==1?key.alternate:key.alphaToken;
            if(token.isEmpty())continue;
            const QString action=canonical(token);
            if(seen.contains(action)&&layer>0) {
                token.clear();
                (layer==1?key.secondary:key.alphaLabel).clear();
            }else seen.insert(action);
        }
        return definitions;
    };
    QList<int> desktopVisible,lcdVisible;
    for(int i=0;i<desktopDefinitions.size();++i)desktopVisible<<i;
    for(int i=0;i<lcdDefinitions.size();++i)
        if(calculatorLayout_!="Pocket"||i<7||(i>=13&&i<25)||i>=31)lcdVisible<<i;
    // Keep memory actions together in the dedicated desktop memory bank.
    auto desktop=unique(forNumberMode(desktopDefinitions),desktopVisible,{"@ms","M","@mplus","@mc"});
    auto lcdSource=forNumberMode(lcdDefinitions);
    if(calculatorLayout_=="Pocket")for(auto &key:lcdSource){
        if(key.token=="*10^"){key.alphaLabel="DRG";key.alphaToken="@angle";}
        if(key.token=="ln("){key.alphaLabel="|x|";key.alphaToken="abs(";}
        if(key.token=="*"){key.alphaLabel="x";key.alphaToken="x";}
        if(key.token=="/"){key.alphaLabel="y";key.alphaToken="y";}
        if(key.token=="+"){key.alphaLabel=",";key.alphaToken=",";}
    }
    auto lcd=unique(lcdSource,lcdVisible,{});
    const auto forBase = [this](QList<KeyDefinition> &definitions) {
        const int radix=literalInputBase_?literalInputBase_:base_->currentData().toInt();
        for(auto &key:definitions) {
            if(key.kind==1&&key.token.size()==1&&key.token[0].isDigit())
                key.primaryAvailable=key.token[0].digitValue()<radix;
            // A radix point is meaningful in every supported number system.
            if(key.token==".")key.primaryAvailable=true;
            // Decimal accepts A–F as a convenience; BIN/OCT do not advertise
            // letters which their numeric input cannot use.
            if(radix<10&&key.alphaToken.startsWith("@hex")&&key.alphaToken.size()==5) {
                key.alphaToken.clear();key.alphaLabel.clear();
            }
        }
    };
    forBase(desktop);forBase(lcd);
    const auto applyRegisters=[this](QList<KeyDefinition> &definitions) {
        if(!registerLayer_)return;
        for(auto &key:definitions) {
            if(key.kind==1&&key.token.size()==1&&key.token[0].isDigit()) {
                const QString token="R"+key.token;key={token,token,"","",1};
            } else if(key.token=="."||key.token=="@shift") {
                const QString label=key.token=="."?QString(decimalSeparator_):QString("SHIFT");
                key={label,"@regExit","","",4};
            } else key={"","","","",0,"","",false};
        }
    };
    applyRegisters(desktop);applyRegisters(lcd);
    auto expanded=expandedDefinitions;applyRegisters(expanded);
    if(customLayout_) {
        const auto &profile=customWide_?customLayout_->landscape:customLayout_->portrait;
        QList<KeyDefinition> custom;QList<int> visible;
        for(const auto &key:profile.keys) {
            const auto primary=CustomKeypad::action(key.primary),shift=CustomKeypad::action(key.shift),alpha=CustomKeypad::action(key.alpha);
            visible.append(custom.size());custom.append({translate_(primary.label.toUtf8().constData()),primary.token,translate_(shift.label.toUtf8().constData()),shift.token,primary.kind,translate_(alpha.label.toUtf8().constData()),alpha.token});
        }
        custom=unique(forNumberMode(custom),visible,{});forBase(custom);applyRegisters(custom);
        for(int i=0;i<customKeys_.size()&&i<custom.size();++i)customKeys_[i]->setDefinition(custom[i]);
    }
    for(int i=0;i<expandedKeys_.size();++i)expandedKeys_[i]->setDefinition(expanded[i]);
    for(auto *button:memories_) {
        auto *key=static_cast<CalculatorKey *>(button);
        const QString label=button->objectName().mid(6);
        key->setDefinition(registerLayer_?KeyDefinition{"","","","",0,"","",false}:
            KeyDefinition{label,"@"+(label=="M+"?QString("mplus"):label.toLower()),"",""});
        key->setModes(false,false);
    }
    for(int i=0;i<desktopKeys_.size();++i)desktopKeys_[i]->setDefinition(desktop[i]);
    for(int i=0;i<lcdKeys_.size();++i)lcdKeys_[i]->setDefinition(lcd[i]);
    setShiftState(shift_,alpha_);
}
QList<CalculatorKey *> CalculatorPanel::keyButtons() const {
    if(design_=="Desktop")return desktopKeys_;
    if(customLayout_)return customKeys_;
    QList<CalculatorKey*> keys;
    for(int i=0;i<31;++i)if(calculatorLayout_!="Pocket" || i<7 || (i>=13&&i<25))keys<<lcdKeys_[i];
    if(calculatorLayout_=="Expanded")keys<<expandedKeys_;
    for(int i=31;i<lcdKeys_.size();++i)keys<<lcdKeys_[i];
    return keys;
}
void CalculatorPanel::setDesign(QString design) {
    if(design=="Classic LCD"){theme_="Classic";design="Calculator";}
    if(design=="Modern LCD"){theme_="Modern Light";design="Calculator";}
    if (design != "Calculator") design = "Desktop";
    if(design_!=design)setRegisterLayer(false);
    design_ = design;
    setMinimumWidth(design_ == "Desktop" ? 330 : 320);
    for (const auto &list:{desktopKeys_,lcdKeys_,expandedKeys_,customKeys_})for(auto *key:list)key->hide();
    for(auto *key:keyButtons())key->show();
    for (const auto &list:{lcdKeys_,expandedKeys_,customKeys_})for(auto *key:list){key->setLook("Calculator");key->setCalculatorTheme(theme_);}
    for (auto *button : memories_) button->setVisible(design_ == "Desktop");
    scientific_->setVisible(design_ == "Desktop");
    for(auto box:{angle_,exact_,base_,notation_})box->setVisible(design_=="Desktop");
    for(auto label:{angleIndicator_,exactIndicator_,baseIndicator_,notationIndicator_})label->setVisible(design_=="Calculator");
    copy_->setVisible(design_ == "Desktop");
    settings_->setVisible(design_ != "Desktop");
    refreshStatus();
    result_->setAlignment(design_=="Desktop"?Qt::AlignLeft|Qt::AlignTop:Qt::AlignRight|Qt::AlignBottom);
    approximate_->setAlignment(design_ == "Desktop" ? Qt::AlignLeft | Qt::AlignVCenter : Qt::AlignRight | Qt::AlignVCenter);
    refreshAppearance(); setShiftState(shift_, alpha_);syncIndicators(); arrange(); updateGeometry(); update();
}
void CalculatorPanel::syncIndicators(){
    angleIndicator_->setText(angle_->currentText());angleIndicator_->setToolTip(angle_->accessibleName());
    exactIndicator_->setText(exact_->currentText());exactIndicator_->setToolTip(exact_->accessibleName());
    baseIndicator_->setText(base_->currentText());baseIndicator_->setToolTip(base_->accessibleName());
    notationIndicator_->setText(notation_->currentText());notationIndicator_->setToolTip(notation_->accessibleName());
    setMemoryActive(memoryActive_);setComplexEnabled(complexEnabled_);
    // History/config restoration can block the combo-box signals. Refresh the
    // radix-dependent keys whenever those restored indicators are synchronized.
    refreshDefinitions();arrange();
}
void CalculatorPanel::setStructuredInput(bool structured){
    structuredInput_=structured;editor_->setStructuredInput(structured);refreshDefinitions();refreshKeyTooltips();
}
void CalculatorPanel::setDecimalSeparator(QChar separator){
    decimalSeparator_=separator==','?',':'.';
    for(const auto& list:{desktopKeys_,lcdKeys_,expandedKeys_,customKeys_})for(auto key:list)key->setDecimalSeparator(decimalSeparator_);
    refreshDefinitions();refreshKeyTooltips();
}
void CalculatorPanel::setDesktopEffects(int effects){
    desktopEffects_=qBound(0,effects,3);for(auto key:desktopKeys_)key->setDesktopEffects(desktopEffects_);
    for(auto key:memories_)static_cast<CalculatorKey*>(key)->setDesktopEffects(desktopEffects_);update();
}
void CalculatorPanel::setKeyTooltipsEnabled(bool enabled) {
    keyTooltipsEnabled_=enabled;
    if(!enabled)QToolTip::hideText();
    refreshKeyTooltips();
}
void CalculatorPanel::setScreenTooltipsEnabled(bool enabled) {
    editor_->setProperty("screenTooltipsEnabled",enabled);result_->setProperty("screenTooltipsEnabled",enabled);
    screenTooltipsEnabled_=enabled;
    if(!enabled)QToolTip::hideText();
}
void CalculatorPanel::setCalculatorTheme(QString theme) {
    theme_=calculatorThemeDefinition(theme).name;
    for(const auto &list:{lcdKeys_,expandedKeys_,customKeys_})for(auto *key:list)key->setCalculatorTheme(theme_);
    refreshAppearance();arrange();update();
}
void CalculatorPanel::setCalculatorLayout(QString layout) {
    clearCustomLayout();
    calculatorLayout_=(layout=="Pocket"||layout=="Expanded")?layout:"Compact";
    refreshDefinitions();
    for(const auto &list:{desktopKeys_,lcdKeys_,expandedKeys_,customKeys_})for(auto *key:list)key->hide();
    for(auto *key:keyButtons())key->show();
    arrange();updateGeometry();
}
bool CalculatorPanel::setCustomLayout(const CustomKeypad::Layout &layout,QString *error) {
    const auto errors=CustomKeypad::validate(layout);
    if(!errors.isEmpty() || layout.stock) { if(error)*error=errors.isEmpty()?QStringLiteral("Create a custom copy before applying a stock layout."):errors.join('\n');return false; }
    customLayout_=layout;customWide_=width()>height();
    rebuildCustomKeys();arrange();updateGeometry();return true;
}
void CalculatorPanel::clearCustomLayout() {
    if(!customLayout_)return;
    customLayout_.reset();qDeleteAll(customKeys_);customKeys_.clear();
    refreshDefinitions();for(auto *key:keyButtons())key->show();arrange();updateGeometry();
}
void CalculatorPanel::rebuildCustomKeys() {
    qDeleteAll(customKeys_);customKeys_.clear();
    if(!customLayout_)return;
    const auto &profile=customWide_?customLayout_->landscape:customLayout_->portrait;
    for(int i=0;i<profile.keys.size();++i) {
        auto *button=new CalculatorKey({},this);button->setObjectName("customKey"+QString::number(i));
        button->setLook("Calculator");button->setCalculatorTheme(theme_);button->setDecimalSeparator(decimalSeparator_);
        connect(button,&QPushButton::clicked,this,[this,button]{dispatchKeyAction(button->token());});
        customKeys_.append(button);
    }
    for(const auto &list:{desktopKeys_,lcdKeys_,expandedKeys_})for(auto *key:list)key->hide();
    refreshDefinitions();for(auto *key:keyButtons())key->show();refreshKeyTooltips();
}
void CalculatorPanel::setComplexEnabled(bool enabled) {
    const bool changed=complexEnabled_!=enabled;
    complexEnabled_=enabled;numberKind_->setText(enabled?"CPLX":"REAL");
    numberKind_->setToolTip(translate_(enabled?"Complex numbers":"Real numbers"));
    if(changed)refreshDefinitions();
}
void CalculatorPanel::setComplexResult(bool complex) { complexResult_=complex;setComplexEnabled(complexEnabled_); }
void CalculatorPanel::setShiftState(bool shift, bool alpha) {
    shift_ = shift; alpha_ = alpha;
    for (auto *button : desktopKeys_) button->setModes(effectiveShift(), effectiveAlpha());
    for (auto *button : lcdKeys_) button->setModes(effectiveShift(), effectiveAlpha());
    for (auto *button : expandedKeys_) button->setModes(effectiveShift(), effectiveAlpha());
    for (auto *button : customKeys_) button->setModes(effectiveShift(), effectiveAlpha());
    setMemoryActive(memoryActive_);
    refreshKeyTooltips();setBusy(busy_);
}
void CalculatorPanel::dispatchKeyAction(const QString &token) {
    if(token.isEmpty())return;
    if(token=="@reg")setRegisterLayer(true);
    else if(token=="@regExit")setRegisterLayer(false);
    else if(token=="@modifierExit"){setRegisterLayer(false);heldShift_=heldAlpha_=false;setShiftState(false,false);}
    if(onAction)onAction(token);
}
void CalculatorPanel::setRegisterLayer(bool active) {
    if(registerLayer_==active)return;
    registerLayer_=active;
    shift_=alpha_=false;
    refreshDefinitions();refreshStatus();
    scientific_->setText(active?QString("REG"):translate_("Scientific"));
    arrange();update();
}
void CalculatorPanel::setLiteralInputBase(int base) {
    if(base!=2&&base!=8&&base!=16)base=0;
    if(literalInputBase_==base)return;
    literalInputBase_=base;refreshDefinitions();
}
void CalculatorPanel::refreshStatus() {
    const QString text=registerLayer_?QString("REG · ")+translate_("Press . or SHIFT to exit"):statusMessage_;
    status_->setText(text);status_->setToolTip(text);
    status_->setVisible(registerLayer_||(design_!="Desktop"&&!text.isEmpty()));
}
void CalculatorPanel::setHeldModifiers(bool shift, bool alpha) {
    if(heldShift_==shift&&heldAlpha_==alpha)return;
    heldShift_=shift;heldAlpha_=alpha;
    setShiftState(shift_,alpha_);
}
void CalculatorPanel::flashToken(QString token) {
    if (token == "@settings" || token == "@copy") {
        auto *button=token=="@settings" ? settings_ : copy_;
        if(button->isVisible() && button->isEnabled()) {
            button->setDown(true); QTimer::singleShot(145,button,[button]{button->setDown(false);});
        }
        return;
    }
    if (token == "=") token = "@eval";
    if (token == "×") token = "*"; else if (token == "÷") token = "/"; else if (token == "−") token = "-";
    auto keys = keyButtons();
    // Prefer the active caption before looking for its inactive layer.
    bool found = false;
    for (auto *key : keys) if (key->token() == token) { key->flash(); found = true; }
    if (!found) for (auto *key : keys) if (key->primaryToken() == token || key->alternateToken() == token || key->alphaToken()==token) key->flash();
    if (design_ == "Desktop") for (auto *button : memories_) {
        auto *key = static_cast<CalculatorKey *>(button); if (key->primaryToken() == token) key->flash();
    }
}
bool CalculatorPanel::activateToken(const QString &inputToken) {
    QString token=inputToken;
    if(registerLayer_) {
        if(token.size()==1&&token[0].isDigit())token="R"+token;
        else if(token=="."||token=="@shift")token="@regExit";
    }
    if(token=="@settings" && design_!="Desktop") { flashToken(token); if(settings_->isEnabled())settings_->click(); return true; }
    for (auto *button : keyButtons()) if (button->primaryToken() == token) {
        if (button->isEnabled()) { button->flash(); button->click(); }
        return true;
    }
    if (design_ == "Desktop") for (auto *button : memories_) {
        auto *key = static_cast<CalculatorKey *>(button);
        if (key->primaryToken() == token) { if (key->isEnabled()) { key->flash(); key->click(); } return true; }
    }
    return false;
}
void CalculatorPanel::setGearIcon(const QIcon &icon) {
    settings_->setIcon(icon); settings_->setText(icon.isNull() ? QString::fromUtf8("⚙") : QString());
    settings_->setIconSize(QSize(18,18));
}
void CalculatorPanel::setStatus(QString status) {
    statusMessage_ = std::move(status);refreshStatus();arrange();
}
void CalculatorPanel::setApproximation(QString text, int repeatStart, int repeatLength, QString exactCopyValue) {
    approximate_->setText(text); approximate_->setToolTip(text);
    static_cast<RecurringLabel *>(approximate_)->setRepeatingRange(repeatStart, repeatLength);
    static_cast<RecurringLabel *>(approximate_)->setExactCopyValue(exactCopyValue);
    arrange();
}
void CalculatorPanel::setMemoryActive(bool active) {
    memoryActive_ = active;
    QStringList indicators;
    if(registerLayer_&&design_!="Desktop")indicators<<"REG";
    else if(design_!="Desktop") { if(effectiveShift())indicators<<"SHIFT"; if(effectiveAlpha())indicators<<"ALPHA"; }
    if(active)indicators<<"M";
    memoryIndicator_->setText(indicators.join("  "));
    memoryIndicator_->setAccessibleName(active ? translate_("Recall memory") : QString());
}
void CalculatorPanel::setBusy(bool busy) {
    busy_ = busy;
    for (auto *key : desktopKeys_) if (key->primaryToken() == "@eval") key->setEnabled(!busy);
    for (auto *key : lcdKeys_+customKeys_) if (key->primaryToken() == "@eval" || key->primaryToken() == "@format") key->setEnabled(!busy);
}
void CalculatorPanel::retranslate(std::function<QString(const char *)> translate) {
    translate_ = std::move(translate);
    editor_->setUiTranslator(translate_);result_->setUiTranslator(translate_);argumentGuide_->setUiTranslator(translate_);
    scientific_->setText(registerLayer_?QString("REG"):translate_("Scientific"));
    refreshStatus();
    editor_->setPlaceholderText(translate_("Enter an expression…")); editor_->setAccessibleName(translate_("Expression"));
    result_->setAccessibleName(translate_("Result")); angle_->setAccessibleName(translate_("Angle unit"));
    exact_->setAccessibleName(translate_("Result format"));
    base_->setAccessibleName(translate_("Number base"));base_->setToolTip(translate_("Number base"));
    notation_->setAccessibleName(translate_("Number notation"));notation_->setToolTip(translate_("Number notation"));
    {QSignalBlocker block(notation_);notation_->setItemText(0,translate_("Normal"));}
    { QSignalBlocker block(exact_); exact_->setItemText(0,translate_("Exact")); exact_->setItemText(1,translate_("Decimal")); }
    copy_->setText(translate_("Copy")); copy_->setAccessibleName(translate_("Copy"));
    settings_->setToolTip(translate_("Settings")); settings_->setAccessibleName(translate_("Settings"));
    // Short mathematical captions stay universal; full localized names are available to assistive technology and tooltips.
    refreshKeyTooltips(); setMemoryActive(memoryActive_);setComplexEnabled(complexEnabled_);syncIndicators(); arrange();
}
void CalculatorPanel::refreshKeyTooltips() {
    const bool lcd=design_=="Calculator";
    const auto theme=calculatorThemeDefinition(theme_);
    const QColor ink=lcd?theme.bodyText:palette().color(QPalette::ToolTipText);
    const QColor background=lcd?theme.body:palette().color(QPalette::ToolTipBase);
    const QColor accent=lcd?theme.legend:palette().color(QPalette::Highlight);
    const QString shiftInk=blend(ink,accent,.55).name();
    const QString alphaInk=blend(ink,lcd?QColor("#69b9d8"):accent,.4).name();
    auto buttons=desktopKeys_+lcdKeys_+expandedKeys_+customKeys_;
    for(auto *button:memories_)buttons.append(static_cast<CalculatorKey *>(button));
    for(auto *button:buttons) {
        const auto &definition=button->definition();
        const auto describe=[&](const QString &token,const QString &label) {
            const QString description=token=="@complex"?translate_(complexEnabled_?"Real numbers":"Complex numbers"):actionDescription(token,translate_);
            return description==token?label:description;
        };
        QStringList accessible;
        QString rows;
        const auto appendTooltipRow=[&](const QString &layer,const QString &token,const QString &label,const QString &color) {
            if(token.isEmpty())return;
            const QString description=describe(token,label);
            accessible.append((layer.isEmpty()?QString():layer+": ")+description);
            const QString artwork=KeyArtwork::iconForAction(token);
            QString caption=token=="@recurring"?QString("<span style=\"text-decoration:overline\">123</span>"):label.toHtmlEscaped();
            if(!artwork.isEmpty()) {
                caption=KeyArtwork::imageHtml(artwork,description,artwork=="cube-root"?ink:QColor(color),DesktopFont::pixels(14),button->devicePixelRatioF());
            } else if(hasKeyCaption(label)) {
                caption=keyCaptionImageHtml(label,description,ink,DesktopFont::pixels(14),button->devicePixelRatioF());
            } else if(MathCaption::supports(token)) {
                caption=MathCaption::imageHtml(token,description,ink,DesktopFont::pixels(14),button->devicePixelRatioF());
            }
            const QString labelCell=layer.isEmpty()?caption:layer.toHtmlEscaped();
            QString detail=layer.isEmpty()?QString():"<b>"+caption+"</b>";
            if(description!=label)detail+=(detail.isEmpty()?QString():" &nbsp; ")+description.toHtmlEscaped();
            rows+=QString("<tr><td valign=\"top\" style=\"color:%1\"><b>%2</b></td><td valign=\"top\">%3</td></tr>")
                .arg(color,labelCell,detail);
        };
        const QString primaryLabel=definition.token=="."?QString(decimalSeparator_):definition.label;
        if(definition.primaryAvailable)appendTooltipRow({},definition.token,primaryLabel,ink.name());
        appendTooltipRow("SHIFT",definition.alternate,definition.secondary,shiftInk);
        appendTooltipRow("ALPHA",definition.alphaToken,definition.alphaLabel,alphaInk);
        const QString helpHtml=rows.isEmpty()?QString():QString("<html><table bgcolor=\"%1\" width=\"%2\" cellspacing=\"3\" cellpadding=\"3\" style=\"color:%3; border:1px solid %4\">").arg(background.name()).arg(qBound(220,width()-24,360)).arg(ink.name(),accent.name())+rows+"</table></html>";
        // Explicit inspection is independent of passive pointer-hover help.
        // Retain every layer's explanation even when hover tooltips are hidden.
        button->setProperty("keyHelpHtml",helpHtml);
        button->setToolTip(keyTooltipsEnabled_?helpHtml:QString());
        // Tooltip visibility is a visual preference; assistive technology keeps
        // the complete key description and all available modifier functions.
        button->setAccessibleDescription(accessible.join('\n'));
        const QString active=button->token();
        const QString activeLabel=active==definition.alphaToken?definition.alphaLabel:active==definition.alternate?definition.secondary:primaryLabel;
        const QString description=describe(active,activeLabel);
        if(!active.isEmpty()&&(active.startsWith('@')||description!=active))button->setAccessibleName(description);
    }
}
void CalculatorPanel::refreshAppearance() {
    if (refreshingPalette_) return;
    refreshingPalette_ = true;
    const bool lcd = design_ == "Calculator";
    const auto theme=calculatorThemeDefinition(theme_);
    const bool classic=lcd&&theme.beveled;
    QColor screenColor = palette().color(QPalette::Base), ink = palette().color(QPalette::Text), rim = palette().color(QPalette::Mid);
    if(lcd){screenColor=theme.display;ink=theme.text;rim=theme.border;}
    QString screenStyle = QString("QFrame#calculatorScreen {background:%1;border:%2px solid %3;border-radius:%4px;}"
        "QLabel#panelScientific,QLabel#panelMemoryIndicator,QLabel#panelApproximate,QLabel#panelStatus,QLabel#panelNumberKind,QLabel#panelAngleIndicator,QLabel#panelExactIndicator,QLabel#panelBaseIndicator,QLabel#panelNotationIndicator {color:%5;background:transparent;border:0;}"
        "QWidget#expression,QWidget#result {background:transparent;color:%5;border:0;padding:0;}"
        "QComboBox {background:%7;color:%5;border:1px solid %8;border-radius:%9px;padding:0 3px;font-size:10px;}"
        "QComboBox::drop-down {border:0;width:14px;background:transparent;}"
        "QComboBox QAbstractItemView {background:%1;color:%5;selection-background-color:%6;}"
        "QPushButton#panelCopy {background:%7;color:%5;border:1px solid %8;border-radius:%9px;padding:0 4px;font-size:10px;}"
        "QPushButton#panelCopy:pressed {background:%6;}"
        "QPushButton#panelSettings {background:transparent;color:%5;border:1px solid %8;border-radius:%9px;padding:1px;font-size:17px;}"
        "QPushButton#panelSettings:hover,QPushButton#panelSettings:focus {background:%7;border:1px solid %5;}"
        "QPushButton#panelSettings:pressed {background:%6;}"
        "QLabel#panelApproximate,QLabel#panelStatus {font-size:10px;}"
        "QLabel#panelMemoryIndicator,QLabel#panelNumberKind,QLabel#panelAngleIndicator,QLabel#panelExactIndicator,QLabel#panelBaseIndicator,QLabel#panelNotationIndicator {font-size:11px;}"
        "QLabel#panelBaseIndicator {border:1px solid %5;padding:0 2px;}")
        .arg(screenColor.name()).arg(classic ? 3 : 1).arg(rim.name()).arg(classic ? 1 : lcd ? 11 : 7)
        .arg(ink.name(),lcd?theme.accent.name():palette().color(QPalette::Highlight).name(), lcd ? blend(screenColor,ink,.07).name() : screenColor.name(),blend(screenColor,ink,.22).name())
        .arg(lcd&&!classic ? 8 : 3);
    QRegularExpression sizeExpression("font-size:([0-9]+)px");auto matches=sizeExpression.globalMatch(screenStyle);QVector<QRegularExpressionMatch> sizes;while(matches.hasNext())sizes<<matches.next();
    for(auto i=sizes.crbegin();i!=sizes.crend();++i)screenStyle.replace(i->capturedStart(1),i->capturedLength(1),QString::number(DesktopFont::pixels(i->captured(1).toInt())));
    if(screen_->styleSheet()!=screenStyle)screen_->setStyleSheet(screenStyle);
    auto documentPalette = lcd?calculatorThemePalette(theme_):palette(); documentPalette.setColor(QPalette::Base,screenColor); documentPalette.setColor(QPalette::Text,ink);
    result_->setPalette(documentPalette);editor_->setPalette(documentPalette);
    for(const auto &list:{desktopKeys_,lcdKeys_,expandedKeys_,customKeys_})for(auto *key:list)key->update();
    refreshKeyTooltips();
    refreshingPalette_ = false; update();
}
void CalculatorPanel::arrange() {
    if(arranging_)return;
    arranging_=true;
    if(customLayout_ && customWide_!=(width()>height())){customWide_=width()>height();rebuildCustomKeys();}
    const bool lcd=design_=="Calculator";
    for(auto *key:keyButtons())key->setProperty("compactKeyFaces",DesktopFont::presetScale()>1);
    const int margin=lcd?6:9, w=qMax(0,width()-margin*2), available=qMax(0,height()-margin*2);
    const bool landscape=lcd&&width()>height();
    const int displayW=w;
    const int gap=lcd?(width()<400?3:4):(height()<470?2:4);
    const int sciRows=calculatorLayout_=="Pocket"?3:calculatorLayout_=="Expanded"?6:5;
    int screenH=lcd?qBound(204,int(available*(calculatorLayout_=="Pocket"?.46:.41)),qMax(204,available-(sciRows*26+4*31+(sciRows+3)*gap+6))):(qMin(210,qMax(registerLayer_?146:128,qMin(int(available*.33),available-(7*32+6*gap+34)))));
    if(landscape)screenH=qBound(190,int(available*.46),qMax(190,available-qMax(sciRows*38,4*54)-6));
    int desktopInputNeed=36,desktopResultNeed=22;
    if(!lcd) {
        // Reserve actual mathematical content before assigning the remaining
        // height to keys. A result fraction needs both numerator and denominator,
        // and its secondary decimal line must not disappear in a short window.
        const auto measure=[displayW](MathEditor *view,int pixels,int minimum,int maximum) {
            QFont font=view->font();font.setPixelSize(DesktopFont::pixels(pixels));view->setFont(font);
            view->setStyleSheet(QString("font-size:%1px;").arg(DesktopFont::pixels(pixels)));
            view->resize(qMax(1,displayW-12),view->height());
            return view->text().isEmpty()?minimum:qBound(minimum,int(std::ceil(view->naturalSize().height())),maximum);
        };
        desktopInputNeed=measure(editor_,18,36,64);
        desktopResultNeed=measure(result_,19,22,72);
        const int secondaryNeed=!approximate_->isHidden()&&!approximate_->text().isEmpty()?15:0;
        const int hintNeed=registerLayer_?qBound(15,status_->fontMetrics().boundingRect(QRect(0,0,qMax(1,displayW-12),100),Qt::TextWordWrap,status_->text()).height()+1,29):0;
        screenH=qMin(210,qMax(screenH,54+6+desktopInputNeed+2+desktopResultNeed+secondaryNeed+hintNeed));
    }
    screen_->setGeometry(margin,margin,displayW,screenH);
    const qreal displayScale=lcd?qBound(1.0,qMin(displayW/500.0,screenH/185.0),2.0):1.0;
    const auto scaled=[displayScale](int n){return DesktopFont::pixels(qRound(n*displayScale));};
    const int inset=lcd?scaled(8):6,modeH=scaled(21);
    if(lcd){
        const QString indicatorStyle=QString("font-size:%1px;").arg(scaled(11));
        for(auto label:{angleIndicator_,exactIndicator_,baseIndicator_,notationIndicator_,numberKind_,memoryIndicator_})label->setStyleSheet(indicatorStyle);
        status_->setStyleSheet(QString("font-size:%1px;").arg(scaled(10)));
        approximate_->setStyleSheet(QString("font-size:%1px;").arg(scaled(10)));
    }else{
        for(auto label:{angleIndicator_,exactIndicator_,baseIndicator_,notationIndicator_,numberKind_,memoryIndicator_,status_,approximate_})label->setStyleSheet(QString());
    }
    settings_->setIconSize(QSize(scaled(18),scaled(18)));
    const int copyW=qBound(42,copy_->fontMetrics().horizontalAdvance(copy_->text())+12,76);
    const int exactW=qBound(68,qMax(exact_->fontMetrics().horizontalAdvance(exact_->itemText(0)),exact_->fontMetrics().horizontalAdvance(exact_->itemText(1)))+22,106);
    const int angleW=61;
    copy_->setGeometry(displayW-inset-copyW,5,copyW,modeH);
    settings_->setGeometry(displayW-inset-scaled(26),scaled(5),scaled(26),modeH);
    exact_->setGeometry(lcd?settings_->x()-6-exactW:copy_->x()-5-exactW,5,exactW,modeH);
    angle_->setGeometry(lcd?inset:exact_->x()-5-angleW,5,angleW,modeH);
    if(lcd){memoryIndicator_->setGeometry(inset+angleW+5,5,qMax(0,exact_->x()-inset-angleW-10),modeH);memoryIndicator_->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);}
    else{memoryIndicator_->setGeometry(angle_->x()-23,5,19,modeH);memoryIndicator_->setAlignment(Qt::AlignCenter);}
    scientific_->setGeometry(inset,5,qMax(0,angle_->x()-inset-27),modeH);
    const int notationW=qBound(72,notation_->fontMetrics().horizontalAdvance(notation_->itemText(0))+23,113);
    base_->setGeometry(inset,29,61,modeH);
    notation_->setGeometry(inset+67,29,notationW,modeH);
    numberKind_->setGeometry(inset+74+notationW,29,qMax(24,displayW-2*inset-74-notationW),modeH);
    if(lcd){
        angleIndicator_->setGeometry(inset,scaled(6),scaled(43),scaled(20));
        const int exactX=qMax(scaled(148),settings_->x()-scaled(95));
        exactIndicator_->setGeometry(exactX,scaled(6),qMin(scaled(89),settings_->x()-exactX-scaled(5)),scaled(20));
        memoryIndicator_->setGeometry(inset+scaled(51),scaled(6),qMax(scaled(40),exactIndicator_->x()-inset-scaled(57)),scaled(20));
        baseIndicator_->setGeometry(inset,scaled(30),scaled(39),scaled(18));
        const int indicatorNotationW=qMin(scaled(102),scaled(notationW));
        notationIndicator_->setGeometry(inset+scaled(49),scaled(29),indicatorNotationW,scaled(20));
        numberKind_->setGeometry(inset+scaled(58)+indicatorNotationW,scaled(29),scaled(48),scaled(20));
    }
    const int statusH=(lcd||registerLayer_)&&!status_->text().isEmpty()?qBound(scaled(15),status_->fontMetrics().boundingRect(QRect(0,0,qMax(1,displayW-2*inset),100),Qt::TextWordWrap,status_->text()).height()+1,scaled(29)):0;
    const int approximateH=!approximate_->isHidden()&&!approximate_->text().isEmpty()?scaled(15):0;
    status_->setGeometry(inset,screenH-statusH-5,qMax(1,displayW-2*inset),statusH);
    approximate_->setGeometry(inset,screenH-statusH-approximateH-5,qMax(1,displayW-2*inset),approximateH);
    const int guideH=argumentGuide_->isHidden()?0:argumentGuide_->heightForWidth(displayW-2*inset);
    argumentGuide_->setGeometry(inset,screenH-statusH-approximateH-guideH-5,qMax(1,displayW-2*inset),guideH);
    const int contentY=scaled(54),contentH=qMax(40,screenH-contentY-statusH-approximateH-guideH-6);
    const int inputH=lcd?qMax(43,int(contentH*.52)):
        qBound(28,qMax(desktopInputNeed,int(contentH*.48)),qMax(28,contentH-desktopResultNeed-2));
    const int resultH=qMax(22,contentH-inputH-2);
    editor_->setGeometry(inset,contentY,qMax(1,displayW-inset*2),inputH);
    result_->setGeometry(inset,contentY+inputH+2,qMax(1,displayW-inset*2),resultH);
    const int editorPixels=DesktopFont::pixels(lcd?qBound(19,qMin(inputH/3,displayW/12),44):qBound(18,inputH/3,25));
    const int resultPixels=DesktopFont::pixels(lcd?qBound(20,qMin(resultH/2,displayW/10),48):qBound(19,resultH/2,28));
    QFont f=editor_->font();f.setPixelSize(editorPixels);editor_->setFont(f);
    f=result_->font();f.setPixelSize(resultPixels);result_->setFont(f);
    editor_->setStyleSheet(QString("font-size:%1px;").arg(editorPixels));
    result_->setStyleSheet(QString("font-size:%1px;").arg(resultPixels));
    // A simple stacked fraction should be visible in full even at 320×540.
    // Longer multiline expressions retain scrolling rather than becoming tiny.
    const auto fitSimple=[](MathEditor *view,int pixels){
        if((DesktopFont::presetScale()>1)||view->text().isEmpty()||view->visualLineCount()>2)return;
        while(pixels>15&&view->naturalSize().height()>view->height()){
            --pixels;QFont f=view->font();f.setPixelSize(pixels);view->setFont(f);
            view->setStyleSheet(QString("font-size:%1px;").arg(pixels));
        }
    };
    editor_->setProperty("preserveMathScale",(DesktopFont::presetScale()>1));result_->setProperty("preserveMathScale",(DesktopFont::presetScale()>1));
    fitSimple(editor_,editorPixels);fitSimple(result_,resultPixels);
    int y=margin+screenH+5;
    if(lcd&&customLayout_){
        const auto &profile=customWide_?customLayout_->landscape:customLayout_->portrait;
        const int keyH=qMax(1,height()-margin-y);
        for(int i=0;i<customKeys_.size();++i){const auto &k=profile.keys[i];
            const int x0=margin+k.column*(w+gap)/profile.columns,x1=margin+(k.column+k.columnSpan)*(w+gap)/profile.columns-gap;
            const int y0=y+k.row*(keyH+gap)/profile.rows,y1=y+(k.row+k.rowSpan)*(keyH+gap)/profile.rows-gap;
            customKeys_[i]->setGeometry(x0,y0,qMax(1,x1-x0),qMax(1,y1-y0));}
        arranging_=false;return;
    }
    if(!lcd){
        const int memoryH=height()<470?24:28;
        for(int i=0;i<memories_.size();++i){int x0=margin+i*(w+gap)/4,x1=margin+(i+1)*(w+gap)/4-gap;memories_[i]->setGeometry(x0,y,x1-x0,memoryH);}
        y+=memoryH+5;
        const int gridH=qMax(7*20,height()-margin-y);
        for(int i=0;i<desktopKeys_.size();++i){int row=i/6,col=i%6;int x0=margin+col*(w+gap)/6,x1=margin+(col+1)*(w+gap)/6-gap,y0=y+row*(gridH+gap)/7,y1=y+(row+1)*(gridH+gap)/7-gap;desktopKeys_[i]->setGeometry(x0,y0,x1-x0,y1-y0);}
    }else{
        const int gridH=qMax(landscape?qMax(sciRows,4)*22:(sciRows+4)*22,height()-margin-y);
        const int scientificW=landscape?int((w-8)*.52):w,numericW=landscape?w-scientificW-8:w;
        const int scientificH=landscape?gridH:qMax(sciRows*28+(sciRows-1)*gap,gridH-int((gridH-5)*(5.0/(sciRows+5.0)))-5);
        const int numericH=landscape?gridH:gridH-scientificH-5;
        const int numericX=landscape?margin+scientificW+8:margin,numericY=landscape?y:y+scientificH+5;
        const auto keys=keyButtons();const int sciCount=7+(sciRows-1)*6;
        for(int i=0;i<sciCount;++i){int row=i<7?0:1+(i-7)/6,col=i<7?i:(i-7)%6,cols=i<7?7:6;int x0=margin+col*(scientificW+gap)/cols,x1=margin+(col+1)*(scientificW+gap)/cols-gap,y0=y+row*(scientificH+gap)/sciRows,y1=y+(row+1)*(scientificH+gap)/sciRows-gap;keys[i]->setGeometry(x0,y0,x1-x0,y1-y0);}
        for(int i=sciCount;i<keys.size();++i){int row=(i-sciCount)/5,col=(i-sciCount)%5;int x0=numericX+col*(numericW+gap)/5,x1=numericX+(col+1)*(numericW+gap)/5-gap,y0=numericY+row*(numericH+gap)/4,y1=numericY+(row+1)*(numericH+gap)/4-gap;keys[i]->setGeometry(x0,y0,x1-x0,y1-y0);}
    }
    arranging_=false;
}
void CalculatorPanel::paintEvent(QPaintEvent *) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    QColor body=palette().color(QPalette::Base), border=palette().color(QPalette::Mid);
    const auto theme=calculatorThemeDefinition(theme_);const bool lcd=design_=="Calculator";
    if(lcd){body=theme.body;border=theme.border;}
    if(!lcd&&desktopEffects_>0)body.setAlpha(desktopEffects_==3?(body.lightnessF()<.5?65:85):desktopEffects_==1?125:220);
    QLinearGradient gradient(rect().topLeft(),rect().bottomRight());gradient.setColorAt(0,body.lighter(104));gradient.setColorAt(1,body.darker(103));
    if(lcd&&theme.metal){gradient.setColorAt(0,QColor("#dee3e4"));gradient.setColorAt(.24,body.darker(105));gradient.setColorAt(.51,body.lighter(116));gradient.setColorAt(.83,body.darker(106));}
    p.setBrush(gradient);p.setPen(QPen(border,1));const int radius=lcd?(theme.beveled?1:8):10;
    p.drawRoundedRect(QRectF(rect()).adjusted(.5,.5,-.5,-.5),radius,radius);
    if(lcd&&theme.beveled) {p.setPen(QPen(QColor(255,255,235,45),2));p.drawLine(3,3,width()-3,3);p.drawLine(3,3,3,height()-3);}
    if(lcd&&theme.metal){p.setPen(QColor(255,255,255,16));for(int y=3;y<height()-2;y+=3)p.drawLine(2,y,width()-3,y);}
}
void CalculatorPanel::resizeEvent(QResizeEvent *e) { QWidget::resizeEvent(e); arrange(); }
void CalculatorPanel::changeEvent(QEvent *e) {
    QWidget::changeEvent(e);
    if(e->type()==QEvent::PaletteChange || e->type()==QEvent::ApplicationPaletteChange) refreshAppearance();
    if(e->type()==QEvent::FontChange)QTimer::singleShot(0,this,[this]{refreshAppearance();arrange();});
}
