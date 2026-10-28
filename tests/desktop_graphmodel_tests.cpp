// SPDX-License-Identifier: MIT
#include "graphmodel.h"
#include <QCoreApplication>
#include <QThread>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace EmciGraph;
static void check(bool condition,const char *message){if(!condition)throw std::runtime_error(message);}
static Series fixture(Kind kind){Series s;s.id="fixture";s.name="Fixture";s.kind=kind;s.expression="fixture";return s;}
int main(int argc,char **argv){QCoreApplication app(argc,argv);try{
    Request request;request.bounds={1e9,1e9+1,-1,1};request.series={fixture(Kind::Surface3D)};
    const auto surface=sample(request,[](const QString&,const QMap<QString,double>&v,int){return ScalarResult{true,1e9+(v.value("x")-1e9)+v.value("y"),{}};});
    check(surface.status==Status::Complete&&!surface.series[0].triangles.isEmpty(),"large-offset surface missing");
    bool distinct=false;
    for(const auto&t:surface.series[0].triangles){
        distinct|=t.a.x()!=t.b.x();
        for(auto p:{t.a,t.b,t.c})check(std::abs(p.z()-(1e9+(p.x()-1e9)+p.y()))<1e-6,"surface world-coordinate precision lost");
    }
    check(distinct,"large-offset surface collapsed to a line");

    request.bounds={-1.13,1.41,-1,1};request.series={fixture(Kind::LessEqual)};
    const auto inequality=sample(request,[](const QString&,const QMap<QString,double>&v,int){const double x=v.value("x");return ScalarResult{x!=0,1/x,{}};});
    check(!inequality.series[0].regions.isEmpty(),"valid inequality half-plane lost");
    for(const auto&region:inequality.series[0].regions)for(auto p:region)check(p.x()<0,"inequality fills across reciprocal pole");

    request={};request.series={fixture(Kind::Explicit)};request.series[0].enabled=false;
    request.tableSeries="fixture";request.tableInputs={0,1,2,3,4,5,6};request.maximumMilliseconds=100;
    int evaluations=0;
    const auto timed=sample(request,[&](const QString&,const QMap<QString,double>&v,int){++evaluations;QThread::msleep(35);return ScalarResult{true,v.value("x"),{}};});
    check(timed.status==Status::Limited,"table timeout reported as complete");
    check(timed.table.size()==request.tableInputs.size(),"timeout dropped requested table rows");
    check(evaluations>=1&&evaluations<request.tableInputs.size(),"timeout failed to stop evaluations");
    for(int i=0;i<timed.table.size();++i){
        check(timed.table[i].input==request.tableInputs[i],"timeout changed table abscissae");
        if(i>=evaluations)check(!timed.table[i].valid&&timed.table[i].error.contains("time limit"),"uncomputed row lacks timeout status");
    }
    check(tableCsv(timed).count('\n')==request.tableInputs.size()+1,"CSV omitted timeout rows");

    auto cancelled=std::make_shared<std::atomic_bool>(false);evaluations=0;
    const auto stopped=sample(request,[&](const QString&,const QMap<QString,double>&v,int){++evaluations;cancelled->store(true);return ScalarResult{true,v.value("x"),{}};},cancelled);
    check(stopped.status==Status::Cancelled&&evaluations==1,"cancelled table continued sampling or accepted a result");
    std::cout<<"PASS desktop graph regressions: double surface coordinates, pole-safe regions, complete timeout CSV, cancellation.\n";
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}return 0;}
