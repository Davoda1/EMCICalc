// SPDX-License-Identifier: MIT
// Render the real shared controls. Compare their compact caption ink with the
// same control given generous vertical space; geometry assertions alone missed
// the Windows clipping reported for Plot / Add curve / graph section buttons.
#include "desktopworkbench.h"
#include "workbenchbutton.h"
#include "calculatorthemes.h"
#include <QtTest/QTest>
#include <iostream>
#include <memory>

namespace {
int checks=0,failures=0;
void check(bool ok,const QString &message){++checks;if(!ok){++failures;qWarning().noquote()<<"FAIL"<<message;}}
void settle(){QApplication::processEvents();QCoreApplication::sendPostedEvents(nullptr,QEvent::LayoutRequest);QApplication::processEvents();}
struct Ink {QRect bounds;int pixels=0;QRgb dominant=0;};
Ink difference(const QImage &caption,const QImage &blank){
 Ink result;QHash<QRgb,int> histogram;int most=0;
 for(int y=0;y<caption.height();++y)for(int x=0;x<caption.width();++x){const auto a=caption.pixel(x,y),b=blank.pixel(x,y);const int delta=qAbs(qRed(a)-qRed(b))+qAbs(qGreen(a)-qGreen(b))+qAbs(qBlue(a)-qBlue(b));if(delta>4){result.bounds=result.bounds.united(QRect(x,y,1,1));++result.pixels;if(delta>60){const int count=++histogram[a];if(count>most){most=count;result.dominant=a;}}}}
 return result;
}
Ink captionInk(QPushButton &button){const auto caption=button.text();const QImage full=button.grab().toImage();button.setText("");const QImage blank=button.grab().toImage();button.setText(caption);return difference(full,blank);}
QPushButton *makeButton(bool workbench,const QString &text,QWidget *parent){return workbench?static_cast<QPushButton*>(new WorkbenchButton(text,parent)):static_cast<QPushButton*>(new DesktopWorkbench::ActionButton(text,parent));}
QString stylesheet(const QString &kind){
 if(kind=="native")return {};
 if(kind=="Classic"){auto css=calculatorThemeStyleSheet("Classic");css.remove("QWidget{font-family:'Segoe UI','Noto Sans',sans-serif;} ");return css+"QPushButton:checked{background:#97c0b4;color:#10221b;} QPushButton:disabled{background:#4e554f;color:#999e99;}";}
 // The desktop's actual button padding and thicker keyboard-focus border.
 return "QPushButton{padding:5px 9px;border:1px solid #718078;border-radius:5px;background:#edf1ee;color:#192720;} QPushButton:focus{border:2px solid #23765c;} QPushButton:pressed,QPushButton:checked{background:#23765c;color:#ffffff;} QPushButton:disabled{background:#dfe4e0;color:#737b76;}";
}
void state(QPushButton &button,const QString &name,QWidget &focusSink){
 button.setEnabled(true);button.setDown(false);button.setCheckable(name=="checked");button.setChecked(name=="checked");focusSink.setFocus();
 if(name=="focused"||name=="pressed")button.setFocus();if(name=="pressed")button.setDown(true);if(name=="disabled")button.setEnabled(false);settle();
}
QString pixels(const Ink &ink){return QString("%1x%2/%3px").arg(ink.bounds.width()).arg(ink.bounds.height()).arg(ink.pixels);}
}
int main(int argc,char **argv){
 QApplication app(argc,argv);QString output;const auto args=app.arguments();const int at=args.indexOf("--screenshots");if(at>=0&&at+1<args.size()){output=args[at+1];QDir().mkpath(output);}
 const QFont baseline=app.font();QStringList styles={app.style()->objectName().toLower(),"fusion","windows"};styles.removeDuplicates();
 qInfo().noquote()<<"Qt"<<qVersion()<<"platform"<<QGuiApplication::platformName()<<"OS"<<QSysInfo::prettyProductName()<<"scale"<<qEnvironmentVariable("QT_SCALE_FACTOR","default");
 int cases=0;
 for(const auto &styleName:styles){auto *native=QStyleFactory::create(styleName);if(!native){qInfo().noquote()<<"SKIP unavailable style"<<styleName;continue;}app.setStyle(native);
  for(const auto &theme:{QString("native"),QString("desktop"),QString("Classic")}){
   app.setStyleSheet(stylesheet(theme));app.setPalette(theme=="Classic"?calculatorThemePalette("Classic"):app.style()->standardPalette());
   QWidget host;host.setFocusPolicy(Qt::StrongFocus);host.resize(420,420);host.show();host.activateWindow();
   for(bool legacyWorkbench:{false,true}){
    std::unique_ptr<QPushButton> button(makeButton(legacyWorkbench,"",&host));button->show();
    for(qreal scale:{.9,1.,1.2}){QFont font=baseline;if(font.pointSizeF()>0)font.setPointSizeF(font.pointSizeF()*scale);else font.setPixelSize(qRound(font.pixelSize()*scale));button->setFont(font);settle();
     qInfo().noquote()<<"RENDER"<<styleName<<theme<<(legacyWorkbench?"WorkbenchButton":"ActionButton")<<"font"<<QFontInfo(button->font()).family()<<button->font().pointSizeF()<<"pt"<<"DPR"<<button->devicePixelRatioF();
     for(const auto &caption:{QString("Plot"),QString("Edit expression"),QString("Add curve"),QString("+ Curve appearance"),QString("+ Calculation context"),QString("Scale & copy gypq"),QString("gypjq ÁÉgj — wrapped expression guidance")})for(int width:{130,280})for(const auto &mode:{QString("normal"),QString("focused"),QString("pressed"),QString("disabled"),QString("checked")}){
      button->setText(caption);state(*button,mode,host);const int height=button->heightForWidth(width);button->resize(width,height);const auto compact=captionInk(*button);button->resize(width,height+64);const auto roomy=captionInk(*button);++cases;
      const QString context=QString("%1/%2/%3/%4pt/%5/%6px/%7: %8 vs %9").arg(styleName,theme,legacyWorkbench?"workbench":"action").arg(button->font().pointSizeF()).arg(mode).arg(width).arg(caption,pixels(compact),pixels(roomy));
      check(!compact.bounds.isEmpty()&&compact.bounds.height()>=roomy.bounds.height()&&compact.bounds.width()>=roomy.bounds.width()&&compact.pixels>=roomy.pixels*.97,context);
      if(compact.bounds.size()!=roomy.bounds.size())qWarning()<<"DETAIL"<<"compact height"<<height<<"roomy height"<<height+64;
      // Compare single-line ink with a native QPushButton in the same state:
      // forced Active ButtonText must not override pressed/disabled QSS ink.
      if(width==280&&caption=="Edit expression"){
       // Larger glyphs provide solid foreground pixels: a tiny native LCD
       // antialiased glyph may contain only colored subpixel edge samples.
       QFont colorFont=font;colorFont.setPixelSize(28);button->setFont(colorFont);button->resize(width,button->heightForWidth(width)+64);state(*button,mode,host);const auto colored=captionInk(*button);
       QPushButton reference(caption,&host);reference.setFont(colorFont);reference.resize(button->size());reference.show();state(reference,mode,host);const auto expected=captionInk(reference);
       check(colored.dominant==expected.dominant,context+QString(" state color %1 vs native %2").arg(QColor(colored.dominant).name(),QColor(expected.dominant).name()));button->setFont(font);
      }
     }
    }
    button->setText("Add curve");button->setEnabled(true);button->setCheckable(false);button->setDown(false);button->resize(180,button->heightForWidth(180));button->setFocus();settle();int clicks=0;QObject::connect(button.get(),&QPushButton::clicked,&host,[&]{++clicks;});QTest::keyClick(button.get(),Qt::Key_Space);check(clicks==1&&!button->isDown(),styleName+theme+": Space activates once and releases");QTest::mouseClick(button.get(),Qt::LeftButton);check(clicks==2&&!button->isDown(),styleName+theme+": mouse click activates once and releases");button->setDefault(true);QTest::keyClick(button.get(),Qt::Key_Return);check(clicks==3,styleName+theme+": default Enter activates once");
   }
   // View is a native menu button. Its arrow keeps its native position and is
   // painted once; wrapped captions reserve the trailing indicator width.
   for(auto direction:{Qt::LeftToRight,Qt::RightToLeft}){
    DesktopWorkbench::ActionButton menuButton("View",&host);QMenu menu;menu.addAction("Graph appearance");menuButton.setMenu(&menu);menuButton.setLayoutDirection(direction);menuButton.show();
    for(const auto &caption:{QString("View"),QString("View graph appearance options")})for(const auto &mode:{QString("normal"),QString("focused"),QString("pressed"),QString("disabled")}){
     menuButton.setText(caption);state(menuButton,mode,host);const int height=menuButton.heightForWidth(130);menuButton.resize(130,height);const auto compact=captionInk(menuButton);menuButton.resize(130,height+64);const auto roomy=captionInk(menuButton);
     check(!compact.bounds.isEmpty()&&compact.bounds.height()>=roomy.bounds.height()&&compact.bounds.width()>=roomy.bounds.width()&&compact.pixels>=roomy.pixels*.97,styleName+theme+mode+": native menu caption ink remains complete in either text direction");
    }
    menuButton.setText("");menuButton.setEnabled(true);menuButton.setDown(false);menuButton.clearFocus();host.setFocus();menuButton.resize(180,64);QPushButton reference("",&host);reference.setMenu(&menu);reference.setLayoutDirection(direction);reference.resize(menuButton.size());reference.show();settle();menuButton.setAttribute(Qt::WA_UnderMouse,false);reference.setAttribute(Qt::WA_UnderMouse,false);
    check(menuButton.grab().toImage()==reference.grab().toImage(),styleName+theme+": empty menu button retains one unmodified native arrow");
   }
  }
  if(!output.isEmpty()){
   app.setStyleSheet(stylesheet("Classic"));app.setPalette(calculatorThemePalette("Classic"));QWidget gallery;gallery.setWindowTitle("EMCICalc button caption fixture");auto *layout=new QVBoxLayout(&gallery);auto *title=new QLabel("Native button caption fixture\n"+styleName+" · "+QFontInfo(baseline).family());title->setWordWrap(true);layout->addWidget(title);
   for(const auto &text:{QString("Plot"),QString("Edit expression"),QString("Add curve"),QString("+ Curve appearance"),QString("+ Calculation context"),QString("+ Manage curves"),QString("gypjq ÁÉgj — wrapped expression guidance")})layout->addWidget(DesktopWorkbench::action(text,&gallery));layout->addWidget(new WorkbenchButton("Calculate / Copy result",&gallery));gallery.resize(320,540);gallery.show();settle();const auto path=QDir(output).filePath("buttons-"+styleName.toLower()+".png");check(gallery.grab().save(path),"native component capture "+path);
  }
 }
 app.setStyleSheet({});qInfo()<<"RASTER CASES"<<cases;std::cout<<checks-failures<<'/'<<checks<<" shared button text checks passed\n";return failures?1:0;
}
