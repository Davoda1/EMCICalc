// SPDX-License-Identifier: MIT
// Native key-caption regressions: complete integral differential and balanced
// U+21D4 spacing, including modifier legends, tooltips and the real main window.
#include "mainwindow.h"
#include "fontscale.h"
#include "calculatorthemes.h"
#include <QtTest/QTest>
#include <iostream>

class DesktopAcceptanceTests {
 int checks_=0,failures_=0;QString directory_;QJsonArray evidence_;
 void check(bool ok,const QString &label){++checks_;if(!ok)++failures_;qInfo().noquote()<<(ok?"PASS":"FAIL")<<label;}
 void settle(){for(int i=0;i<3;++i){QCoreApplication::processEvents();QCoreApplication::sendPostedEvents(nullptr,QEvent::LayoutRequest);QTest::qWait(8);}}
 static QImage render(CalculatorKey &key,qreal ratio){QImage image(QSize(qCeil(key.width()*ratio),qCeil(key.height()*ratio)),QImage::Format_ARGB32_Premultiplied);image.setDevicePixelRatio(ratio);image.fill(Qt::transparent);key.render(&image);return image;}
 static QList<QRectF> parts(CalculatorKey &key,const char *name){QList<QRectF> result;for(const auto &part:key.property(name).toList())result.append(part.toRectF());return result;}
 static int difference(QRgb a,QRgb b){return qMax(qAbs(qRed(a)-qRed(b)),qMax(qAbs(qGreen(a)-qGreen(b)),qAbs(qBlue(a)-qBlue(b))));}
 static QRect occupied(const QImage &image,const QImage &blank,const QRectF &area,qreal ratio){
  // Detect antialiased ink at consistent coverage, rather than a fixed RGB
  // delta that rejects more edge pixels on lower-contrast skins. The blank
  // face is otherwise identical, so its maximum difference is stroke contrast.
  int contrast=0;for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x)contrast=qMax(contrast,difference(image.pixel(x,y),blank.pixel(x,y)));
  const int threshold=qMax(1,qRound(contrast*.20));QRect result;const QRect pixels=QRectF(area.x()*ratio,area.y()*ratio,area.width()*ratio,area.height()*ratio).adjusted(-.75,-.75,.75,.75).toAlignedRect().intersected(image.rect());for(int y=pixels.top();y<=pixels.bottom();++y)for(int x=pixels.left();x<=pixels.right();++x)if(difference(image.pixel(x,y),blank.pixel(x,y))>threshold)result=result.united(QRect(x,y,1,1));return result;}
 static QJsonArray rect(const QRectF &r){return {r.x(),r.y(),r.width(),r.height()};}
 void geometry(CalculatorKey &key,const char *property,int count,const QString &label){
  const auto list=parts(key,property);check(list.size()==count,label+": complete caption components");if(list.size()!=count)return;
  QRectF total;bool contained=true;for(const auto &part:list){total=total.united(part);contained&=key.rect().contains(part.toAlignedRect());}
  check(contained,label+": all caption ink rectangles contained");
  check(list[1].left()-list[0].right()>=.9,label+": explicit nonzero ink gap");
  if(count==3)check(qAbs((list[1].left()-list[0].right())-(list[2].left()-list[1].right()))<.01,label+": equal S/arrow and arrow/D ink gaps");
 }
 void rendererSweep(){
  const qreal ratio=qApp->devicePixelRatio();
  for(const auto &skin:calculatorThemeNames())for(const auto &family:QStringList{"DejaVu Sans","Liberation Sans","DejaVu Sans Mono"})for(const auto &size:{QSize(46,36),QSize(64,50),QSize(92,64)}){
   QFont font(family);font.setStyleStrategy(QFont::NoFontMerging);
   for(const auto &label:QStringList{"∫ dx","S⇔D"}){
    KeyDefinition definition{label,label=="∫ dx"?"integrate(":"@format"};CalculatorKey key(definition),blank({"",""});
    for(auto *button:{&key,&blank}){button->setLook("Calculator");button->setCalculatorTheme(skin);button->setFont(font);button->resize(size);}
    const QImage pixels=render(key,ratio),background=render(blank,ratio);const int count=label=="∫ dx"?2:3;
    const QString name=skin+" / "+family+QString(" / %1x%2 / ").arg(size.width()).arg(size.height())+label;
    geometry(key,"primaryCaptionParts",count,name);const auto list=parts(key,"primaryCaptionParts");if(list.size()!=count)continue;
    QList<QRect> ink;bool drawn=true;for(const auto &part:list){ink.append(occupied(pixels,background,part,ratio));drawn&=!ink.last().isEmpty();}
    check(drawn,name+": actual renderer draws every component (including dx)");
    if(count==3&&drawn){const int leftGap=ink[1].left()-ink[0].right()-1,rightGap=ink[2].left()-ink[1].right()-1;const bool balanced=leftGap>=0&&rightGap>=0&&qAbs(leftGap-rightGap)<=2;
     if(!balanced){qWarning()<<"CAPTION RASTER DIAGNOSTIC"<<name<<"ratio"<<ratio<<"gaps"<<leftGap<<rightGap<<"measured ink"<<ink<<"layout"<<list;if(!directory_.isEmpty()){pixels.save(QDir(directory_).filePath("renderer-failure-caption.png"));background.save(QDir(directory_).filePath("renderer-failure-background.png"));}}
     check(balanced,name+": rasterized gaps stay separated and balanced within two physical pixels");}
    check(key.token()==definition.token&&key.accessibleName()==label,name+": insertion action and accessible label unchanged");
   }
  }
  // Custom labels may also be displayed on Desktop key faces. Preserve the
  // shared caption geometry under each existing Desktop surface effect.
  for(const auto &appearance:QStringList{"Light","Dark"})for(int effect=0;effect<4;++effect)for(const auto &label:QStringList{"∫ dx","S⇔D"}){
   QPalette palette;palette.setColor(QPalette::Button,appearance=="Light"?QColor("#eeeeee"):QColor("#30343b"));palette.setColor(QPalette::ButtonText,appearance=="Light"?Qt::black:Qt::white);qApp->setPalette(palette);
   CalculatorKey key({label,label=="∫ dx"?"integrate(":"@format","∫ dx","integrate(",0,"S⇔D","@format"});key.setLook("Desktop");key.setDesktopEffects(effect);key.resize(140,48);render(key,ratio);
   geometry(key,"primaryCaptionParts",label=="∫ dx"?2:3,appearance+QString(" Desktop effect%1 ").arg(effect)+label);
   geometry(key,"secondaryCaptionParts",2,"Desktop inactive integral caption");geometry(key,"tertiaryCaptionParts",3,"Desktop inactive format caption");
  }
  qApp->setPalette(qApp->style()->standardPalette());
  // At readable large size, the vector is demonstrably a double arrow: two
  // occupied horizontal bands at its middle, with an unpainted central band.
  CalculatorKey arrow({"S⇔D","@format"}),blank({"",""});for(auto *key:{&arrow,&blank}){key->setLook("Calculator");key->resize(180,100);}
  const auto pixels=render(arrow,ratio),background=render(blank,ratio);const auto list=parts(arrow,"primaryCaptionParts");
  if(list.size()==3){const auto area=list[1];const int x=qRound(area.center().x()*ratio);int runs=0;bool prior=false;for(int y=qCeil(area.top()*ratio);y<=qFloor(area.bottom()*ratio);++y){const bool ink=difference(pixels.pixel(x,y),background.pixel(x,y))>45;if(ink&&!prior)++runs;prior=ink;}check(runs==2,"S⇔D vector preserves the double horizontal shaft of U+21D4");}
  CalculatorKey integral({"∫ dx","integrate(","d/dx","diff(",0,"max","max("}),menu({"MENU","@menu","STAT","@statistics",0,"S⇔D","@format"});
  for(auto *key:{&integral,&menu}){key->setLook("Calculator");key->resize(78,60);}
  integral.setModes(true,false);render(integral,ratio);geometry(integral,"secondaryCaptionParts",2,"SHIFT integral legend");check(integral.token()=="diff("&&integral.accessibleName()=="d/dx","SHIFT still activates derivative with unchanged accessible name");
  integral.setModes(false,true);render(integral,ratio);geometry(integral,"tertiaryCaptionParts",2,"ALPHA integral legend");check(integral.token()=="max(","ALPHA still activates max");
  integral.setModes(false,false);render(integral,ratio);geometry(integral,"primaryCaptionParts",2,"released integral modifiers");
  render(menu,ratio);geometry(menu,"tertiaryCaptionParts",3,"MENU inactive ALPHA format legend");menu.setModes(false,true);render(menu,ratio);geometry(menu,"primaryCaptionParts",3,"MENU selected ALPHA format caption");check(menu.token()=="@format"&&menu.accessibleName()=="S⇔D","MENU ALPHA action and accessibility unchanged");
 }
 void capture(MainWindow &w,const QString &name){
  settle();const QPixmap pixmap=w.grab();QJsonArray keys;
  for(auto *key:w.calculator_->keyButtons())if(key->primaryToken()=="integrate("||key->primaryToken()=="@format"||key->primaryToken()=="@menu"){
   key->grab();const int count=key->primaryToken()=="integrate("?2:3;
   if(key->primaryToken()=="@menu"&&key->alphaToken()!="@format") {check(key->alphaToken().isEmpty(),name+": Compact keeps established duplicate-action removal on MENU");continue;}
   const bool inactiveMenu=key->primaryToken()=="@menu"&&key->token()!="@format";
   if(inactiveMenu&&key->property("compactKeyFace").toBool())check(key->property("keyHelpHtml").toString().contains("data-key-caption=\"S⇔D\""),name+": compact MENU retains complete ALPHA help");
   else geometry(*key,inactiveMenu?"tertiaryCaptionParts":"primaryCaptionParts",count,name+" / "+key->primaryToken());
   // Large compact faces intentionally hide inactive legends. Their complete
   // composite captions remain in the separately verified help image.
   QJsonArray caption;for(const auto &part:parts(*key,"primaryCaptionParts"))caption.append(rect(part));keys.append(QJsonObject{{"token",key->primaryToken()},{"key_rect",rect(QRectF(key->mapTo(&w,QPoint()),key->size()))},{"primary_parts",caption}});
  }
  if(!directory_.isEmpty())check(pixmap.save(QDir(directory_).filePath(name+".png")),name+": native full application capture saved");
  evidence_.append(QJsonObject{{"name",name},{"width",w.width()},{"height",w.height()},{"font",w.fontSizeMode_},{"presentation",w.design_},{"skin",w.calculatorTheme_},{"appearance",w.appearance_},{"device_pixel_ratio",w.devicePixelRatioF()},{"keys",keys}});
 }
 void appSweep(){
  MainWindow w(true);w.show();QElapsedTimer timer;timer.start();while((!w.catalogReady_||!w.registersReady_)&&timer.elapsed()<15000)settle();check(w.catalogReady_&&w.registersReady_,"native app worker and metadata ready");
  capture(w,"desktop-fresh");
  w.designBox_->setCurrentIndex(w.designBox_->findData("Calculator"));if(w.settingsDraftActive_)w.applySettingsDraft();w.showPage(0);w.resize(320,540);
  w.calculatorLayoutBox_->setCurrentIndex(w.calculatorLayoutBox_->findData("Compact"));w.fontSizeBox_->setCurrentIndex(w.fontSizeBox_->findData("Medium"));
  for(const auto &appearance:QStringList{"Light","Dark"}){w.appearanceBox_->setCurrentIndex(w.appearanceBox_->findData(appearance));for(const auto &skin:calculatorThemeNames()){
   w.calculatorThemeBox_->setCurrentIndex(w.calculatorThemeBox_->findData(skin));capture(w,"calculator-"+QString(skin).replace(' ','-').toLower()+"-"+appearance.toLower()+"-320");
  }}
  w.calculatorThemeBox_->setCurrentIndex(w.calculatorThemeBox_->findData("Classic"));w.appearanceBox_->setCurrentIndex(w.appearanceBox_->findData("Light"));w.resize(480,800);capture(w,"calculator-classic-medium-480");
  w.calculator_->setKeyTooltipsEnabled(true);bool integral=false,format=false;QString formatAccessible;
  const auto verifyHelp=[&](CalculatorKey *key){
   const QString help=key->property("keyHelpHtml").toString();const QString caption=key->primaryToken()=="integrate("?"∫ dx":"S⇔D";
   check(help.contains("data-key-caption=\""+caption+"\"")&&help.contains("base64,"),key->primaryToken()+": tooltip uses the complete matching composite image");
   const QRegularExpression encoded("<img data-key-caption=\""+QRegularExpression::escape(caption)+"\"[^>]*src=\"data:image/png;base64,([^\"]+)\"");const auto match=encoded.match(help);QImage helpImage;if(match.hasMatch())helpImage.loadFromData(QByteArray::fromBase64(match.captured(1).toLatin1()),"PNG");
   bool leftInk=false,rightInk=false;for(int y=0;y<helpImage.height();++y)for(int x=0;x<helpImage.width();++x)if(qAlpha(helpImage.pixel(x,y))>100){leftInk|=x<helpImage.width()/3;rightInk|=x>helpImage.width()*2/3;}
   check(!helpImage.isNull()&&helpImage.width()>helpImage.height()&&leftInk&&rightInk,key->primaryToken()+": matching tooltip raster contains both ends of the complete caption");
   check(!key->accessibleDescription().contains("<img")&&!key->accessibleDescription().contains("base64"),key->primaryToken()+": accessible description remains plain text");
  };
  for(auto *key:w.calculator_->keyButtons())if(key->primaryToken()=="integrate("||key->primaryToken()=="@format") {verifyHelp(key);if(key->primaryToken()=="integrate(")integral=true;else {format=true;formatAccessible=key->accessibleName();}}
  check(integral&&format,"established integral and dedicated result-format keys remain present");
  // Compact removes MENU's duplicate format action and
  // restores it in Pocket, where the dedicated format key is absent.
  w.calculatorLayoutBox_->setCurrentIndex(w.calculatorLayoutBox_->findData("Pocket"));w.resize(320,540);settle();CalculatorKey *menu=nullptr;
  for(auto *key:w.calculator_->keyButtons())if(key->primaryToken()=="@menu")menu=key;
  check(menu&&menu->alphaToken()=="@format","Pocket preserves established MENU ALPHA format action");if(menu)verifyHelp(menu);capture(w,"calculator-pocket-medium-320");
  if(menu)menu->setProperty("diagnoseLegendFit",true);
  w.handleAction("@shift");settle();check(menu&&menu->token()=="@statistics","Pocket SHIFT keeps the Statistics action");capture(w,"calculator-pocket-shift-320");
  if(menu&&!menu->property("legendsFit").toBool())qWarning()<<"POCKET SHIFT LEGEND FIT DIAGNOSTIC"
   <<"Qt"<<qVersion()<<"platform"<<QGuiApplication::platformName()<<"ratio"<<menu->devicePixelRatioF()
   <<"applicationFont"<<qApp->font()<<"keyFont"<<menu->font()<<"keyRect"<<menu->rect()
   <<"legendArea"<<menu->property("secondaryLabelArea")<<"legends"<<menu->property("legendFitDiagnostics")
   <<"tertiaryParts"<<menu->property("tertiaryCaptionParts");
  check(menu&&menu->property("legendsFit").toBool(),"Pocket SHIFT validates the actually fitted inactive composite legend");
  bool plainMenuFits=false;
  if(menu)for(const auto &entry:menu->property("legendFitDiagnostics").toList()){
   const auto fit=entry.toMap();if(fit.value("label").toString()!="MENU"||fit.value("method").toString()!="plain-ink")continue;
   const QRectF target=fit.value("target").toRectF(),ink=fit.value("tightInk").toRectF(),painted=fit.value("paintedInk").toRectF();const qreal scale=fit.value("inkScale").toDouble();
   plainMenuFits=!target.isEmpty()&&!ink.isEmpty()&&!painted.isEmpty()&&scale>0&&scale<=1
    &&qAbs(painted.width()-ink.width()*scale)<.01&&qAbs(painted.height()-ink.height()*scale)<.01
    &&target.adjusted(-.01,-.01,.01,.01).contains(painted)
    &&(ink.width()<=target.width()||scale<1);
  }
  check(plainMenuFits,"Pocket SHIFT plain MENU uses complete fitted ink inside its allocated legend target");
  if(menu)menu->setProperty("diagnoseLegendFit",false);
  w.handleAction("@shift");
  w.handleAction("@alpha");settle();check(menu&&menu->token()=="@format"&&menu->accessibleName()==formatAccessible&&!formatAccessible.isEmpty(),"actual Pocket ALPHA selects format action and matching semantic accessible name");capture(w,"calculator-pocket-alpha-320");w.handleAction("@alpha");
  w.calculatorLayoutBox_->setCurrentIndex(w.calculatorLayoutBox_->findData("Compact"));
  for(const auto &font:QStringList{"Small","Large"}){w.fontSizeBox_->setCurrentIndex(w.fontSizeBox_->findData(font));for(const auto &size:{QSize(320,540),QSize(480,800)}){w.resize(size);capture(w,"calculator-classic-"+font.toLower()+QString("-%1").arg(size.width()));}}
  w.saveTimer_->stop();
 }
