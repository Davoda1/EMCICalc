#!/usr/bin/env python3
"""Create native EMCICalc runtime packages. Python 3.9+, no third-party modules."""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tarfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
VERSION = '1.0.0'
RELEASE = VERSION
DEB_VERSION = VERSION
SYSTEM_DLLS = set('advapi32 bcrypt bcryptprimitives cfgmgr32 comctl32 comdlg32 crypt32 cryptbase d2d1 d3d9 d3d11 d3d12 dcomp dbghelp dnsapi dwmapi dwrite dxgi gdi32 glu32 imm32 iphlpapi kernel32 kernelbase mpr msvcrt netapi32 ntdll ole32 oleaut32 opengl32 powrprof propsys psapi rpcrt4 secur32 setupapi shell32 shlwapi user32 userenv usp10 uxtheme version winhttp wininet winmm winscard winspool wintrust wldap32 ws2_32 wtsapi32 ucrtbase normaliz shcore synchronization gdi32full combase msimg32 avrt authz cryptnet hid magnification mf mfplat mfreadwrite mfuuid mmdevapi ncrypt netutils opengl32 pdh profapi shcore sspicli urlmon win32u windowscodecs wlanapi'.split())
HOST_ELF = re.compile(r'^(?:ld-linux|lib(?:c|m|pthread|dl|rt|util|resolv|nss_[^.]+)\.so|lib(?:GL|GLX|GLdispatch|EGL|OpenGL|drm|gbm|vulkan|cuda|nvidia))')

def run(args, **kw):
    return subprocess.run([str(a) for a in args], check=True, text=True, **kw)

def copy(src: Path, dst: Path):
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)

def executable(build: Path, windows=False):
    name = 'EMCICalc.exe' if windows else 'EMCICalc'
    for p in [build/name, build/'Release'/name, build/'bin'/name, build/'bin'/'Release'/name]:
        if p.is_file(): return p
    raise RuntimeError(f'Built executable {name} not found in {build}')

def documentation(stage):
    for name in ('LICENSE', 'README.md', 'README.html', 'THIRD-PARTY.md'):
        p = ROOT/name
        if p.exists(): copy(p, stage/name)
    shutil.copytree(ROOT/'docs', stage/'docs', dirs_exist_ok=True)
    third_party=ROOT/'third_party'
    if third_party.exists():
        for metadata in third_party.iterdir():
            if metadata.is_file() and metadata.suffix in ('.md','.json'):
                copy(metadata,stage/'docs'/'source-information'/metadata.name)
        for name in ('manifest.json','SHA256SUMS.txt'):
            if (third_party/'sources'/name).exists():
                copy(third_party/'sources'/name,stage/'docs'/'source-information'/name)
        if (third_party/'licenses').exists():
            shutil.copytree(third_party/'licenses',stage/'docs'/'licenses'/'third-party',dirs_exist_ok=True)

def application_fonts(stage, windows=False):
    source=ROOT/'assets'/'fonts'
    required=('NotoSansCJKjp-Regular.otf','NotoSansCJKsc-Regular.otf','OFL-NotoSansCJK.txt','font-manifest.json')
    if any(not (source/name).is_file() for name in required):
        raise RuntimeError('Bundled CJK fallback fonts or license metadata are missing')
    target=stage/'fonts' if windows else stage/'share'/'emcicalc'/'fonts'
    shutil.copytree(source,target,dirs_exist_ok=True)

def definitions(stage, candidates):
    source = next((p for p in candidates if p and (p/'functions.xml').is_file()), None)
    if source is None:
        raise RuntimeError('Qalculate definitions missing; pass --definitions DIRECTORY')
    target=stage/'share'/'qalculate'
    target.mkdir(parents=True, exist_ok=True)
    # Fixed mathematical definitions only. Per-user downloaded exchange-rate caches are not packaged.
    for name in ('prefixes.xml','currencies.xml','units.xml','functions.xml','variables.xml'):
        if (source/name).exists(): copy(source/name,target/name)

def checksum(path):
    h=hashlib.sha256()
    with path.open('rb') as f:
        for part in iter(lambda:f.read(1024*1024),b''): h.update(part)
    return h.hexdigest()

def required_glibc(files):
    """Highest actual versioned GLIBC import in application plus bundled ELFs."""
    versions=set()
    for file in files:
        result=run(['readelf','--version-info',file],capture_output=True).stdout
        versions.update(re.findall(r'\bGLIBC_(\d+(?:\.\d+)+)\b',result))
    if not versions:
        raise RuntimeError('Cannot determine the actual glibc baseline')
    return max(versions,key=lambda version:tuple(map(int,version.split('.'))))

