# Forge onboarding release checklist

This is a local release candidate, not permission to publish or merge.

1. Review only the onboarding commits in ACE-ECHO and Echo Hub. Preserve the
   existing research worktrees. There are no DSL, Scheduler, Gem5 semantic or
   RTL source changes in this candidate.
2. Require platform and workload unit tests, a fresh same-source Gem5 build,
   and the three-case/two-task exact-output smoke in a new checkout. Retain
   the build receipt, host/compiler hashes, pinned submodule identities,
   output hashes, raw ticks/actual clock conversion and failure evidence.
3. Do not distribute proprietary compilers, RTL, IP, tools or credentials
   without their owner's permission. Document how an authorized developer
   obtains them. The fast entrypoint does not require RTL or a license server.
4. After explicit publication approval, push the reviewed Hub branch first;
   verify that the exact Hub gitlink commit is remotely fetchable. Then push
   the platform onboarding branch and verify the exact DSL gitlink too.
   Do not push the platform with a locally-only Hub commit.
5. Repeat a recursive clone from the actual published URLs using a different
   authorized user's access. Do not use local submodule URL overrides for
   this publication check. Before that check, report local reproduction only.
6. Merge into/default to `main` or tag a release only with separate approval.
   Until then, the onboarding branch is the documented entrypoint. Do not
   silently combine ACE-ECHO's old main with the latest Hub.

The historical bootstrap report in `docs/VALIDATION.md` describes a different
workload/revision. It is not evidence for the new onboarding release.
`components/manifest.json` also retains snapshot provenance; current runs
must use actual gitlinks and setup/build receipts rather than assuming the
historical binary hashes name installed files.

Acceptance boundaries:

- `make test`: host-side regression checks; no accelerator execution claim.
- `make smoke`: real independent Venus compile plus Gem5 fast and all-output
  checks against a provisional mathematical smoke model.
- Neither implies a human-approved application golden, full L1 firmware,
  complete HIVE/OAI application reproduction, or RTL consistency.
- No new autonomous LLM executor is included. The AI host drives the existing
  Forge skill; the Forge CLI validates/initializes the request.
