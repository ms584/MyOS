from pathlib import Path
import json,re,statistics,sys
source=Path(sys.argv[1]);rows=[]
pattern=r'DIAGNOSTIC EGL TSC frame/gap/finish/present ([0-9a-f]{16}) ([0-9a-f]{16}) ([0-9a-f]{16}) ([0-9a-f]{16})'
for match in re.finditer(pattern,source.read_text()):
 frame,gap,finish,present=(int(value,16) for value in match.groups())
 rows.append(dict(frame=frame,gap_tsc=gap,finish_tsc=finish,present_tsc=present))
assert rows,'No completed EGL timing frames yet'
paired=[row for row in rows if row['gap_tsc']]
summary=dict(records=len(rows),median_tsc={key:statistics.median(row[key] for row in rows) for key in ('gap_tsc','finish_tsc','present_tsc')},
 median_sampled_interval_fractions={key:statistics.median(row[key]/(row['gap_tsc']+row['finish_tsc']+row['present_tsc']) for row in paired) for key in ('gap_tsc','finish_tsc','present_tsc')} if paired else {},
 caveats=['Raw guest TSC intervals, not CPU cycles/utilization or performance percentages',
 'Other VM full regression runs concurrently; host scheduling affects all intervals',
 'Gap includes GL calls, Cobalt work, scheduling and waits; not exclusively drawing',
 'EGL logging is serial-only after native window ownership; diagnostic work perturbs execution',
 'Completed frames only; no native responsiveness acceptance'])
print(json.dumps(summary,indent=2))
source.with_name('egl-timing-analysis.json').write_text(json.dumps(dict(summary=summary,rows=rows),indent=2))