def linux(args, output):
    stage=output/'staging'/f'EMCICalc-{RELEASE}-Linux-x86_64'
    if stage.exists(): shutil.rmtree(stage)
    stage.mkdir(parents=True)
    binary=stage/'EMCICalc'
    copy(executable(args.build_dir),binary)
    binary.chmod(0o755)
    if args.qt_plugins:
        plugin_root=args.qt_plugins.resolve()
    else:
        qpaths=shutil.which('qtpaths6') or '/usr/lib/qt6/bin/qtpaths'
        plugin_root=Path(run([qpaths,'--query','QT_INSTALL_PLUGINS'],capture_output=True).stdout.strip())
    for category,names in {
        'platforms':['libqxcb.so','libqoffscreen.so'],
        'xcbglintegrations':['libqxcb-glx-integration.so','libqxcb-egl-integration.so'],
        'tls':['libqopensslbackend.so'],
    }.items():
        for name in names:
            source=plugin_root/category/name
            if source.exists(): copy(source,stage/'plugins'/category/name)
    if not (stage/'plugins'/'platforms'/'libqxcb.so').exists():
        raise RuntimeError('Qt xcb platform plugin is required')
    if not (stage/'plugins'/'tls'/'libqopensslbackend.so').is_file():
        raise RuntimeError('Qt OpenSSL TLS plugin is required for currency rate downloads')
    (stage/'qt.conf').write_text('[Paths]\nPrefix=.\nPlugins=plugins\nLibraries=lib\n',encoding='utf-8')
    queue=[binary,*sorted((stage/'plugins').rglob('*.so'))]
    seen=set()
    provenance={}
    # Qt's OpenSSL backend loads these at runtime, so ldd alone cannot find them.
    import ctypes.util
    for name in ('ssl', 'crypto'):
        soname=ctypes.util.find_library(name)
        candidates=[Path(p)/soname for p in os.environ.get('LD_LIBRARY_PATH','').split(':') if p and soname]
        if soname:
            cache=run(['ldconfig','-p'],capture_output=True).stdout
            candidates.extend(Path(p) for p in re.findall(r'\s'+re.escape(soname)+r'\s+.*?=>\s+(\S+)',cache))
        source=next((p for p in candidates if p.is_file()),None)
        if source is None: raise RuntimeError(f'OpenSSL runtime library {name} was not found')
        target=stage/'lib'/soname; copy(source,target);seen.add(soname)
        provenance[soname]=str(source.resolve());queue.append(target)
    while queue:
        current=queue.pop()
        result=run(['ldd',current],capture_output=True).stdout
        if 'not found' in result: raise RuntimeError(f'Missing ELF dependency for {current}:\n{result}')
        for line in result.splitlines():
            match=re.match(r'\s*(\S+)\s+=>\s+(/\S+)',line)
            if not match: continue
            name,source=match.groups()
            if HOST_ELF.match(name) or name in seen: continue
            seen.add(name)
            dst=stage/'lib'/name
            copy(Path(source),dst)
            provenance[name]=str(Path(source).resolve())
            queue.append(dst)
    # Every binary resolves relocatably, including plugins loaded by absolute path.
    for file in [binary,*sorted((stage/'lib').iterdir()),*sorted((stage/'plugins').rglob('*.so'))]:
        rpath='$ORIGIN/lib' if file==binary else ('$ORIGIN' if file.parent==stage/'lib' else '$ORIGIN/../../lib')
        run(['patchelf','--set-rpath',rpath,file],capture_output=True)
    if not args.compiled_definitions:
        definitions(stage,[args.definitions,Path('/usr/share/qalculate')])
    documentation(stage)
    application_fonts(stage)
    license_dir=stage/'docs'/'dependency-notices'
    for source in sorted(set(provenance.values())):
        query=subprocess.run(['dpkg-query','-S',source],text=True,capture_output=True)
        if query.returncode: continue
        for line in query.stdout.splitlines():
            package=line.split(': ',1)[0].split(':')[0]
            notice=Path('/usr/share/doc')/package/'copyright'
            if notice.is_file(): copy(notice,license_dir/(package+'.copyright'))
    # Local, extracted build dependencies have no entry in the host dpkg database.
    # Accept the matching runtime's notices explicitly for those builds.
    if args.dependency_notices:
        if not args.dependency_notices.is_dir():
            raise RuntimeError('The dependency notice directory does not exist')
        shutil.copytree(args.dependency_notices,license_dir,dirs_exist_ok=True)
    (stage/'docs'/'runtime-manifest.json').write_text(json.dumps({'version':VERSION,'edition':'Desktop','platform':'linux-x86_64','glibcBaseline':required_glibc([binary,*sorted((stage/'lib').iterdir()),*sorted((stage/'plugins').rglob('*.so'))]),'libraries':{name:{'sha256':checksum(stage/'lib'/name),'sourceLibrary':Path(source).name} for name,source in provenance.items()},'plugins':{str(p.relative_to(stage)):{'sha256':checksum(p)} for p in sorted((stage/'plugins').rglob('*.so'))}},indent=2)+'\n',encoding='utf-8')
    glibc_floor=required_glibc([binary,*sorted((stage/'lib').iterdir()),*sorted((stage/'plugins').rglob('*.so'))])
    desktop='[Desktop Entry]\nType=Application\nName=EMCICalc\nComment=Scientific and graphing calculator\nExec=emcicalc\nIcon=emcicalc\nTerminal=false\nCategories=Utility;Calculator;Science;Math;\nStartupWMClass=EMCICalc\n'
    copy(ROOT/'assets'/'icon.png',stage/'share'/'icons'/'emcicalc.png')
    copy(ROOT/'assets'/'icon.png',stage/'share'/'emcicalc'/'icon.png')
    archive=output/f'EMCICalc-{RELEASE}-Linux-x86_64.tar.gz'
    if not args.stage_only:
        with tarfile.open(archive,'w:gz') as tar:
            for item in sorted(stage.iterdir()):
                tar.add(item,arcname=item.name)
    deb=output/'staging'/'deb-root'
    if deb.exists(): shutil.rmtree(deb)
    # A real executable owns /usr/bin; private libraries never enter the global lib namespace.
    entry=deb/'usr'/'bin'/'emcicalc'
    copy(binary,entry)
    run(['patchelf','--set-rpath','$ORIGIN/../lib/emcicalc',entry],capture_output=True)
    private_lib=deb/'usr'/'lib'/'emcicalc'
    shutil.copytree(stage/'lib',private_lib)
    shutil.copytree(stage/'plugins',private_lib/'plugins')
    for plugin in (private_lib/'plugins').rglob('*.so'):
        run(['patchelf','--set-rpath','$ORIGIN/../..',plugin],capture_output=True)
    shutil.copytree(stage/'share'/'emcicalc',deb/'usr'/'share'/'emcicalc')
    if (stage/'share'/'qalculate').exists():
        shutil.copytree(stage/'share'/'qalculate',deb/'usr'/'share'/'emcicalc'/'qalculate')
    docdir=deb/'usr'/'share'/'doc'/'emcicalc'
    shutil.copytree(stage/'docs',docdir)
    for name in ('LICENSE','README.md','THIRD-PARTY.md'):
        if (stage/name).exists(): copy(stage/name,docdir/name)
    (deb/'usr'/'share'/'applications').mkdir(parents=True)
    (deb/'usr'/'share'/'applications'/'io.github.davoda1.emcicalc.desktop').write_text(desktop,encoding='utf-8')
    copy(ROOT/'assets'/'icon.png',deb/'usr'/'share'/'icons'/'hicolor'/'256x256'/'apps'/'emcicalc.png')
    control=deb/'DEBIAN'; control.mkdir()
    total=sum(p.stat().st_size for p in (deb/'usr').rglob('*') if p.is_file())//1024
    (control/'control').write_text(f'Package: emcicalc\nVersion: {DEB_VERSION}\nSection: science\nPriority: optional\nArchitecture: amd64\nMaintainer: davoda1\nInstalled-Size: {total}\nDepends: libc6 (>= {glibc_floor}), libgl1, libegl1, libopengl0, libdrm2, fontconfig, ca-certificates\nDescription: EMCICalc scientific and graphing calculator\n Native Qt scientific calculator with libqalculate and interactive graphs.\n',encoding='utf-8')
    debpath=output/f'emcicalc_{DEB_VERSION}_amd64.deb'
    if args.stage_only:
        return [stage,deb]
    run(['dpkg-deb','--root-owner-group','--build',deb,debpath])
    return [archive,debpath]

