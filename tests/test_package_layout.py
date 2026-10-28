#!/usr/bin/env python3
"""Exercise packaging path transformations without claiming ELF/runtime validation."""
import argparse
import importlib.util
from pathlib import Path
import subprocess
import tarfile
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('emcicalc_package', ROOT / 'scripts/package.py')
package = importlib.util.module_from_spec(spec)
spec.loader.exec_module(package)

class PackageLayoutTests(unittest.TestCase):
    def test_native_entries_and_private_debian_resources(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / 'source'
            build = root / 'build'
            plugins = root / 'qt-plugins'
            system = root / 'system'
            output = root / 'output'
            for path in [source, build, plugins, system, output]:
                path.mkdir()
            def write(path, data=b'fixture'):
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(data)
            write(build / 'EMCICalc', b'\x7fELFapplication fixture')
            for name in ['platforms/libqxcb.so', 'platforms/libqoffscreen.so', 'tls/libqopensslbackend.so']:
                write(plugins / name, b'\x7fELFplugin fixture')
            for name in ['libssl.so.3', 'libcrypto.so.3']:
                write(system / name, b'\x7fELFlibrary fixture')
            for name in ['NotoSansCJKjp-Regular.otf', 'NotoSansCJKsc-Regular.otf', 'OFL-NotoSansCJK.txt', 'font-manifest.json']:
                write(source / 'assets/fonts' / name)
            write(source / 'assets/icon.png')
            write(source / 'docs/guide.md')
            write(source / 'LICENSE')
            args = argparse.Namespace(build_dir=build, qt_plugins=plugins,
                compiled_definitions=False, definitions=source / 'definitions',
                dependency_notices=None, stage_only=False)
            write(args.definitions / 'functions.xml')
            calls = []
            def run(command, **kwargs):
                command = list(map(str, command))
                calls.append(command)
                stdout = ''
                if command[0] == 'ldconfig':
                    stdout = '\n'.join(f' {name} (libc6,x86-64) => {system/name}' for name in ['libssl.so.3','libcrypto.so.3'])
                elif command[0] == 'dpkg-deb':
                    write(Path(command[-1]), b'Debian archive fixture')
                return subprocess.CompletedProcess(command, 0, stdout, '')
            with patch.object(package, 'ROOT', source), patch.object(package, 'run', run), \
                 patch.object(package, 'required_glibc', return_value='2.38'), \
                 patch('ctypes.util.find_library', side_effect=lambda name: 'lib'+name+'.so.3'), \
                 patch.object(package.subprocess, 'run', return_value=subprocess.CompletedProcess([], 1, '', '')):
                products = package.linux(args, output)
            stage = output / 'staging/EMCICalc-1.0.0-Linux-x86_64'
            deb = output / 'staging/deb-root'
            self.assertEqual((stage/'EMCICalc').read_bytes(), (build/'EMCICalc').read_bytes())
            self.assertFalse((stage/'EMCICalc').is_symlink())
            self.assertFalse((stage/'bin').exists())
            self.assertIn('Prefix=.\n', (stage/'qt.conf').read_text())
            entry = deb/'usr/bin/emcicalc'
            self.assertTrue(entry.is_file())
            self.assertFalse(entry.is_symlink())
            self.assertEqual(entry.stat().st_mode & 0o111, 0o111)
            self.assertFalse((deb/'usr/bin/qt.conf').exists())
            for relative in ['usr/lib/emcicalc/libssl.so.3', 'usr/lib/emcicalc/plugins/platforms/libqxcb.so',
                             'usr/share/emcicalc/fonts/NotoSansCJKjp-Regular.otf',
                             'usr/share/emcicalc/icon.png', 'usr/share/emcicalc/qalculate/functions.xml',
                             'usr/share/doc/emcicalc/LICENSE',
                             'usr/share/applications/io.github.davoda1.emcicalc.desktop']:
                self.assertTrue((deb/relative).is_file(), relative)
            self.assertIn(['patchelf','--set-rpath','$ORIGIN/lib',str(stage/'EMCICalc')], calls)
            self.assertIn(['patchelf','--set-rpath','$ORIGIN/../lib/emcicalc',str(entry)], calls)
            self.assertIn(['patchelf','--set-rpath','$ORIGIN/../..',str(deb/'usr/lib/emcicalc/plugins/platforms/libqxcb.so')], calls)
            self.assertIn('Version: 1.0.0\n', (deb/'DEBIAN/control').read_text())
            self.assertNotIn('Homepage:', (deb/'DEBIAN/control').read_text())
            with tarfile.open(products[0]) as archive:
                names = archive.getnames()
                self.assertIn('EMCICalc', names)
                self.assertTrue(archive.getmember('EMCICalc').isfile())
                self.assertNotIn('bin/EMCICalc', names)
                self.assertFalse(any(name.endswith(('.sh','.cmd','.bat','.ps1')) for name in names))
            package.update_checksums(output)
            checksums = (output/'SHA256SUM.txt').read_text().splitlines()
            self.assertEqual(len(checksums), 2)
            self.assertEqual(len(list(output.glob('*.sha256'))), 0)

if __name__ == '__main__':
    unittest.main()
