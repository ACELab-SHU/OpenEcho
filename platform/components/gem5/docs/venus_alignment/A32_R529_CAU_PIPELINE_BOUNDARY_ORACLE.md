# A32 R529 CAU pipeline boundary oracle

Date: 2026-08-13

Status: RTL boundary proved; candidate implementation rejected and fully
reverted to the R508/R525 accepted baseline.

## Scope

This iteration split task20 sequence 33 `VRANGE` and task17 sequence 4 `VMUL`
into operand admission, arithmetic output, result FIFO, stable VRF requester,
bank grant, and retirement boundaries.  No LSU/VFU fixed latency, task, PC,
DAG, address, or data special case was accepted.

## RTL proof

The task20 tail RTL oracle observes `venus_cau_wrapper` directly:

| boundary | RTL edge |
| --- | ---: |
| issue/lane command | 11 |
| operand requester first active | 13 |
| operand valid/ready and CAU input/output valid | 17 |
| `result_queue_d` count=1/valid=01 | 17 |
| `result_queue_q` count=1/valid=01 and result request | 19 |

Thus latency-zero CAU output enters `result_queue_d` on the input handshake
edge, while the VRF requester is driven only from `result_queue_q` on the
following CAU edge.

The existing task17 normal-CAU oracle independently shows, for `VMUL`:

| boundary | RTL edge |
| --- | ---: |
| operand requester grant | 15 |
| operand ready/consume | 19 |
| `result_queue_d` | 23 |
| `result_queue_q` / result request | 25 |

This proves grant-to-consume=2 lane cycles, consume-to-result-D=2 cycles for
`LatMul=2`, and result-D-to-Q=1 cycle.  The D/Q boundary is not a `VRANGE`
special case.

## Gem5 first divergence

At the accepted baseline, task20 sequence 33 exposes the CAU result requester
one lane cycle before RTL.  The first stable bank-0 requester vectors match on
the two preceding edges; gem5 then presents CAU result master 9 at edge 4275,
where RTL does not present it until edge 4276.  Initial per-bank RR state and
the lane command/requester vector before that edge match, so this is upstream
of the RR winner selection.

Task17 demonstrates why globally delaying CAU result visibility is invalid.
Its RAW-forwarded VMUL already has the correct public timing because two model
errors cancel: its arithmetic result enters the gem5 result queue one cycle
late, while that new queue entry is offered to VRF one cycle too early.

## Rejected structural experiments

The following general experiments were run without workload special cases:

1. Remove immediate result-queue service.  This makes task20 sequences 31--37
   exact, but task17 regresses from sequence 4 and accumulates across 683 of
   687 instructions.
2. Register all ordinary CAU VRF responses, directly capture operand-pop into
   the arithmetic pipe, and give result entries an explicit following-edge
   visibility tick.  This exposes event-order coupling between the operand
   consumer and arithmetic pipe; task17 sequence 4 becomes 66 rather than 64
   cycles and ordinary CAU traffic regresses substantially.
3. Give the CAU arithmetic event sole bubble-shift ownership.  In the current
   mixed polling/event implementation this still does not reproduce the RTL
   edge ordering and therefore was also rejected.

All behavior changes from these experiments were reverted.  The diagnostic
stable-requester trace remains as observability only.

## Restored regression

- task17: all 687 entries have identical fire/recycle/duration fields versus
  R502; first VMUL is again 64 cycles.
- task20: all 8,837 entries have identical fire/recycle/duration fields versus
  R525; sequences 29--40 are restored.
- restored binary SHA256:
  `da186ad190cdad09983209b5ca42620d6962dfa73cf8ec52a1fc8bc1f982f09d`.

Evidence is under
`evidence/a32_r529_cau_pipeline_oracle_20260813/`.  Rejected runs remain under
`/tmp/a32_r533` through `/tmp/a32_r543` and are not acceptance evidence.

## Next implementation boundary

The next change must replace the CAU's split periodic-poll/start-event
interaction with one explicit clocked state transition covering:

`operand_queue_q -> cau input handshake -> arithmetic pipe D/Q -> result_queue_d/q`.

Only after that state machine reproduces both the latency-zero task20 oracle
and the RAW VMUL task17 oracle should its stable result requester be connected
to the already persistent per-bank RR model.  This avoids encoding another
pair of compensating delays.
