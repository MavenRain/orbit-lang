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

## S3

Slice S3 (base-span pruning, PLAN section 4 and section 8) is complete on
commit 7734007 plus the S3 changes. The design is mechanism (a) of PLAN
section 8 "Builder call (S3, 2026-10-16)": a visibility tag on the 66
BUILTINS (VIS_INTERNAL for the kit names and the sealed-lang base ops),
the origin ORIGIN_KIT for the contract prelude, and a check mode. Guest
mode is the default and refuses an INTERNAL name, a Nat or word literal
and a `state` or `event` declaration (REFUSE_KIT). `langc --kit` selects
the kit mode, and the kit gate scripts pass it. The table SIGNATURE in
`src/front/check.c` holds the 11 types and 9 operations of the design
signature with their kinds (REFUSE_SIG). The gate `make -B check` is
GREEN, exit 0, with the clang `-Werror` check.

| Count | S2 (7734007) | S3 | Reason |
|---|---|---|---|
| examples | 24 | 25 | `signature` (guest): the four type slots from formers and `act` at its slot type (the Z2 flip) |
| parse refusals | 19 | 19 | no change (the 17 kit rows run under `--kit`, deviation 2) |
| check refusals | 91 | 119 | 22 REFUSE_KIT rows and 6 REFUSE_SIG rows (tables below) |
| kit-only refusals | none | 22 | new gate assertion: each REFUSE_KIT row gives `ok` under `--kit`, so the refusal comes from the mode only |
| evals | 61 | 62 | `signature flipped => inr unit` (a former value prints with no kit type) |

The other counts do not change: runs 5, run refusals 10, asmtool 8 of 8,
builds 3, lower refusals 14, build checks 31, dispatch 20, evm steps 43,
precompile steps 18, mock 13 rows, diff 15 rows, emit 24, and the
review, domain, path, fuel, output, lower and hashing regressions. Each
S3 count change is from session 4; session 3 (the kit lists) changed no
count.

Rows per mode. A kit row uses a kit name or a literal, is listed in a
`kit.txt` file and runs under `--kit`. Its expected text does not change.

| Gate | Kit rows | Guest rows |
|---|---|---|
| examples (`examples/kit.txt`) | 24 | 1 (`signature`) |
| check refusals (`test/check/kit.txt`) | 80 | 39 (the 28 S3 rows and 11 older rows with no kit text) |
| parse refusals (`test/parse/kit.txt`) | 17 | 2 |
| evals | 61 | 1 (`signature flipped`) |

REFUSE_KIT rows (22, guest mode). Each gives `ok` under `--kit`. The
Mutant column names the single-site mutant that admits the row.

| Row | Form | Mutant |
|---|---|---|
| `kit-type-nat`, `kit-type-u256`, `kit-type-addr`, `kit-type-flag`, `kit-type-map` | `Nat`, `U256`, `Addr`, `Flag`, `Map Nat Nat` as a type | K1 |
| `kit-op-nat` | `natAdd` | K1 |
| `kit-map-op` | `mapEmpty` | K1 |
| `base-natdiv`, `base-natmod`, `base-hash`, `base-addrtou256` | `natDiv`, `natMod`, `hash`, `addrToU256` | K1 |
| `base-ecrecover`, `base-ecadd`, `base-ecmul`, `base-ecpairing` | `ecrecover`, `ecAdd`, `ecMul`, `ecPairing` applied | K1 |
| `kit-env` | `Env` (a family of the contract prelude, ORIGIN_KIT) | K2 |
| `kit-pay` | `pay : Addr -> Addr -> U256 -> Out` | K2b (K1 + K2) |
| `kit-literal-nat`, `kit-literal-word` | the literals `3 : Nat` and `5u : U256` | K3 (K1 + the literal test) |
| `kit-state` | `state State := makeState` (no field) | K4 |
| `kit-event` | `event Opened` | K4 |
| `kit-entry` | that state and an entry over `Env` and `Out` that gives `none` | K4b (K2 + K4) |

REFUSE_SIG rows (6). Each refuses in both modes.

| Row | Form | Mutant |
|---|---|---|
| `sig-defined` | `def Orbit : Type 0 := Unit` (a defined name, reserved until S4) | K6 |
| `sig-primitive` | `def ZKWitness : Type 0 := Unit` (a primitive name, reserved until S8) | K6 |
| `sig-primitive-op` | `def proveZK : Unit -> Unit` (a primitive operation) | K6 |
| `sig-slot-universe` | `def Outcome : Type 1 := Type 0` (a slot at the wrong universe) | K7 |
| `sig-act-type` | `act : Group -> Choice -> Choice` (the wrong slot type) | K7 |
| `sig-act-early` | `act` before the Member, Choice and Group slots | K7 |

