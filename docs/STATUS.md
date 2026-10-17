# orbit-lang status

> Write here: one paragraph. Say which milestone the build implements, and
> which milestone it does not complete yet.

## Implemented

> Write here: one item for each feature that works. Name the former (F1 to
> F15), the core type or the core operation.

- <feature>.

## Remaining M0 work

> Write here: one numbered item for each missing feature. Say what each item
> needs first.

1. <work item>.

## Known limits

> Write here: one item for each limit of the current build, with its cause
> (a fuel budget, a host limit, a stack size).

- <limit>.

## Internal boundaries

> Write here: one paragraph for each module group. Say which module holds
> which job, which tests check which tables, and which types a program
> cannot use.

## S2

Slice S2 (the port of the sealed-lang S2 to S4 kernel, PLAN section 8) is
complete on commit 6cd1104 plus the S2 changes. The port applies
sealed-lang bc8e168 first, then the sealed-lang S3 and S4 commits (route
Q, PLAN section 8 "Builder call (S2, 2026-10-16)"). The gate
`make -B check` is GREEN, exit 0, with the clang `-Werror` check.

| Count | S0 | S2 port | S2 | Reason |
|---|---|---|---|---|
| examples | 19 | 24 | 24 | port: 6 sealed-lang examples come in (`circle`, `kan`, `paths`, `precompile`, `quot`, `trunc`); the sample domain example `inventory` goes |
| parse refusals | 13 | 15 | 19 | port: `dangling-at`, `system-top`; S2: 4 irrelevance marker spellings |
| check refusals | 57 | 83 | 91 | port: 27 sealed-lang S2 to S4 rows come in, the sample domain row `domain-name` goes; S2: 8 Prop-like rows |
| evals | 55 | 61 | 61 | port: the sealed-lang S2 to S4 eval rows |
| lower refusals | 11 | 14 | 14 | port: `pairing-open`, `path-word`, `loop-word` |
| build checks | 28 | 31 | 31 | port: the sealed-lang build rows |
| precompile steps | 0 | 18 | 18 | port: sealed-lang bc8e168 gives the runner 0x01, 0x06, 0x07, 0x08 (`test/precompile.sh`) |
| path regressions | none | passed | passed | port: `test/pathtool.c` (sealed-lang S3 and S4) |

The other counts do not change: runs 5, run refusals 10, asmtool 8 of 8,
builds 3, dispatch 20, evm steps 43, mock 13 rows, diff 15 rows, emit 24,
and the review, domain, fuel, output, lower and hashing regressions.

Prop-like refusals (USER ruling S1: no proof irrelevance). The ported
kernel admits no Prop-like form. One hardening: the reserved names gain
`Prop` and `SProp` (`src/front/check.c` RESERVED, rule R4), thus
`def Prop : Type 1 := Type 0` is REFUSE_NAME. Before this change the
kernel admitted it as a plain name. The 12 new rows:

| Row | Form | Code |
|---|---|---|
| check `prop-type`, `sprop-type` | `Prop`, `SProp` as a type | NAME_UNKNOWN |
| check `prop-def`, `sprop-def` | `Prop`, `SProp` as a defined name | REFUSE_NAME |
| check `trunc1-refl`, `trunc0-refl`, `isprop-refl` | refl between two non-convertible elements of Trunc1 A, Trunc0 A and an isProp-typed type | PATH_END |
| check `loop-twice-not-loop` | a path of paths by refl between two different loops (K) | PATH_UIP |
| parse `irrelevant-dot`, `irrelevant-brace`, `irrelevant-attr` | irrelevant binder markers `.(x : Nat)`, `{x : Nat}` and the attribute `#[irrelevant]` | LEX_CHAR |
| parse `irrelevant-at` | irrelevant binder marker `(@irrelevant x : Nat)` | PARSE_EXPECT |

UIP and K were already refused by the ported rows `path-uip`, `path-k`
and `loop-not-refl` (PATH_UIP, rule R9).

Mutants, one at a time (runner `orbit-lang-work/S2-mutants.sh`). The
runner saves each file with cp, mutates it with awk, moves its mtime 2 s
back, rebuilds with `make -B build`, refuses a stale `build/langc`, runs
the rows and restores the file with cp. After the loop, check.c, lexer.c
and evm.c equal their saves, and `make -B check` is GREEN.

| Mutant | Edit | Killer | Verdict |
|---|---|---|---|
| M1 Prop names not reserved | check.c RESERVED: `"Prop", "SProp"` removed | check refusals `prop-def`, `sprop-def`: want REFUSE_NAME, got exit 0 | KILLED |
| M2 braces read as parentheses | lexer.c: `{` and `}` lex as TOK_LPAREN and TOK_RPAREN | parse refusal `irrelevant-brace`: want LEX_CHAR, got exit 0 | KILLED |
| M3 PATH_END fails without a diagnostic | check.c end_fail: the PATH_END branch returns 0 | `trunc1-refl`, `trunc0-refl`, `isprop-refl`: want PATH_END, got exit 1 with no message | KILLED (diagnostic loss) |
| M4 PATH_UIP fails without a diagnostic | check.c end_fail: the PATH_UIP branch returns 0 | `loop-twice-not-loop`, `path-uip`, `path-k`, `loop-not-refl`: want PATH_UIP, got exit 1 with no message | KILLED (diagnostic loss) |
| M3b PATH_END end check passes | check.c end_fail: the PATH_END branch returns 1 (a pass for path_ends) | `trunc1-refl`, `trunc0-refl`, `isprop-refl`: want PATH_END, got exit 0 | KILLED |
| M4b PATH_UIP end check passes | check.c end_fail: the PATH_UIP branch returns 1 (a pass for path_ends) | `loop-twice-not-loop`, `path-uip`, `path-k`, `loop-not-refl`: want PATH_UIP, got exit 0 | KILLED |
| M5 STATICCALL result not tested | evm.c: the ISZERO after STATICCALL replaced by POP, the jump to revert removed | precompile.sh `addBad`: want revert, got out 0 | KILLED |

Score: 7 of 7 killed. Run 1 of the loop was INVALID for M2 to M5: the
`/bin/sh` `-nt` test compares whole seconds, thus a rebuild in the same
second looked stale. The 2 s mtime move fixed the runner. M3b and M4b
make the end check pass, so they test admission, not only the
diagnostic.

Builder deaths. The wf-builder stage died before any file change in each
of the three S2 build sessions, and the main loop did the work by hand:

| Session | Fable (429, rate limit) | opus fallback (`reasoning_extraction`) |
|---|---|---|
| 1 | req_011Cg71xjDA2CJ7g7K9rZhTE | req_011Cg722qZGP3W8JehHvqrks |
| 2 | req_011Cg72gdeiPd58RznUBTLvZ | req_011Cg72mEGvebFbGSicpwQ2B |
| 3 | req_011Cg73h3EuJGu3Bhhh5GbVf | req_011Cg73rmyv7miyGmj8aGi4c |

Session 4 (the M3b and M4b mutants, this section and the gate) did not
call a builder.
