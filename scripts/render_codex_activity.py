"""Render a screenshot-derived activity illustration; no API or credentials required."""
import json
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
data = json.loads((ROOT / 'data/codex-activity-visual.json').read_text())
colors = ['#192638', '#183d66', '#226caf', '#349bfa', '#8bd5ff']

def render(animated=True):
    motion = '''@keyframes reveal { from { opacity: 0; transform: translateY(5px); } to { opacity: 1; transform: translateY(0); } }
.cell { animation: reveal .5s cubic-bezier(.23,1,.32,1) both; }
@media (prefers-reduced-motion: reduce) { .cell { animation: none; } }''' if animated else ''
    parts = ['''<svg xmlns="http://www.w3.org/2000/svg" width="1100" height="430" viewBox="0 0 1100 430" role="img" aria-labelledby="title desc">
<title id="title">One month with Codex — September 2026</title>
<desc id="desc">A blue activity grid reconstructed from a user-provided annual token activity screenshot. Cells reveal once, then remain still. Colors represent screenshot intensity, not exact token counts. Month labels run from November to October.</desc>
<defs><linearGradient id="panel" x2="1" y2="1"><stop stop-color="#101c2c"/><stop offset="1" stop-color="#0a111d"/></linearGradient></defs>
<style>text { font-family: ui-monospace, SFMono-Regular, Consolas, monospace; } ''' + motion + '''</style>
<rect x="1" y="1" width="1098" height="428" rx="18" fill="url(#panel)" stroke="#2a3b51"/>
<path d="M1 49H1099" stroke="#2a3b51"/>
<circle cx="25" cy="25" r="5" fill="#ff6b6b"/><circle cx="44" cy="25" r="5" fill="#ffd166"/><circle cx="63" cy="25" r="5" fill="#6ee7b7"/>
<text x="88" y="30" fill="#94a9c3" font-size="12">applelog / field-notes / codex</text>
<text x="1044" y="30" text-anchor="end" fill="#76bfff" font-size="12">SEP 2026</text>
<text x="36" y="96" fill="#f0f6ff" font-size="29" font-weight="700">One month with Codex.</text>
<text x="36" y="124" fill="#a7bad0" font-size="14">I spent September 2026 using Codex.</text>
<text x="36" y="164" fill="#76bfff" font-size="11" letter-spacing="2">TOKEN ACTIVITY / YEAR VIEW</text>
''']
    for c, col in enumerate(data['columns']):
        for row, level in enumerate(col):
            if level is None:
                continue
            parts.append(f'<rect class="cell" x="{36+c*19.8:.1f}" y="{184+row*19}" width="15.6" height="15.6" rx="4" fill="{colors[level]}" style="animation-delay:{c*.045+row*.03:.3f}s"/>')
    months = [('Nov',0),('Dec',5),('Jan',10),('Feb',14),('Mar',19),('Apr',23),('May',28),('Jun',32),('Jul',37),('Aug',41),('Sep',46),('Oct',50)]
    for name,c in months:
        parts.append(f'<text x="{36+c*19.8:.1f}" y="339" font-size="12" fill="'+('#8bd5ff' if name=='Sep' else '#94a9c3')+f'">{name}</text>')
    parts.append('''<path d="M36 359H1064" stroke="#24364c"/>
<text x="36" y="388" fill="#a7bad0" font-size="12">Screenshot-derived illustration · no exact token totals</text>
<text x="36" y="409" fill="#7890aa" font-size="11">Activity records usage, not code quality or project completion.</text>
<text x="877" y="388" fill="#94a9c3" font-size="11">Less</text>''')
    for i,color in enumerate(colors):
        parts.append(f'<rect x="{913+i*20}" y="376" width="14" height="14" rx="3" fill="{color}"/>')
    parts.append('<text x="1019" y="388" fill="#94a9c3" font-size="11">More</text></svg>')
    return '\n'.join(parts)+'\n'

for name, animated in [('codex-september.svg',True),('codex-september-static.svg',False)]:
    (ROOT/'assets'/name).write_text(render(animated))
