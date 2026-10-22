"""Check the compiled Xenos fetch interface against es_gpu.c, not GLSL spelling."""
from pathlib import Path
import re
import sys

def check(source):
    match = re.search(r'es_vert_container\[\]\s*=\s*\{(.*?)\};', source, re.S)
    if not match:
        raise ValueError('Missing compiled vertex container')
    words = [int(x, 16) for x in re.findall(r'0x[0-9a-fA-F]+', match[1])]
    if len(words) < 7 or words[0] != 0x102A1101:
        raise ValueError('Not an xsdk vertex container')
    record = words[6] // 4
    if record + 7 >= len(words):
        raise ValueError('Truncated vertex fetch record')
    start = record + 8 + words[record + 6] * 2
    count = words[record + 7]
    if count != 3 or start + count > len(words):
        raise ValueError('Expected three vertex inputs')
    inputs = {((word >> 12) & 15, (word >> 16) & 15) for word in words[start:start+count]}
    if inputs != {(0, 0), (10, 0), (5, 0)}:
        raise ValueError(f'Expected POSITION0/COLOR0/TEXCOORD0, found {sorted(inputs)}')
    if not re.search(r'ES_FRAG_SAMPLER_0_0\s*=\s*0\b', source):
        raise ValueError('Fragment sampler does not use texture slot 0')
    return start

source = Path(sys.argv[1]).read_text()
check(source)
# Mutation check: this gate must reject a shader bound to the wrong sampler slot.
bad = re.sub(r'(ES_FRAG_SAMPLER_0_0\s*=\s*)0\b', r'\g<1>1', source)
try:
    check(bad)
except ValueError:
    pass
else:
    raise AssertionError('Broken sampler was accepted')
print('PASS: compiled POSITION0/COLOR0/TEXCOORD0 and sampler 0; wrong-sampler mutation rejected')
