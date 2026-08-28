#!/usr/bin/env python3
"""Render a HostLink descriptor's manual content as reviewable markdown.

    tools/manual_copy.py descriptor.json > docs/manual-copy.md

Feed it either the host-harness dump or a live capture:
    lib/alchemy-sdk/tools/hostlink-cli/hostlink.mjs -p <port> descriptor
"""
import json, sys

d = json.load(open(sys.argv[1]))
m = d["manual"]; comps = {c["id"]: c for c in d["components"]}
o = []; w = o.append

w("# Colacut — interactive manual copy\n")
w("> **Generated file — do not edit.** Regenerating overwrites it.\n"
  "> The copy lives in `src/capicola_manual.cpp`; edit there, then rerun\n"
  "> `tools/manual_copy.py`. This is the exact text the programmer on\n"
  "> hermeticmodular.com renders.\n")
w(f"**Tagline** — {d['module']['tagline']}\n")
w("## Preamble\n"); w(m["preamble"] + "\n")
for s in m["sections"]:
    w(f"## Section — {s['title']}  `#{s['id']}`\n"); w(s["body"] + "\n")

def fields(cid, title, pf=None):
    w(f"## {title}\n")
    for f in comps[cid]["fields"]:
        if pf is not None and f.get("page") != pf: continue
        disp = f.get("disp", {}); k = disp.get("kind", ""); rng = ""
        if k in ("linear", "exp"):
            u = (" " + disp["unit"]) if disp.get("unit") else ""
            rng = f" — {k} {disp.get('lo')}…{disp.get('hi')}{u}"
        elif k == "enum": rng = f" — {' / '.join(disp.get('labels', []))}"
        elif k == "norm": rng = " — 0…100 %"
        w(f"### {f['name']}  `{f['id']}`{rng}")
        w(f.get("help", "—"))
        if f.get("see"): w(f"\n*See also: {', '.join(f['see'])}*")
        w("")

fields("pager", "Primary page knobs", 0)
fields("pager", "Depth page knobs", 1)
fields("routing", "Routing page")
fields("secondary", "Secondary page")

w("## Jacks\n")
for j in d["jacks"]:
    w(f"### {j['name']}  `{j['id']}` — {j['sig']}, silk **{j.get('short', '—')}**")
    w(j.get("help", "—"))
    if j.get("see"): w(f"\n*See also: {', '.join(j['see'])}*")
    w("")

w("## Buttons\n")
for b in d["buttons"]:
    w(f"### {b['name']}  `{b['id']}`"); w(b.get("help", "—")); w("")
    for a in b.get("actions", []):
        w(f"- **{a['label']}** (`{a['gesture']}`) — {a.get('help', '—')}")
    if b.get("see"): w(f"\n*See also: {', '.join(b['see'])}*")
    w("")

w("## Presets — SDK stock text (override with `Manual::PresetsHelp`)\n")
w(m["presets"]["help"] + "\n")
print("\n".join(o))
