# Golden and correctness contract

## Authority states

- `SOURCE_IMPLEMENTATION`: user-provided implementation that must first build,
  run, and gain deterministic tests.
- `PROVISIONAL_GOLDEN`: an agent-created or not-yet-reviewed reference. It may
  guide implementation and Gem5 work but cannot close final RTL correctness.
- `AUTHORITATIVE_GOLDEN`: exactly one reference bundle whose digest a human
  accepted.
- `MISSING`: reference work is required before Venus generation.

Do not combine multiple authorities by voting. Use additional implementations
as cross-checks, then resolve disagreements and freeze one authority.

## Runnable reference adapter

Declare argv arrays for `prepare`, `build`, `run`, and `extract`, plus the
environment identity, input/output schemas, and comparison policy. Avoid shell
strings and implicit environment state. Preserve command receipts and source
revision.

For a supplied foreign-language source, build and execute its real call path,
then add canonical, boundary, extreme, aligned/unaligned, fixed-seed random,
successful, and expected-failure cases. For intent/spec-only input, implement a
clear scalar reference in Python, MATLAB, or C and stop for human review before
promoting its digest.

## Fixed-point gate

A floating reference measures mathematical accuracy only. Before integer
Venus comparison, freeze:

- every input, intermediate checkpoint, and output width;
- signedness and complex/interleaved layout;
- Q format and scaling;
- accumulator width and integer-promotion behavior;
- shift direction and negative-value behavior;
- rounding and tie rule;
- saturation or wrap/overflow behavior;
- alias, in-place, stride, and tail behavior;
- error, early-return, and CRC-failure outputs.

Compare the approved integer representation bit-for-bit.

## Test corpus and configuration matrix

Freeze actual input payloads, configs, seeds, outputs, and SHA-256 identities.
Never regenerate random cases inside a candidate measurement. For radio
applications include noiseless, AWGN, frequency-selective multipath, MIMO
rank/correlation and near-singular cases, timing/CFO variation, supported
modulation, LDPC base graphs, successful decode, and expected CRC failure as
applicable.

All declared configurations must pass the golden/Gem5 matrix. RTL must cover
every structurally different class plus minimum/maximum boundaries. Label an
unrun configuration `SOFTWARE_VERIFIED`, not `RTL_QUALIFIED`.

## Output coverage

Compare all externally visible outputs and lengths. For DAGs compare final
outputs and every reference-available task checkpoint. Identify the first
divergent checkpoint. Never accept a final payload/CRC match as a substitute
for a required soft-output or intermediate mismatch.
