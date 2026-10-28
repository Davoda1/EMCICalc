#!/usr/bin/env python3
"""Audit an extracted native EMCICalc Linux runtime; no source/runtime launcher."""
from __future__ import annotations
import argparse, hashlib, json, os, pathlib, re, shutil, subprocess, tempfile, time

def call(args, env=None):
    r=subprocess.run([str(x) for x in args],text=True,capture_output=True,env=env)
    if r.returncode:raise RuntimeError(f'{args[0]} failed ({r.returncode}): {r.stderr.strip()}')
    return r.stdout

def audit(stage: pathlib.Path, launch: bool=False, deb=None, install_root=None):
    stage=stage.resolve();binary=stage/'EMCICalc'
    env=os.environ.copy()
    for key in ['LD_LIBRARY_PATH','QT_PLUGIN_PATH','QT_QPA_PLATFORM_PLUGIN_PATH','QML2_IMPORT_PATH','QT_QPA_FONTDIR']:
        env.pop(key,None)
    env['QT_QPA_PLATFORM']='offscreen';env['EMCICALC_EMBEDDED_DEFINITIONS']='1'
    if binary.is_symlink() or not binary.is_file() or binary.read_bytes()[:4]!=b'\x7fELF':raise RuntimeError('EMCICalc must be a real native ELF binary at the archive root')
    if not os.access(binary,os.X_OK):raise RuntimeError('Native entry point is not executable')
    elfs=[];versions=set();errors=[];rpaths={};dependencies={}
    for p in stage.rglob('*'):
        if not p.is_file():continue
        with p.open('rb') as f:magic=f.read(4)
        if magic!=b'\x7fELF':continue
        elfs.append(p);relative=str(p.relative_to(stage))
        dynamic=call(['readelf','-d',p]);matches=re.findall(r'\((?:RPATH|RUNPATH)\).*?\[(.*?)\]',dynamic)
        rpaths[relative]=matches
        if any('/workspace/' in s or '/dev/shm/' in s for s in matches):errors.append(f'Build-path RPATH: {relative}')
        deps=call(['ldd',p],env=env);dependencies[relative]=deps
        if 'not found' in deps:errors.append(f'Missing dependency: {relative}')
        for name,path in re.findall(r'\s*(\S+)\s+=>\s+(/\S+)',deps):
            if ('/workspace/' in path or '/dev/shm/' in path) and not pathlib.Path(path).resolve().is_relative_to(stage):
                errors.append(f'External build dependency: {relative} -> {path}')
        versions.update(re.findall(r'\bGLIBC_(\d+(?:\.\d+)+)\b',call(['readelf','--version-info',p])))
    forbidden=[str(p.relative_to(stage)) for p in stage.rglob('*') if p.is_file() and p.suffix.lower() in ('.sh','.cmd','.ps1','.bat')]
    if forbidden:errors.append(f'Unexpected scripts in runtime package: {forbidden}')
    report={'version':'1.0.0','edition':'Desktop','nativeEntryPoint':True,'elfFilesChecked':len(elfs),'highestRequiredGlibcVersion':max(versions,key=lambda v:tuple(map(int,v.split('.')))) if versions else None,'entrySHA256':hashlib.sha256(binary.read_bytes()).hexdigest(),'rpaths':rpaths,'dependencies':dependencies,'errors':errors,'runtimeLaunch':{'performed':False}}
    if launch:
        with tempfile.TemporaryDirectory(prefix='emcicalc-package-qa-') as tmp:
            runtimeenv=dict(env,XDG_CONFIG_HOME=tmp,QT_LOGGING_RULES='qt.qpa.*=false')
            log=pathlib.Path(tmp)/'startup.log';start=time.monotonic()
            with log.open('wb') as stream:
                process=subprocess.Popen([str(binary)],env=runtimeenv,stdout=stream,stderr=subprocess.STDOUT)
                try:process.wait(timeout=3);alive=False
                except subprocess.TimeoutExpired:alive=True;process.terminate();process.wait(timeout=15)
            report['runtimeLaunch']={'performed':True,'platform':'Linux Qt offscreen','freshProfile':True,'aliveAfterThreeSeconds':alive,'exitCodeAfterBoundedTermination':process.returncode,'elapsedSeconds':round(time.monotonic()-start,3),'log':log.read_text(errors='replace'),'limitation':'Native process and Qt rendering backend exercised; this does not claim an X11/Wayland clean-machine session.'}
            if not alive:errors.append('Native startup exited before bounded observation completed')
    if deb:
        if not install_root:raise RuntimeError('--deb requires --install-root for isolated installation/removal')
        target=pathlib.Path(install_root).resolve();target.mkdir(parents=True,exist_ok=True)
        db=target/'var/lib/dpkg';db.mkdir(parents=True,exist_ok=True);(db/'status').touch()
        if (target/'opt').exists() and any((target/'opt').iterdir()):raise RuntimeError('Installation root must not already contain /opt packages')
        lifecycle={};arguments=['dpkg',f'--root={target}','--force-depends','--force-not-root']
        lifecycle['installLog']=call(arguments+['--install',pathlib.Path(deb).resolve()])
        entry=target/'usr/bin/emcicalc'
        lifecycle['symlinkTarget']=os.readlink(entry) if entry.is_symlink() else None
        native=entry
        lifecycle['nativeTargetExecutable']=not native.is_symlink() and native.is_file() and os.access(native,os.X_OK) and native.read_bytes()[:4]==b'\x7fELF'
        lifecycle['nativeTargetSHA256']=hashlib.sha256(native.read_bytes()).hexdigest() if native.is_file() else None
        # Debian's RPATH differs deliberately from the portable binary.
        lifecycle['nativeTargetRpath']=re.findall(r'\((?:RPATH|RUNPATH)\).*?\[(.*?)\]',call(['readelf','-d',native]))
        lifecycle['nativeTargetRpathValid']=lifecycle['nativeTargetRpath']==['$ORIGIN/../lib/emcicalc']
        if launch and lifecycle['nativeTargetExecutable']:
            with tempfile.TemporaryDirectory(prefix='emcicalc-deb-qa-') as tmp:
                runtimeenv=dict(env,XDG_CONFIG_HOME=tmp,QT_LOGGING_RULES='qt.qpa.*=false')
                log=pathlib.Path(tmp)/'startup.log'
                with log.open('wb') as stream:
                    process=subprocess.Popen([str(native)],env=runtimeenv,stdout=stream,stderr=subprocess.STDOUT)
                    try:process.wait(timeout=3);alive=False
                    except subprocess.TimeoutExpired:alive=True;process.terminate();process.wait(timeout=15)
                lifecycle['installedNativeLaunch']={'platform':'Linux Qt offscreen','freshProfile':True,'aliveAfterThreeSeconds':alive,'exitCodeAfterBoundedTermination':process.returncode,'log':log.read_text(errors='replace')}
                if not alive:errors.append('Installed Debian native target exited before bounded observation completed')
        lifecycle['removeLog']=call(arguments+['--purge','emcicalc'])
        lifecycle['entryRemoved']=not entry.exists() and not entry.is_symlink()
        lifecycle['scope']='dpkg install/purge in an isolated filesystem root, with dependency checks bypassed because host OS libraries are outside that root; no clean-machine dependency claim.'
        report['debianLifecycle']=lifecycle
        if lifecycle['symlinkTarget'] is not None or not lifecycle['nativeTargetExecutable'] or not lifecycle['nativeTargetRpathValid'] or not lifecycle['entryRemoved']:errors.append('Debian lifecycle verification failed')
    report['passed']=not errors
    return report

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('stage',type=pathlib.Path);p.add_argument('--json',type=pathlib.Path,required=True);p.add_argument('--launch',action='store_true');p.add_argument('--deb',type=pathlib.Path);p.add_argument('--install-root',type=pathlib.Path);a=p.parse_args()
    report=audit(a.stage,a.launch,a.deb,a.install_root);a.json.parent.mkdir(parents=True,exist_ok=True);a.json.write_text(json.dumps(report,indent=2)+'\n')
    print(f"Linux package audit: {'PASS' if report['passed'] else 'FAIL'}; {report['elfFilesChecked']} ELF files; glibc {report['highestRequiredGlibcVersion']}; {len(report['errors'])} errors")
    for e in report['errors']:print(e)
    return 0 if report['passed'] else 1
if __name__=='__main__':raise SystemExit(main())
