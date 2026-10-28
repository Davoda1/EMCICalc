// SPDX-License-Identifier: MIT
#include "fontscale.h"
#include <cmath>
#include <iostream>
int main(int argc,char **argv){
 QApplication app(argc,argv);DesktopFont::initialize(app);QWidget root;QLabel text("Font",&root);QFont f=text.font();f.setPointSizeF(15);text.setFont(f);
 auto check=[&](bool ok,const char *label){if(!ok){std::cerr<<label<<"\n";return false;}return true;};bool ok=true;
 for(int i=0;i<30;++i){
  DesktopFont::setSizeMode("Large");ok&=check(std::abs(text.font().pointSizeF()-18)<.01,"Large uses nominal font");
  DesktopFont::setSizeMode("Small");ok&=check(std::abs(text.font().pointSizeF()-13.5)<.01,"Small uses nominal font");
  DesktopFont::setSizeMode("System");ok&=check(std::abs(text.font().pointSizeF()-15)<.01,"System restores nominal font");
 }
 DesktopFont::setSizeMode("fixed");ok&=check(DesktopFont::sizeMode()=="Medium","Legacy fixed migrates Medium");
 QLabel dynamic("New hidden page",&root);DesktopFont::setNominal(&dynamic,20);DesktopFont::setSizeMode("Large");DesktopFont::apply(&root);const auto first=dynamic.font();DesktopFont::apply(&root);ok&=check(first==dynamic.font(),"Repeated apply is idempotent");
 DesktopFont::setSizeMode("System");ok&=check(DesktopFont::sizeMode()=="System","System selection");
 std::cout<<(ok?"desktop font scaling passed\n":"desktop font scaling FAILED\n");return ok?0:1;
}