public:
 int run(const QString &directory){directory_=directory;if(!directory_.isEmpty())QDir().mkpath(directory_);rendererSweep();appSweep();
  if(!directory_.isEmpty()){QFile file(QDir(directory_).filePath("caption-verification.json"));check(file.open(QIODevice::WriteOnly),"caption evidence manifest writable");if(file.isOpen())file.write(QJsonDocument(QJsonObject{{"schema",1},{"product_version","1.0.0"},{"qt",qVersion()},{"platform",QGuiApplication::platformName()},{"style",qApp->style()->objectName()},{"device_pixel_ratio",qApp->devicePixelRatio()},{"checks",checks_},{"failures",failures_},{"provenance","Native GNU/Linux application widgets; Qt platform stated; no mockup"},{"captures",evidence_}}).toJson());}
  std::cout<<checks_-failures_<<'/'<<checks_<<" desktop key caption checks passed\n";return failures_?1:0;}
};
int main(int argc,char **argv){QTemporaryDir profile;if(!profile.isValid())return 2;QVector<QByteArray> args;for(int i=0;i<argc;++i)args<<argv[i];args<<"--config-root"<<profile.path().toUtf8();QVector<char*> pointers;for(auto &arg:args)pointers<<arg.data();int count=pointers.size();pointers<<nullptr;QApplication app(count,pointers.data());app.setStyle("Fusion");QCoreApplication::setApplicationName("EMCICalc");DesktopFont::initialize(app);QString output;const int at=args.indexOf("--screenshots");if(at>=0&&at+1<args.size())output=QString::fromUtf8(args[at+1]);return DesktopAcceptanceTests().run(output);}
