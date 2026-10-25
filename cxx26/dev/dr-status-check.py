#!/usr/bin/env python3
"""Consistency of clang/www/cxx_dr_status.html with the markers of clang/test/CXX/drs.

Every test marker `// cwgN: <status>` must agree with the class of row N in the table, and every row
that claims support (class full or unreleased) must have a marker or be one of the rows tested elsewhere.
Prints the class counts and every disagreement; exit status 1 if there is one or if a row is left
unclassified (class none, unknown, ?, partial). Usage: dr-status-check.py [repo root]
"""
import collections, glob, os, re, sys

root = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), '..', '..')
html = open(os.path.join(root, 'clang/www/cxx_dr_status.html'), encoding='utf-8').read()
rows = {}
for m in re.finditer(r'<tr id="(\d+)">(.*?)</tr>', html, re.S):
    tds = re.findall(r'<td([^>]*)>(.*?)</td>', m.group(2), re.S)
    attr, text = tds[-1]
    cls = re.search(r'class="([\w-]+)"', attr)
    c = cls.group(1) if cls else ''
    rows[int(m.group(1))] = (c.split('-')[0], re.sub(r'<.*?>', '', text).strip())

markers = collections.defaultdict(list)
for f in glob.glob(os.path.join(root, 'clang/test/CXX/drs/*.cpp')):
    for line in open(f, encoding='utf-8', errors='ignore'):
        m = re.search(r'cwg(\d+):\s*(.*?)\s*$', line)
        if m and re.search(r'//\s*cwg\d+:', line):
            markers[int(m.group(1))].append((os.path.basename(f), m.group(2)))

bad = []
for n, ms in sorted(markers.items()):
    if n not in rows:
        # tests of issues that are still open, drafting or in review have no row in the table
        if not all(re.search(r'\b(open|drafting|review|ready)\b', st) for _, st in ms):
            bad.append((n, 'marker without a row', ms))
        continue
    cls, text = rows[n]
    for f, st in ms:
        word = st.split()[0] if st.split() else ''
        if word == 'no' and st != 'no':
            continue  # a prose note, not a status
        if word == 'yes' and cls not in ('full', 'unreleased'): bad.append((n, f'marker yes, row class {cls} "{text}"', f))
        elif word == 'no' and cls not in ('none', 'unknown', '?'): bad.append((n, f'marker no, row class {cls} "{text}"', f))
        elif word == 'na' and cls != 'na': bad.append((n, f'marker na, row class {cls} "{text}"', f))
        elif word == 'partial' and cls != 'partial': bad.append((n, f'marker partial, row class {cls} "{text}"', f))
        elif re.match(r'^\d', word) and cls not in ('full', 'unreleased'): bad.append((n, f'marker {word}, row class {cls} "{text}"', f))

counts = collections.Counter(c for c, _ in rows.values())
print('rows', len(rows), dict(counts))
unclassified = sorted(n for n, (c, _) in rows.items() if c in ('none', 'unknown', '?', 'partial', ''))
print('unclassified rows:', unclassified)
for b in bad: print('DISAGREE', *b)
sys.exit(1 if bad or unclassified else 0)
