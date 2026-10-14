import argparse
import re
from pathlib import Path


PATTERNS = [
    rb'AIza[0-9A-Za-z_-]{35}',
    rb'gh[pousr]_[A-Za-z0-9]{30,}',
    rb'github_pat_[A-Za-z0-9_]{40,}',
    rb'hf_[A-Za-z0-9]{30,}',
    rb'sk-(?:proj-)?[A-Za-z0-9_-]{32,}',
    rb'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----',
    rb'/Users/[A-Za-z0-9._-]+/',
]


def verify(root):
    root = root.resolve()
    if not root.is_dir():
        raise ValueError('Release folder does not exist')
    count = 0
    for p in sorted(root.rglob('*')):
        relative = p.relative_to(root)
        if '.git' in relative.parts:
            continue
        if p.is_symlink():
            raise ValueError('Symlink not allowed in release: ' + str(relative))
        if not p.is_file():
            continue
        if p.name in ('.DS_Store', 'SOURCES.tsv', 'secrets.h', 'secrets.json') or p.name.startswith(('._', '.env')):
            raise ValueError('Private/local metadata: ' + str(relative))
        if any(re.search(pattern, p.read_bytes()) for pattern in PATTERNS):
            raise ValueError('Possible private data: ' + str(relative))
        count += 1
    print('PASS:', count, 'files screened for local metadata and basic privacy signatures. No integrity verification; not a universal secret detector.')


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description='Limited local privacy checks; no checksum manifest required.')
    ap.add_argument('folder', type=Path)
    verify(ap.parse_args().folder)
