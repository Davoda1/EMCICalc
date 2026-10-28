#!/usr/bin/env python3
"""Retrieve unchanged corresponding-source inputs against their recorded hashes."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import tempfile
import urllib.request

ROOT = Path(__file__).resolve().parents[1] / 'third_party' / 'sources'

def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--all', action='store_true', help='Retrieve every optional source archive')
    parser.add_argument('--verify-local', action='store_true', help='Verify available files without network access')
    parser.add_argument('files', nargs='*', help='Exact manifest-relative filenames')
    args = parser.parse_args()
    entries = json.loads((ROOT / 'manifest.json').read_text())['entries']
    indexed = {entry['file']: entry for entry in entries}
    optional = json.loads((ROOT / 'retrieved-files.json').read_text())['files']
    if not (args.all or args.verify_local or args.files):
        parser.error('Select --all, --verify-local, or exact filenames')
    selected = list(indexed) if args.verify_local else (optional if args.all else args.files)
    verified = skipped = 0
    for name in selected:
        if name not in indexed:
            parser.error('Unknown manifest filename: ' + name)
        entry = indexed[name]
        target = ROOT / name
        if not target.resolve().is_relative_to(ROOT.resolve()):
            raise RuntimeError('Manifest path escapes source directory')
        if target.is_file():
            if digest(target) != entry['sha256']:
                raise RuntimeError('Existing source checksum mismatch: ' + name)
            verified += 1
            continue
        if args.verify_local:
            skipped += 1
            continue
        url = entry.get('url', '')
        if not url.startswith('https://'):
            raise RuntimeError('Missing HTTPS retrieval URL: ' + name)
        target.parent.mkdir(parents=True, exist_ok=True)
        with tempfile.NamedTemporaryFile(dir=target.parent, delete=False) as stream:
            temporary = Path(stream.name)
            try:
                with urllib.request.urlopen(url, timeout=60) as response:
                    while block := response.read(1024 * 1024):
                        stream.write(block)
                stream.flush()
                if digest(temporary) != entry['sha256']:
                    raise RuntimeError('Downloaded source checksum mismatch: ' + name)
                temporary.replace(target)
            finally:
                temporary.unlink(missing_ok=True)
        verified += 1
    print(f'Verified {verified} source inputs; {skipped} optional inputs absent.')

if __name__ == '__main__':
    main()
