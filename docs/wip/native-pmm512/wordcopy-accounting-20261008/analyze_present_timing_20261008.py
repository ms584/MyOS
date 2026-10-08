from pathlib import Path
import json,re,statistics,sys
source=Path(sys.argv[1])
rows=[]
for match in re.finditer(r'DIAGNOSTIC present TSC pid/frame/width/height/gap/present (\d+) (\d+) (\d+) (\d+) (\d+) (\d+)',source.read_text()):
 pid,frame,width,height,gap,present=map(int,match.groups())
 rows.append(dict(pid=pid,frame=frame,width=width,height=height,gap_tsc=gap,present_tsc=present))
assert rows,'No completed frame timing records yet'
paired=[r for r in rows if r['gap_tsc']]
summary=dict(records=len(rows),sizes=sorted(set((r['width'],r['height']) for r in rows)),
 median_present_tsc=statistics.median(r['present_tsc'] for r in rows),
 max_present_tsc=max(r['present_tsc'] for r in rows),
 median_gap_tsc=statistics.median(r['gap_tsc'] for r in paired) if paired else None,
 median_present_fraction_of_sampled_interval=statistics.median(r['present_tsc']/(r['present_tsc']+r['gap_tsc']) for r in paired) if paired else None,
 caveats=['Raw QEMU guest TSC, not instructions/CPU cycles or CPU percent',
 'Only completed sampled frames; diagnostic serial logging excluded from following gap',
 'Prevalidation before stripe copy is included in preceding gap, not present',
 'Long gap includes userspace rendering, scheduling, waits and host execution; cannot attribute glFinish',
 'No responsiveness acceptance or frequency conversion'])
print(json.dumps(summary,indent=2))
output=source.with_name('present-timing-analysis.json')
output.write_text(json.dumps(dict(summary=summary,rows=rows),indent=2))
