#!/usr/bin/env python3
"""Run bounded desktop source checks without installing a native toolchain."""
from __future__ import annotations

import ast
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / '.ci-results' / 'source-checks.json'
SECRET = re.compile(rb'(?:github_pat_[A-Za-z0-9_]{25,}|gh[pousr]_[A-Za-z0-9]{25,}|-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----)')
PRIVATE_PATH = re.compile(r'(?<![\w:/])(?:/(?:home|Users)/[A-Za-z0-9_.-]+/|/root/[A-Za-z0-9_.-]+/|/workspace/(?:scratch|projects)/|[A-Za-z]:\\Users\\[^\\\s"\x27]+\\)')
BLOCKED_SUFFIXES = {'.jks', '.keystore', '.p12', '.pfx', '.pem', '.pyc', '.apk', '.exe', '.dll', '.so'}


def source_paths() -> list[Path]:
    result = subprocess.run(
        ['git', 'ls-files', '--cached', '--others', '--exclude-standard', '-z'],
        cwd=ROOT, check=True, capture_output=True, timeout=30,
    )
    return [ROOT / name.decode('utf-8') for name in sorted(set(result.stdout.split(b'\0'))) if name]


def source_guard(paths: list[Path]) -> dict:
    """Inspect actual Git inputs, keeping upstream license attribution intact."""
    errors = []
    first_party_text = 0
    for path in paths:
        relative = path.relative_to(ROOT).as_posix()
        if relative.startswith('.ci-results/'):
            continue
        if path.is_symlink():
            errors.append(f'{relative}: tracked source symlinks are not permitted')
            continue
        if not path.is_file():
            errors.append(f'{relative}: source file is missing')
            continue
        if path.suffix.lower() in BLOCKED_SUFFIXES or path.name == '.env':
            errors.append(f'{relative}: build output, private signing file or local environment file')
        if path.stat().st_size > 100 * 1024 * 1024:
            errors.append(f'{relative}: exceeds GitHub normal file limit; use verified retrieval')
        data = path.read_bytes()
        if SECRET.search(data):
            errors.append(f'{relative}: possible embedded credential/private key (content suppressed)')
        try:
            text = data.decode('utf-8')
        except UnicodeDecodeError:
            continue
        # Upstream sources/notices keep their legitimate names and contact fields.
        # First-party dependency inventories remain in scope even under third_party.
        upstream = relative.startswith(('third_party/licenses/', 'third_party/sources/', 'assets/fonts/'))
        if not upstream:
            first_party_text += 1
            if PRIVATE_PATH.search(text):
                errors.append(f'{relative}: embedded private home/build path (content suppressed)')
            if re.search(r'(?i)(?:' + 'mail' + r'to:|^Homepage\s*:)', text, re.M):
                errors.append(f'{relative}: first-party personal contact/homepage field')
        if relative.startswith('.github/workflows/'):
            for action in re.findall(r'^\s*-?\s*uses:\s*([^\s#]+)', text, re.M):
                if not action.startswith('./') and not re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_./-]+@[0-9a-f]{40}', action):
                    errors.append(f'{relative}: external actions must be pinned to full commit SHAs')
    if errors:
        raise RuntimeError('\n'.join(errors))
    return {'git_inputs': len(paths), 'first_party_text_files': first_party_text}


def release_metadata() -> dict:
    cmake = (ROOT / 'CMakeLists.txt').read_text(encoding='utf-8')
    found = re.search(r'project\(EMCICalc\s+VERSION\s+(\d+\.\d+\.\d+)\s', cmake)
    if not found:
        raise RuntimeError('CMake product version is absent or not three-component semantic version')
    version = found.group(1)
    main = (ROOT / 'src/main.cpp').read_text(encoding='utf-8')
    resource = (ROOT / 'assets/windows.rc').read_text(encoding='utf-8')
    package = (ROOT / 'scripts/package.py').read_text(encoding='utf-8')
    window = (ROOT / 'src/mainwindow.cpp').read_text(encoding='utf-8')
    expected = {
        'application version': f'setApplicationVersion("{version}")' in main,
        'Windows file version': f'FILEVERSION {version.replace(".", ",")},0' in resource,
        'Windows product version': f'PRODUCTVERSION {version.replace(".", ",")},0' in resource,
        'package release': re.search(r'^VERSION\s*=\s*[\x27\"]' + re.escape(version) + r'[\x27\"]', package, re.M) is not None,
        'desktop identity': 'setDesktopFileName("io.github.davoda1.emcicalc")' in main,
        'storage namespace': 'setOrganizationName("davoda1")' in main,
        'profile isolation': 'filePath("davoda1/EMCICalc")' in window and '{"profileFormat",1}' in window,
        'first-party publisher': 'VALUE "CompanyName", "davoda1\\0"' in resource,
    }
    failures = [name for name, matches in expected.items() if not matches]
    if failures:
        raise RuntimeError('Release metadata mismatch: ' + ', '.join(failures))
    inputs = json.loads((ROOT / 'third_party/libqalculate-build.json').read_text())
    archive = ROOT / 'third_party/sources' / ('libqalculate-' + inputs['version'] + '.tar.gz')
    with archive.open('rb') as stream:
        if hashlib.file_digest(stream, 'sha256').hexdigest() != inputs['source_sha256']:
            raise RuntimeError('Included libqalculate source SHA-256 mismatch')
    return {'version': version, 'engine_version': inputs['version'], 'engine_source_verified': True}


def main() -> int:
    REPORT.parent.mkdir(exist_ok=True)
    checks = []
    started = time.monotonic()
    try:
        paths = source_paths()
        checks.append({'name': 'source-privacy-and-action-pins', 'result': source_guard(paths)})
        checks.append({'name': 'versions-and-engine-integrity', 'result': release_metadata()})
        python_count = shell_count = 0
        for path in paths:
            relative = path.relative_to(ROOT)
            if relative.parts[0] == 'third_party':
                continue
            if path.suffix == '.py':
                ast.parse(path.read_text(encoding='utf-8'), filename=relative.as_posix())
                python_count += 1
            elif path.suffix == '.sh':
                subprocess.run(['bash', '-n', str(path)], check=True, timeout=20)
                shell_count += 1
        checks.append({'name': 'syntax', 'python_files': python_count, 'shell_files': shell_count})
        env = os.environ.copy()
        env['PYTHONDONTWRITEBYTECODE'] = '1'
        batches = [
            ('PE audit fixtures', ['tests/test_pe_audit.py'], 60),
            ('runtime packaging layout fixtures', ['tests/test_package_layout.py'], 60),
            ('source release fixtures', ['tests/test_release_source.py'], 120),
            ('localization', ['tests/localization_check.py'], 180),
            ('available corresponding-source hashes', ['scripts/fetch_dependency_sources.py', '--verify-local'], 120),
        ]
        for name, args, seconds in batches:
            print(f'Checking {name}', flush=True)
            subprocess.run([sys.executable, *args], cwd=ROOT, env=env, check=True, timeout=seconds)
            checks.append({'name': name, 'passed': True})
    except (OSError, ValueError, RuntimeError, SyntaxError, subprocess.SubprocessError) as error:
        checks.append({'name': 'failure', 'detail': str(error)})
        REPORT.write_text(json.dumps({'passed': False, 'checks': checks}, indent=2) + '\n')
        print(str(error), file=sys.stderr)
        return 1
    REPORT.write_text(json.dumps({'passed': True, 'elapsed_seconds': round(time.monotonic() - started, 2), 'checks': checks}, indent=2) + '\n')
    print(f'Desktop source checks passed ({len(checks)} groups).')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
