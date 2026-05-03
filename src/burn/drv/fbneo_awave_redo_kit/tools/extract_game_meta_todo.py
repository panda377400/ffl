#!/usr/bin/env python3
"""Draft helper: list game config macro lines that should be moved to awave_driverdb."""
from pathlib import Path
import re, sys
root = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
if root.name != 'atomiswave': root = root / 'atomiswave'
for name in ['d_awave.cpp','d_naomi.cpp']:
    path = root / name
    if not path.exists(): continue
    print(f'## {name}')
    text = path.read_text(errors='ignore')
    for i,line in enumerate(text.splitlines(),1):
        if re.search(r'AW_CONFIG|NAOMI_CONFIG|BurnDrv|ZipEntries\[\]', line):
            print(f'{i}: {line}')
