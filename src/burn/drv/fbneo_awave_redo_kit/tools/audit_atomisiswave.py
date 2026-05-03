#!/usr/bin/env python3
from pathlib import Path
from collections import Counter
import sys

root = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
if root.name != 'atomiswave':
    root = root / 'atomiswave'
files = [p for p in root.rglob('*') if p.is_file()]
print(f'root={root}')
print(f'file_count={len(files)}')
print(f'total_bytes={sum(p.stat().st_size for p in files)}')
print('suffixes=')
for k, v in Counter(p.suffix for p in files).most_common():
    print(f'  {k or "<none>"}: {v}')
for prefix in ['flycast/core', 'awave_wrap_', 'd_awave.cpp', 'd_naomi.cpp', 'awave_core.cpp']:
    if prefix.endswith('.cpp'):
        ps = [root / prefix]
    elif prefix == 'awave_wrap_':
        ps = [p for p in files if p.name.startswith(prefix)]
    else:
        ps = [p for p in files if str(p.relative_to(root)).startswith(prefix)]
    ps = [p for p in ps if p.exists()]
    print(f'{prefix}: files={len(ps)} bytes={sum(p.stat().st_size for p in ps)}')
