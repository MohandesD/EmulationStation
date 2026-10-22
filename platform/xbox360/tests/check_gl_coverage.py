"""Fail when upstream adds a GL call outside the Xbox adapter. Not a runtime test."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[3]
calls = set()
for subtree in ('es-core/src', 'es-app/src'):
    for path in (root / subtree).rglob('*'):
        if path.suffix not in ('.h', '.cpp'):
            continue
        source = re.sub(r'/\*.*?\*/|//[^\n]*', '', path.read_text(), flags=re.S)
        calls.update(re.findall(r'\b(gl[A-Z]\w*)\s*\(', source))
provided = set(re.findall(r'\b(gl[A-Z]\w*)\s*\(', (root / 'platform/xbox360/es_gl.h').read_text()))
assert not calls - provided, f'Unsupported GL calls: {sorted(calls - provided)}'
print(f'PASS: all {len(calls)} upstream GL entry points have adapter declarations')