Mutants, one at a time (runner `orbit-lang-work/S3-mutants.sh`, the S2
head, `row()` and `mut()` verbatim). The new check helper `admit()` runs
a guest row: exit 0 with `ok` is an admission (the mutant is KILLED),
any other result is a GAP. After the loop, `src/front/check.c` and
`src/main.c` equal their saves (cmp), and `make -B check` is GREEN.

| Mutant | Edit | Killer | Verdict |
|---|---|---|---|
| K1 INTERNAL builtin admitted | check.c `kit_head`: `return 0 * (h->builtin->vis == VIS_INTERNAL);` | 15 rows `kit-type-*`, `kit-op-nat`, `kit-map-op`, `base-*`: want REFUSE_KIT, got `ok` | KILLED |
| K2 ORIGIN_KIT family admitted | check.c `kit_head`: `return 0 * (fam != NO_FAMILY) && ...` | `kit-env`: want REFUSE_KIT, got `ok` | KILLED |
| K2b K1 + K2 | both edits | `kit-pay`: want REFUSE_KIT, got `ok` | KILLED |
| K3 K1 + literal admitted | K1 and check.c `literal_ok`: `return 0 * guest(c) ? ...` | `kit-literal-nat`, `kit-literal-word`: want REFUSE_KIT, got `ok` | KILLED |
| K4 state and event admitted | check.c `check_decl`: `if (0 * (d->is_event \|\| d->is_state) ...` | `kit-state`, `kit-event`: want REFUSE_KIT, got `ok` | KILLED |
| K4b K2 + K4 | both edits | `kit-entry`: want REFUSE_KIT, got `ok` | KILLED |
| K5 kit mode always on | main.c: `opt.mode = 1 ? CHECK_KIT : CHECK_GUEST;` | all 22 REFUSE_KIT rows: want REFUSE_KIT, got `ok` | KILLED |
| K6 reserved signature name admitted | check.c `check_sig`: `return 1 ? 1 : FAIL(c, "REFUSE_SIG", "%s is a ...` | `sig-defined`, `sig-primitive`, `sig-primitive-op`: want REFUSE_SIG, got `ok` | KILLED |
| K7 slot type not checked | check.c `check_sig`: `return 1 \|\| slot_ok(...)` | `sig-slot-universe`, `sig-act-type`, `sig-act-early`: want REFUSE_SIG, got `ok` | KILLED |

Score: 9 of 9 killed, 50 admissions, 0 GAP. Run 1 of the loop was
INVALID: the K1 check function looped with `for f`, and thus wrote over
the global `$f` of `mut()`. K1 was not restored, and K2 to K7 ran on top
of K1. The fix: check.c was restored from a scratch save by cp (only
line 1890 was different), the loop variable is now `name`, and the
final "equals every save" line prints only when each cmp passes. Run 2
is the clean run above.

Deviations from the S3 build brief (7):

1. `test/abi.expect` lines 46 to 51 are the 7 lines that
   `build/langc --kit abi examples/events.lang --json` prints after
   `langc: USAGE:` (USER ruling 2026-10-17: the usage text is the
   expected text of the `--json` row).
2. `test/parse/kit.txt` exists: `deep.lang` (PARSE_DEPTH) and 16 rows
   with kit text. Parse rows fail before the checker, so the mode does
   not change their result.
3. All 24 old examples are kit rows. `signature.lang` is the only guest
   example.
4. The gate.sh usage row `langc --kit` with no other argument exits 2.
5. `test/fronttool.c` `domain()` wraps `checked(text, origin, mode,
   want)`. The new `guest()` (ORIGIN_PROGRAM, CHECK_GUEST) carries two
   rows, because an ORIGIN_DOMAIN row never gives REFUSE_KIT.
6. The 28 S3 check rows are guest rows outside `test/check/kit.txt`
   (the one exception to "listed if and only if kit text").
7. The `langc` calls of `test/buildtool.c` (execl) and
   `test/lowertool.c` (system) pass `--kit`. Session 2 changed the 38
   script call sites but not these two C sites, and guest mode refused
   `storage.lang` and `contract.lang` before the write.

Mutant-table deviations: K4 runs without K1 and K4b runs without K1,
because `kit-state`, `kit-event` and `kit-entry` use no INTERNAL
builtin.

S3 adds no binder, no universe and no declaration form, so no new
Prop-like form opens and no Prop-like row is added.

Builder deaths. In S3 session 1 the wf-builder stage died before any
file change, and the opus fallback died too:

| Session | Fable (429, rate limit) | opus fallback (`reasoning_extraction`) |
|---|---|---|
| 1 | req_011Cg7Gwq2cfDx1ecqQvQP9Y | req_011Cg7GyTPBAhCqGyUb1d7dC |

Sessions 2 to 5 (PART A, the kit lists, PARTS B and C, this section and
the gate) did not call a builder. The main loop did the work by hand.