def pe_imports(path,objdump):
    output=run([objdump,'-p',path],capture_output=True).stdout
    return re.findall(r'DLL Name:\s*(\S+)',output,re.I)

def windows(args,output):
    if not args.sysroot: raise RuntimeError('--sysroot is required for Windows runtime packaging')
    prefix=args.sysroot
    stage=output/'staging'/f'EMCICalc-{RELEASE}-Windows-x64'
    if stage.exists(): shutil.rmtree(stage)
    stage.mkdir(parents=True)
    exe=stage/'EMCICalc.exe'; copy(executable(args.build_dir,True),exe)
    copy(ROOT/'assets'/'icon.png',stage/'icon.png')
    objdump=args.objdump or shutil.which('x86_64-w64-mingw32-objdump') or shutil.which('objdump')
    if not objdump: raise RuntimeError('objdump is required for recursive DLL inspection')
    plugin_root=next((p for p in [prefix/'share'/'qt6'/'plugins',prefix/'lib'/'qt6'/'plugins',prefix/'plugins'] if (p/'platforms'/'qwindows.dll').exists()),None)
    if not plugin_root: raise RuntimeError('Qt qwindows.dll missing from sysroot')
    for category,names in {'platforms':['qwindows.dll'],'styles':['qmodernwindowsstyle.dll','qwindowsvistastyle.dll'],'tls':['qschannelbackend.dll']}.items():
        for name in names:
            if (plugin_root/category/name).exists(): copy(plugin_root/category/name,stage/category/name)
    if not (stage/'tls'/'qschannelbackend.dll').is_file():
        raise RuntimeError('Qt Schannel TLS plugin is required for currency rate downloads')
    (stage/'qt.conf').write_text('[Paths]\nPrefix=.\nPlugins=.\n',encoding='utf-8')
    lookup={p.name.lower():p for p in (prefix/'bin').glob('*') if p.suffix.lower()=='.dll'}
    lookup.update({p.name.lower():p for p in args.build_dir.glob('*.dll')})
    queue=[exe,*stage.rglob('*.dll')]; seen=set(); manifest={}
    while queue:
        for dependency in pe_imports(queue.pop(),objdump):
            name=dependency.lower()
            if name in seen: continue
            seen.add(name)
            if name.startswith(('api-ms-win-','ext-ms-win-')) or name.removesuffix('.dll') in SYSTEM_DLLS: continue
            source=lookup.get(name)
            if source is None: raise RuntimeError(f'Unresolved Windows runtime dependency: {dependency}')
            target=stage/source.name; copy(source,target); queue.append(target)
            manifest[source.name]={'sha256':checksum(target),'source':str(source.relative_to(prefix)) if source.is_relative_to(prefix) else source.name}
    if not args.compiled_definitions:
        definitions(stage,[args.definitions,prefix/'share'/'qalculate'])
    documentation(stage)
    application_fonts(stage,windows=True)
    licenses=prefix/'share'/'licenses'
    if licenses.exists(): shutil.copytree(licenses,stage/'docs'/'dependency-notices',dirs_exist_ok=True)
    (stage/'docs'/'runtime-manifest.json').write_text(json.dumps({'version':VERSION,'edition':'Desktop','platform':'windows-x64','libraries':manifest,'plugins':{str(p.relative_to(stage)):{'sha256':checksum(p)} for p in sorted(stage.rglob('*.dll')) if p.parent!=stage}},indent=2)+'\n',encoding='utf-8')
    # DLL presence is insufficient: every named/ordinal import must be exported
    # by the actual bundled dependency, including forwarded and delayed imports.
    audit_script=ROOT/'scripts'/'audit_pe.py'
    if not audit_script.is_file(): raise RuntimeError('Required Windows symbol-audit helper is missing')
    run([sys.executable,audit_script,stage,'--json',stage/'docs'/'pe-symbol-audit.json'])
    if args.stage_only:
        return [stage]
    archive=output/f'EMCICalc-{RELEASE}-Windows-x64.zip'
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for file in sorted(stage.rglob('*')):
            if file.is_file(): z.write(file,file.relative_to(stage))
    return [archive]

