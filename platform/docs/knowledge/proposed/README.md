# Consolidated proposal review

Review abstract rules, not individual debugging stories.

- `20260820-venus-programming-contract-v1.json` is the review unit for stable
  Venus syntax, memory and ABI contracts.
- `20260820-venus-performance-design-principles-v1.json` is the review unit for
  portable optimization methods and AI search prompts.
- `20260820-venus2-compatibility-profile-bf82d804.json` collects values and
  behaviors scoped only to the recorded Venus2 backend and RTL digest.

Older proposal files remain immutable source evidence and counterexamples.
Their presence does not imply that their workload-specific wording should be
promoted. The index records which consolidated review unit owns each source.

Neither consolidated proposal edits `foundations/` or `live/`. After human
scope review, generate an exact shadow patch against the recorded base
digests, run the required regressions, and bind the final decision to the
proposal, patch and base digests.
