# ACE-ECHO Knowledge Base

This directory is the reviewable knowledge plane for ACE-ECHO Forge. Runtime
logs and candidate trees belong under `runs/`; they are evidence references,
not live knowledge.

Knowledge has three evidence classes:

- `foundations/`: ISA, ABI, DSL, compiler, Gem5, RTL, and backend facts with
  explicit source labels.
- `proposed/`: run-derived lessons awaiting replay and human review.
- `live/`: reviewed cross-run rules and prompt records only.

`seed-prompts.json` contains creative starting hypotheses. A seed can guide AI
search but cannot establish correctness, compatibility, or performance. A
prompt reaches `live/registry.json` only after reproducible evidence and a
human acceptance decision bound to the proposal, patch, and base digests.

Backend-specific measurements remain namespaced by backend and hardware
digest. Only abstract hypotheses may transfer to a new Venus generation, and
they restart as unverified.
