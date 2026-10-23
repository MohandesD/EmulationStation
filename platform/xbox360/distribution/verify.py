"""Verify a fresh extracted package without network access or file writes."""
import hashlib
import json
import sys
from pathlib import Path, PurePosixPath


def verify(root):
    root = root.resolve(strict=True)
    manifest = json.loads((root/'manifest.json').read_text(encoding='utf-8'))
    expected = set()
    for item in manifest['files']:
        name = item['path']
        rel = PurePosixPath(name)
        if rel.is_absolute() or '..' in rel.parts or '\\' in name or ':' in name or name in expected:
            raise ValueError('Unsafe/duplicate manifest path')
        expected.add(name)
        path = root.joinpath(*rel.parts)
        if not path.is_file() or path.is_symlink() or root not in path.resolve().parents:
            raise ValueError('Missing/unsafe file: '+name)
        if path.stat().st_size != item['size'] or hashlib.sha256(path.read_bytes()).hexdigest() != item['sha256']:
            raise ValueError('Content mismatch: '+name)
    actual = {p.relative_to(root).as_posix() for p in root.rglob('*') if p.is_file()}
    # Running Python must not change a fresh package; scripts suppress pycache.
    if actual != expected | {'manifest.json', 'SHA256SUMS.txt'}:
        raise ValueError('Unexpected/missing files: '+repr(sorted(actual ^ (expected | {'manifest.json','SHA256SUMS.txt'}))))
    sums = {}
    for line in (root/'SHA256SUMS.txt').read_text().splitlines():
        digest, name = line.split('  ', 1)
        if name in sums or name not in expected | {'manifest.json'}:
            raise ValueError('Invalid checksum entry')
        sums[name] = digest
        if hashlib.sha256((root/name).read_bytes()).hexdigest() != digest:
            raise ValueError('Checksum mismatch: '+name)
    if set(sums) != expected | {'manifest.json'}:
        raise ValueError('Incomplete checksums')
    return len(expected)


if __name__ == '__main__':
    try:
        count = verify(Path(__file__).resolve().parents[1])
    except (OSError, ValueError, KeyError) as error:
        sys.exit(str(error))
    print(f'Verified {count} payload files and manifest. No console connection.')
