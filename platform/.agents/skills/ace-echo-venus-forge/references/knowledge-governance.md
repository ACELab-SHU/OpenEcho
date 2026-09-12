# Knowledge governance

## Layers

Keep formal knowledge under `docs/knowledge/`:

- `foundations`: reviewed instruction syntax, user guides, ABI, DSL, compiler,
  Gem5, RTL, manifests, and source indices;
- `empirical`: backend/hardware-digest-scoped probes, compatibility, timing, and
  failure evidence;
- `seed-prompts`: creative hypotheses that make no truth/performance claim;
- `run_local`: current search trajectory and value updates;
- `proposed`: cross-run lessons awaiting replay and human review;
- `live`: reviewed rules and prompt records only.

The skill stores workflow and retrieval instructions, not bulk hardware facts.

## Retrieval

Freeze a retrieval manifest per search round with authority, evidence state,
backend/hardware scope, freshness, selected item IDs, rejected item IDs, token
limit, and content digest. Similarity creates an eligible pool. Learned value
may rerank it but cannot override authority, correctness, ABI, resource, or
comparability gates.

## Lesson record

Record category, normalized observation, prompt/hypothesis, applicability and
exclusion boundaries, backend/hardware digest, operator/application family,
shape class, evidence artifact digests, measured before/after values,
counterexamples, regressions, risk, and lesson fingerprint.

Automatically write `run_local` and `proposed`. A general performance prompt
normally needs two independent workloads or shape classes. A compatibility
rule needs a focused positive/negative probe and Gem5/RTL evidence. Neither
becomes live without human acceptance.

## Promotion

Generate a shadow patch against a frozen base knowledge digest. Run required
positive, negative, and historical regressions without editing live knowledge.
Present proposal, patch, base, evidence, counterexamples, and regression
digests for review. Stop as `REVIEW_REQUIRED`.

Promote only the exact human-approved patch if all three digests still match.
Record rejected and deferred fingerprints too. Create a reversible commit; do
not push or merge without separate approval.

## Gem5/RTL mismatch learning

Keep RTL immutable. Preserve identical firmware/input identities, find the
first divergent instruction or address event, and produce the smallest
positive/negative probe. Treat the target RTL result as deployment behavior.
Propose a reversible Gem5 semantic patch, static linter rule, regression, and
foundation-doc update. Require human review before applying Gem5 or live Wiki
changes.