def update_checksums(output):
    products=sorted(p for p in output.iterdir() if p.is_file() and
                    (p.name.endswith(('.zip','.tar.gz','.deb'))))
    (output/'SHA256SUM.txt').write_text(''.join(f'{checksum(p)}  {p.name}\n' for p in products),encoding='utf-8')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--platform',choices=['windows','linux'],required=True)
    p.add_argument('--stage-only',action='store_true',help='Prepare audited runtime/debian filesystem trees without writing release archives')
    p.add_argument('--build-dir',type=Path,default=ROOT/'build')
    p.add_argument('--output',type=Path,default=ROOT/'dist')
    p.add_argument('--sysroot',type=Path)
    p.add_argument('--definitions',type=Path)
    p.add_argument('--objdump')
    p.add_argument('--qt-plugins',type=Path,help='Exact native Qt plugin directory (for extracted dependency prefixes)')
    p.add_argument('--dependency-notices',type=Path,help='Additional Linux dependency notices for locally extracted runtime libraries')
    p.add_argument('--compiled-definitions',action='store_true',help='The libqalculate build embeds its definition data; omit external XML files')
    args=p.parse_args()
    args.build_dir=args.build_dir.resolve(); output=args.output.resolve(); output.mkdir(parents=True,exist_ok=True)
    products=linux(args,output) if args.platform=='linux' else windows(args,output)
    for file in products: print(file)
    if not args.stage_only:
        update_checksums(output)

if __name__=='__main__':
    try: main()
    except (OSError,RuntimeError,subprocess.CalledProcessError) as e:
        print(f'Packaging failed: {e}',file=sys.stderr); sys.exit(1)
