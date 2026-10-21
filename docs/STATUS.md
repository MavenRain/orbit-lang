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

## S4

Slice S4 (Orbit and the dependent eliminator, PLAN section 8) is
complete on commit 3a1ba76 plus the S4 changes. The kernel declares
`Orbit` and `orbitProj` directly after the program definition `act`
(src/front/front.c `orbit_after_act`, origin ORIGIN_DOMAIN, R inline):
Orbit is the quotient of the voter orbits by the relation of `act`, with
the HIT, QIT and truncation formers of the S2 port. The former `elim`
is the dependent eliminator of `Sum A B` and `Unit`. The gate
`make -B check` is GREEN, exit 0, with the clang `-Werror` check (log
`orbit-lang-S4-gate.log`).

USER ruling (S4, 2026-10-17, PLAN section 8): "Add a dependent
eliminator in S4". A probe on 3a1ba76 showed that no Orbit value can get
to a word position: each Orbit fold needs an isSet proof of its target,
and with no dependent eliminator and no Unit eta, no Sum has one.
`elim P c1 .. cn x` takes a motive `P : X -> Type k`, one case for each
constructor and the scrutinee `x : X`, and has the type `P x`, for X
`Sum A B` or `Unit`. The kernel refuses elim on a type with path
constructors (O21), on Nat, List or a Path (no K), and elim with a wrong
case count, case type or motive domain. With elim, `examples/orbit.lang`
proves isSet Choice and folds Orbit into Choice.

| Count | S3 (3a1ba76) | S4 | Reason |
|---|---|---|---|
| examples | 25 | 26 | `orbit` (guest): the slots, `act` (the Z2 flip), isSet Choice by elim and the fold `agree` of Orbit into Choice |
| check refusals | 119 | 132 | 6 Orbit rows and 7 elim and Prop-like rows (table below) |
| evals | 62 | 65 | `orbit agreeAll => inl unit`, `agreeSplit => inr unit`, `agreeFlipped => inr unit` |
| lower refusals | 14 | 15 | `orbit-word` (REFUSE_LOWER) |
| build checks | 31 | 32 | one check for each lower refusal (`orbit-word`) |

The other counts do not change: parse refusals 19, kit-only refusals 22,
runs 5, run refusals 10, asmtool 8 of 8, builds 3, dispatch 20, evm
steps 43, precompile steps 18, mock 13 rows, diff 15 rows, emit 24, and
the review, domain, path, fuel, output, lower and hashing regressions.
Session 4 added `elim-motive-dom` (check refusals 131 to 132).

Rows per mode. The S4 check rows and `orbit.lang` use no kit name and no
literal, so they are guest rows outside the `kit.txt` lists. The lower
rows run under `--kit` (S3 deviation 7), and `orbit-word` reads the
contract state.

| Gate | Kit rows | Guest rows |
|---|---|---|
| examples (`examples/kit.txt`) | 24 | 2 (`signature`, `orbit`) |
| check refusals (`test/check/kit.txt`) | 80 | 52 (39 S3 rows and the 13 S4 rows) |
| parse refusals (`test/parse/kit.txt`) | 17 | 2 |
| evals | 61 | 4 (`signature flipped` and the 3 `orbit` rows) |
| lower refusals | 15 | none |

S4 rows. The Mutant column names the mutant that admits the row.

| Row | Code | Form | Mutant |
|---|---|---|---|
| `orbit-guest-def` | REFUSE_SIG | a program `def Orbit` after `act` | G1 |
| `orbitproj-guest-def` | REFUSE_SIG | a program `def orbitProj` after `act` | G1 |
| `orbit-no-act` | NAME_UNKNOWN | `Orbit` in a program with the four type slots and no `act` | O1 |
| `orbit-fold-no-set` | TYPE_MISMATCH | an Orbit fold into Choice with no quotSquash case | F1 |
| `orbit-fake-set` | PATH_END | the quotSquash case `fun x y p q i j => x` (the ends are not p and q) | P1b |
| `orbit-boundary` | PATH_END | a quotEq path with refl as the witness of R | P1 |
| `elim-case-type` | TYPE_MISMATCH | an inl case of type Unit where the motive gives Sum Unit Unit | E4 |
| `elim-case-count` | TYPE_MISMATCH | elim on Sum Unit Unit with one case | E2 |
| `elim-scrutinee` | TYPE_MISMATCH | elim on Trunc0 (a type with path constructors, O21) | E1 |
| `elim-path-k` | TYPE_MISMATCH | elim on a Path with the case refl (no K) | E1b |
| `elim-motive-dom` | TYPE_MISMATCH | a motive on Sum Unit Unit for a Unit scrutinee | E3 |
| `elim-motive-prop` | NAME_UNKNOWN | a motive into `Prop` (no sort Prop, R11) | N1b |
| `unit-no-eta` | PATH_END | refl between two Unit variables (no Unit eta) | P1 |
| `orbit-word` (lower) | REFUSE_LOWER | `spin`: elim at a word position on the fold `agree` of a formal hcomp at Orbit | none (L1 ruling) |

Mutants, one at a time (runner `orbit-lang-work/S4-mutants.sh`, the S3
head, `row()`, `mut()` and `admit()` verbatim). The new helper
`lowadmit()` runs `langc --kit build` on a lower row: exit 0 is an
admission. Run 1 (session 4) ran G0 to L1. Run 2 (session 5, log
`orbit-lang-S4-s5-mutants.log`) ran them again and added P1b, E1b, N1b
and the probe L1r. The table gives run 2. After the loop,
`src/front/check.c`, `src/front/front.c`, `src/front/eval.c` and
`domain/domain.lang` equal their saves (cmp), and `make -B check` is
GREEN.

| Mutant | Edit | Killer | Verdict |
|---|---|---|---|
| G0 kernel_sig admits | check.c `kernel_sig`: `if (1 \|\| d->origin != ORIGIN_PROGRAM \|\| ...` | both guest-def rows: want REFUSE_SIG, got REFUSE_NAME (a diagnostic kill) | KILLED |
| G1 G0 + name_free + K6 | G0, `return 1 \|\| why == NULL ? 1 : FAIL(c, "REFUSE_NAME" ...` and `return 1 ? 1 : FAIL(c, "REFUSE_SIG", "%s is a ...` | `orbit-guest-def`, `orbitproj-guest-def`: got `ok` | KILLED |
| O1 Orbit with no act | front.c: the anchor is `Group`, not `act`, and R is `d`, not `(act g d)` | `orbit-no-act`: got `ok` | KILLED |
| E1 scrutinee test admits | check.c `rule_elim`: `if (0 * !val_is(st, OP_SUM) && ...` | `elim-scrutinee`: got `ok`; GAP `elim-path-k` (TYPE_INFER: the case type is a Pi) | KILLED |
| E1b E1 + a K case type | E1 and check.c `elim_case`: a VAL_PATHP scrutinee gets the one case type `P refl` (the `rule_refl` core) | `elim-path-k`: got `ok` | KILLED |
| E2 case count from the call | check.c `rule_elim`: `uint32_t want = k->argc - 2u;` | `elim-case-count`: got `ok` | KILLED |
| E3 motive domain admits | check.c `rule_elim`: `\|\| 0 * !conv_values(c->m, c->level, pt->dom, st))` | `elim-motive-dom`: got `ok` | KILLED |
| E4 inl case inferred | check.c `rule_elim`: case 0 is `infer(c, k->args[1], &pt)` | `elim-case-type`: got `ok` | KILLED |
| F1 fold fills a missing case | check.c `rule_fold`, `fold_family`: the count test is `<=`, and a missing last case is the first case | `orbit-fold-no-set`: got `ok` | KILLED |
| P1 PATH_END passes | check.c `end_fail`: `return 1 ? 1 : FAIL(c, "PATH_END", ...` | `orbit-boundary`, `unit-no-eta`: got `ok`; GAP `orbit-fake-set` (PATH_UIP, R9) | KILLED |
| P1b P1 + PATH_UIP passes | P1 and `return 1 ? 1 : FAIL(c, "PATH_UIP", ...` | `orbit-fake-set`: got `ok` | KILLED |
| N1 the domain gains Prop | domain.lang: `def Prop : Type 1 := Type 0` | GAP `elim-motive-prop` (REFUSE_NAME: Prop is a reserved name, R4) | SURVIVED |
| N1b N1 + RESERVED without Prop | N1 (by hand) and check.c `RESERVED[] = {"Bool", "Text", "SProp"}` | `elim-motive-prop`: got `ok` | KILLED |
| L1 elim on a formal hcomp reduces | eval.c `reduce_elim`: the hcomp line reduces elim on the hcomp base | GAP `orbit-word` (the same REFUSE_LOWER) | SURVIVED (equivalent, L1 ruling) |
| L1r probe | eval.c `reduce_elim`: the hcomp line returns `internal(m, "L1r")` | `orbit-word`: the same REFUSE_LOWER, no INTERNAL | the line is not reached |

Score: 12 of 14 mutants killed, 13 admissions. N1 meets a second refusal
class (the RESERVED list, R4), and N1b, which removes both, is killed.
E1b and P1b close the run 1 GAPs of E1 and P1.

L1 ruling (session 5). `blocked` (src/front/eval.c) makes elim on a
neutral scrutinee a stuck elim before the formal hcomp test of
`reduce_elim`. The fold of a formal hcomp is an `hcomp_stuck` value
(VAL_STUCK), so it never gets to that test. The test meets only a VAL_OP
formal hcomp, which is a value of a type with path constructors, and
elim refuses those types (O21). A Sum or Unit scrutinee is never a
formal hcomp. The probe L1r keeps the same REFUSE_LOWER on `orbit-word`,
so the row does not reach the line. The `orbit-word` refusal is the K4c
refusal of a neutral elim at a word position (src/lower.c:645); the
lowerer has no elim case. L1 is an equivalent mutant for this row, and
no lowerer admission mutant is added (one site does not admit the row).
Item 4 of the S4 build report ("orbit-word needs it") was wrong. PLAN
section 9 O22 has the new wording.

Deviations from the S4 build brief (9):

1. `elim-motive-dom` is new (session 4): the motive domain test had no
   row.
2. `elim-motive-prop`: the case `unit` is now `Unit`. The row is well
   typed when a sort Prop exists, so only the name refuses it.
3. G1 needs three refusal classes: `kernel_sig`, the R4 test of
   `name_free` and the K6 defined-name test. G0 alone changes REFUSE_SIG
   to REFUSE_NAME (a diagnostic kill).
4. O1 needs two sites: the anchor (`Group`, not `act`) and R with no
   `act`.
5. E1b, P1b and N1b have two sites each. N1b edits two files:
   domain.lang by hand and check.c by `mut()`.
6. The L1 ruling and the O22 wording (above).
7. isSet Choice is proved by "sets from loops" (J on q reduces isSet A
   to: each loop is refl), with a normalising map for each elim case
   (normU, normL, normR), not encode-decode or Hedberg. J on q never
   meets an inl and inr pair, so no proof needs disjointness. `disjoint`
   (either into Type 0) is in the example and checks. Sanity mutants by
   hand (scratch): loopL with `inl a` gives PATH_UIP (R9); the agreeAct
   inr case refl gives PATH_END.
8. `orbit-fold-no-set` folds into Choice (the probe used Trunc0).
9. `orbit-word`: a fold over Quot in an inferred position gives
   TYPE_INFER, so the fold is the typed definition `agree`. With a
   constant point case the residual reduced away (build exit 0), so the
   point case is `agree0` and the hcomp base reads the state. The row
   uses `elim`, not `either`, at the word position. No new lowerer site.

S4 adds one binder (the motive of `elim`) and no universe. The
Prop-like rows are `elim-motive-prop` (a motive into Prop),
`elim-path-k` (no K) and `unit-no-eta` (no Unit eta, no strict
proposition).

Builder deaths. In S4 session 1 the wf-builder stage died before any
file change, and the opus fallback died too:

| Session | Fable (429, rate limit) | opus fallback (`reasoning_extraction`) |
|---|---|---|
| 1 | req_011Cg7azZME7M6BDdZ3ZeN1f | req_011Cg7b3sWCmbHpgS8pVgsEz |

Sessions 2 to 5 (PARTS A to C, the mutant runs, this section and the
gate) did not call a builder. The main loop did the work by hand.

## S5

Slice S5 (Glue, ua, uaBeta and hcomp at U, the coin-lang merge, PLAN
section 8) is complete on commit 9e28399 plus the S5 changes. The port
takes the coin-lang commits 44c917f (Fin and the equivalences), 91d7091
(Glue, ua and uaBeta) and 34acff5 (hcomp at Glue and at U) in order,
with `git apply --3way --index`, then by hand. Glue, glue and unglue
are formers with VIS_GUEST (F17). ua, uaArrow, isContr, fiber, isEquiv,
Equiv, idEquiv, isoCap, isoPath and isoToEquiv are domain definitions
(ORIGIN_DOMAIN, domain/domain.lang). Fin, finAdd and finNeg are
VIS_INTERNAL (O23). The gate `make -B check` is GREEN, exit 0, with the
clang `-Werror` check (log `orbit-lang-S5-close-gate.log`).

Rulings. The S5 builder call (PLAN section 8, 2026-10-17) gives the
port order, the guest forms and the refusal rows. O22 holds with hcomp
at U (PLAN section 9): hcomp at U gives a Glue type, hcomp at a Glue
type gives a glue element, and unglue of a glue element over a Sum base
reduces by the hcomp of data, so `blocked` sees a constructor or a
stuck value before the formal hcomp test of `reduce_elim`. The eval row
`orbit gElim` pins it. O23: `Fin n` takes `n : Nat` and its elements
are Nat literals, which guest code cannot write (O3 (a)), so Fin is
internal and its rows run under `--kit`. FORMERS gives Fin as a kit
note, not as F18, and SPEC says F1 to F17.

| Count | S4 (9e28399) | S5 | Reason |
|---|---|---|---|
| examples | 26 | 29 | `equiv` (Fin and the equivalences), `glue` (Glue, ua and uaBeta), `univ` (hcomp at U), all kit |
| check refusals | 132 | 147 | B1 3 Fin rows, B2 7 Glue and ua rows, B3 `hcomp-universe` out (hcomp at Type 0 is now a Glue type) and `hcomp-universe-cap`, `hcomp-higher-universe` in, B4 4 rows (`ua-non-equiv` and 3 Prop-like rows) |
| evals | 65 | 75 | `equiv addMod => 1`, `rot2 => 0`, `glue uaAt9 => 1`, `rotTwice => 2`, `univ transTwo => 0`, `circle loopAt0 => 1`, `loop2At0 => 2`, `trunc truncAt => trunc1 (Fin 3) 1`, `orbit flipAt => inr unit`, `gElim => inr unit` (O22) |
| lower refusals | 15 | 16 | `glue-word` (REFUSE_LOWER) |
| build checks | 32 | 33 | one check for each lower refusal (`glue-word`) |
| path regression rows | 26 | 36 | `test/pathtool.c`: B1 3, B2 4, B3 3 (the hand port of the coin-lang `test/path-regressions.c`) |

The other counts do not change: parse refusals 19, kit-only refusals 22,
runs 5, run refusals 10, asmtool 8 of 8, builds 3, dispatch 20, evm
steps 43, precompile steps 18, mock 13 rows, diff 15 rows, emit 24, and
the review, domain, fuel, output, lower and hashing regressions.

Rows per mode. The Fin rows, the Glue rows on `Fin 3` and the examples
`equiv`, `glue` and `univ` use Nat literals, so they are kit rows. The
guest rows use no kit name and no literal. The lower rows run under
`--kit` (S3 deviation 7).

| Gate | Kit rows | Guest rows |
|---|---|---|
| examples (`examples/kit.txt`) | 27 | 2 (`signature`, `orbit`) |
| check refusals (`test/check/kit.txt`) | 89 | 58 (52 S4 rows and 6 S5 rows) |
| parse refusals (`test/parse/kit.txt`) | 17 | 2 |
| evals | 69 | 6 (`signature flipped` and the 5 `orbit` rows) |
| lower refusals | 16 | none |

S5 rows. The Mutant column names the S5 mutant that admits the row. An
older site has no S5 mutant.

| Row | Code | Mode | Form | Mutant |
|---|---|---|---|---|
| `fin-range` | FIN_RANGE | kit | the literal 3 as a value of `Fin 3` | M1 |
| `fin-all-case` | FIN_CASE | kit | a function on `Fin 3` whose case 1 fails (PATH_END) | M2 |
| `fin-case-stuck` | FIN_CASE | kit | a function on `Fin 2` whose case 1 fails (PATH_END) | M2 |
| `glue-equiv` | GLUE_EQUIV | kit | a Glue branch for `~i` whose `e` is not an `Equiv T (Fin 3)` | M5 (a diagnostic kill) |
| `glue-cover` | SYSTEM_COVER | kit | a Glue system whose keys join to `i`, not `i \/ ~i` | none (M3 SURVIVED) |
| `glue-boundary` | GLUE_BOUNDARY | kit | glue where unglue of the branch for `~i` is 0 and the base is 1 | M6 |
| `unglue-type` | UNGLUE_TYPE | kit | unglue of a term of type Nat | M7 |
| `glue-binder` | GLUE_BINDER | kit | a Glue tube binder `j` that a branch uses | M4 |
| `ua-not-refl` | PATH_UIP | kit | refl between `ua e` and refl (a path of paths) | older site |
| `ua-beta-neutral` | PATH_END | guest | uaBeta on a neutral equivalence: transp along `ua e` stays an hcomp, not `(witness e) x` | older site |
| `hcomp-universe-cap` | HCOMP_CAP | kit | hcomp at Type 0 with the base Nat and the branch for `~j` Fin 3 at i0 | older site |
| `hcomp-higher-universe` | HCOMP_UNIVERSE | guest | hcomp at Type 1 | M8 |
| `ua-non-equiv` | TYPE_MISMATCH | guest | ua of a function, not of an equivalence | older site |
| `glue-prop` | NAME_UNKNOWN | guest | a Glue at `Prop` (no sort Prop, R11) | older site |
| `isequiv-two-proofs` | PATH_END | guest | refl between two isEquiv proofs `p` and `q` of one map | older site |
| `ua-two-proofs` | PATH_UIP | guest | refl between the ua of one map with two equivalence proofs | older site |
| `glue-word` (lower) | REFUSE_LOWER | kit build | `glueWord`: hcomp at a word position (K4c) | none (no new lower.c site) |

Mutants, one at a time (runner `orbit-lang-work/S5-mutants.sh`, log
`orbit-lang-work/S5-mutants.log`). `row()`, `mut()`, `admit()` and
`lowadmit()` are the S4 helpers verbatim, and `krow()`, `kadmit()` and
`erow()` are new (kit rows and eval rows). A pristine pass comes first.
After the loop, `src/front/check.c` equals every save and
`src/front/eval.c` equals the M9 save (cmp), and `make -B check` is
GREEN (log `orbit-lang-S5-mutants-gate.log`).

| Mutant | Edit | Killer | Verdict |
|---|---|---|---|
| M1 FIN_RANGE admits | check.c `elab_inner` TERM_NAT: `if (0 * (t->nat >= hint->args[0]->nat))` | `fin-range`: got `ok` | KILLED |
| M2 FIN_CASE takes case 0 | check.c `finAll`: a failed case `j` takes the core of case 0 (`args[j + 1u] = args[1u];`) | `fin-all-case`, `fin-case-stuck`: got `ok` | KILLED |
| M3 SYSTEM_COVER admits | check.c, the Glue cover test: `if (0 * !conv_values(c->m, c->level, join, pf))` | GAP `glue-cover` (PATH_END: at i0 the type `Glue (Fin 3) i1 [ ]` is not Fin 3, as S4 N1) | SURVIVED |
| M4 GLUE_BINDER admits | check.c `glue_tube`: `if (0 * (tube->name != NULL) && strcmp(tube->name, "_") != 0)` | `glue-binder`: got `ok` | KILLED |
| M5 GLUE_EQUIV keeps the inner failure | check.c `glue_branch`: `if (1 \|\| kept_failure(code))` | `glue-equiv`: want GLUE_EQUIV, got TYPE_MISMATCH (a diagnostic kill, as S4 G0) | KILLED |
| M6 GLUE_BOUNDARY admits | check.c `glue_boundary`: `if (0 * !conv_values(c->m, c->level, vu, va))` | `glue-boundary`: got `ok` | KILLED |
| M7 UNGLUE_TYPE admits | check.c `rule_unglue`: unglue of a term of another type is the term (`{ k->type = gt; return g; }`) | `unglue-type`: got `ok` | KILLED |
| M8 HCOMP_UNIVERSE at n > 1 | check.c `rule_hcomp`: `av->nat != 0u` becomes `av->nat > 1u` | `hcomp-higher-universe`: got `ok` | KILLED |
| M9 glue_at fallback reverts | eval.c `glue_at`: `return internal(m, "a line that changes its type former");` | `circle loopAt0`, `loop2At0`: INTERNAL, not 1 and 2 | KILLED |

Score: 8 of 9 mutants killed, 7 admissions, one diagnostic kill (M5)
and two eval differences (M9). M3 meets a second refusal class: with
the cover test gone, `glue-cover` refuses with PATH_END at i0. M5 has no
simple admission: each change that admits the branch leaves no valid
branch core, so the kill is the changed code. Open item M3b (for the
USER): a cover row with no path end, for example a Glue type at a
neutral `i` in a def of type Type 0. S5 does not add it.

Deviations from the S5 builder call (8):

1. The port uses `git apply --3way --index` and hand ports, not
   `cherry-pick -n`. orbit-lang had no coin-lang objects, so session 2
   ran `git -C ~/Documents/orbit-lang fetch --no-tags
   ~/Documents/coin-lang HEAD` (objects and FETCH_HEAD only, no ref).
2. Not ported: the src/json.c hunks, test/json (`equiv.json`,
   `glue.json`, `univ.json`), the test/emit rows `fin-case-term` and
   `fin-case-type` and the `json_contains` rows. orbit-lang has no JSON
   or emit front for them.
3. Not ported: the coin-lang Makefile hunk (it links $(TARGET) into
   build/path-regressions, a target that orbit-lang does not have) and
   the coin-lang docs (PLAN, STATUS, host README).
4. test/path-regressions.c goes by hand into test/pathtool.c
   (`print_refused`, `accepts` and `system_conversion` with the prelude
   argument, the static EQUIV prelude, 10 rows).
5. `ua-two-proofs` gives PATH_UIP (a path of paths), not the PATH_END
   of the builder call. The refusal is correct.
6. A kernel fix that is not in the three commits: `loopT` gave INTERNAL
   at eval.c `glue_at`, because in orbit-lang `loop i0` reduces to
   `base` before the fold, so the end of `code2` has no GlueRec. The fix
   comes from coin-lang 8c79e9a (S4a): `ival_subst` splits into
   `ival_replace` and `face_norm`, `env_isubst` and `val_isubst` are new
   (with `case VAL_WORD:`), and `glue_at` falls back to the line at a
   fresh variable. The new code runs only where `glue_at` returned
   INTERNAL. M9 pins it.
7. B5: Fin is a kit note in FORMERS, not F18 (VIS_INTERNAL, O23), so
   SPEC says F1 to F17. formers/tcc-evm-contract.md also names F17, and
   its stale lower row count 13 is now 16.
8. The row `hcomp-universe` goes out of the check rows and kit.txt (B3,
   as the builder call says), and `hcomp-universe-cap` keeps a HCOMP_CAP
   row at Type 0.

S5 adds one binder form (the Glue tube `fun (_ : I) =>`, which no
branch can use) and no universe. The Prop-like rows are `glue-prop` (a
Glue at Prop), `isequiv-two-proofs` and `ua-two-proofs` (no proof
irrelevance for isContr or isEquiv).

Builder deaths: none. S5 called no builder (the USER route of S2 to
S4). The main loop did the work by hand: B1 to B5 in sessions 3 to 5,
the mutant run in session 5, and this section and the gate in session
6.

## S6

Slice S6 (HIITs and QIITs, PLAN section 8) is complete on commit 8dd82a3
plus the B4 changes. S6 is new work in this lineage (O2 (a));
zkpolicy-lang a1780a9 and f176ac6 are a design reference only. USER
commit 6fa6a15 holds B1 (the keyword `and`, the index telescope, the
constructor result and the block in the parser and the AST) and B2 (the
indices of FamilyInfo, the result index of CtorInfo, HIT_INDEX,
BLOCK_ORDER, Size and Bag) with the USER B2 safety fixes. USER commit
8dd82a3 holds B3 (indexed and block folds in check and eval, Charter and
Ratified, `examples/hiit.lang` and `examples/qiit.lang`, 5 eval rows)
with the USER fixes. B4 adds 6 rows, the mutant run (M3b and 8 S6
mutants) and the records. Size, Bag, Charter and Ratified are domain
families (ORIGIN_DOMAIN, `domain/domain.lang:50-78`), so guest code can
use them. Nat stays VIS_INTERNAL. The gate `make -B check` is GREEN, exit
0, with the clang `-Werror` check (log `orbit-lang-S6-B4-gate.log`).

Rulings. The S6 builder call (PLAN section 8, 2026-10-17) gives the
surface, the two refusal codes, the fold forms and the slices. The USER
ruling S6: Bag is indexed by the domain family Size (Nat stays
internal); O21 and `Orbit ≃ Bag n` move to S7; the M3b mutant is in S6.
FORMERS gives the indexed family with path constructors as F18 and the
block as F19, so SPEC says F1 to F19.

| Count | S5 (a12973a) | S6 | Reason |
|---|---|---|---|
| examples | 29 | 31 | `hiit` (Bag and its fold), `qiit` (Charter, Ratified and the block fold), both guest |
| check refusals | 147 | 150 | B3 `bag-no-squash`, B4 `block-fold-target` and `block-fold-no-squash`, all TYPE_MISMATCH |
| evals | 75 | 80 | B3 `hiit bagTwoCount`, `bagSwapCount`, `qiit adoptedZero`, `adoptedTwo`, `adoptedLaw` |

The other counts do not change: parse refusals 19, kit-only refusals 22,
runs 5, run refusals 10, asmtool 8 of 8, builds 3, lower refusals 16,
build checks 33, dispatch 20, evm steps 43, precompile steps 18, mock 13
rows, diff 15 rows, emit 24, and the review, domain, path, fuel, output,
lower and hashing regressions. Only `domain/domain.lang` can declare a
family (a program family is REFUSE_DATA), so the HIT_INDEX and
BLOCK_ORDER rows are fronttool `domain` rows (a domain override,
front.c:67), and the domain regressions run them.

Rows per mode. The S6 check rows and examples use no kit name and no
literal, so they are guest rows.

| Gate | Kit rows | Guest rows |
|---|---|---|
| examples (`examples/kit.txt`) | 27 | 4 (`signature`, `orbit`, `hiit`, `qiit`) |
| check refusals (`test/check/kit.txt`) | 89 | 61 (58 S5 rows and 3 S6 rows) |
| parse refusals (`test/parse/kit.txt`) | 17 | 2 |
| evals | 69 | 11 (6 S5 rows and the 5 S6 rows) |
| lower refusals | 16 | none |

S6 rows. The Mutant column names the S6 mutant that admits the row or
changes its result. An older site has no S6 mutant.

| Row | Code | Mode | Form | Mutant |
|---|---|---|---|---|
| `bag-no-squash` | TYPE_MISMATCH | guest | a Bag fold with no `bagSquash` case (the missing coherence case, at the `rule_fold` case count) | none (older site) |
| `block-fold-target` | TYPE_MISMATCH | guest | a Charter block fold with one target type, not two | none (FA leaves it TYPE_MISMATCH at a later site) |
| `block-fold-no-squash` | TYPE_MISMATCH | guest | a Charter block fold with no `ratifiedSquash` case | none (as `block-fold-target`) |
| fronttool `hiit-wrong-index` | HIT_INDEX | domain | an end of the path constructor `vbad` is the family at another index | none in the S6 run |
| fronttool `hiit-no-result` | HIT_INDEX | domain | a constructor of an indexed family states no result | HI1 |
| fronttool `hiit-wrong-head` | HIT_INDEX | domain | the result of `vnil` is `Sz`, not the family `V` (B4) | HI2 |
| fronttool `block-order` | BLOCK_ORDER | domain | the header of `P0` names the later sort `Q0` | BO, BOb (code kills) |
| fronttool `index-prop` | NAME_UNKNOWN | domain | `Prop` in an index telescope (rule R4) | none (older site) |
| fronttool `block-ok` | ok | domain | a field names a later sort of its block | none |
| fronttool Even/Odd `one` | ok | domain | a block fold with the targets Nat and Unit (B4) | SR |
| fronttool Even/Odd `two` | ok | domain | a case that uses the folded sibling field (B4) | FS |
| fronttool `block-fold-extra` | TYPE_MISMATCH | domain | a block fold with an extra argument before the value (B4) | FA |
| fronttool USER rows (6fa6a15, 8dd82a3) | TYPE_MISMATCH, HIT_BOUNDARY, REFUSE_STATE, ok | domain | index arguments in point and path cases, the fibre of a transport result, the print order of block targets, a sibling recursive field at its target (Box), a path field over a sibling (L/R), the parameter telescope (2 rows), reserved constructor signatures in a header fold, a block universe, a state with an index | SR, FS (Box, L/R) |
| `glue-cover` (S5) | SYSTEM_COVER | kit | a Glue system that does not cover its face (the S5 M3 row) | M3b |

Mutants, one at a time (runner `orbit-lang-work/S6-mutants.sh`, log
`orbit-lang-work/S6-mutants.log` for run 2, log
`orbit-lang-work/S6-mutants.run1.log` for run 1). A pristine pass comes
first. After the loop, `src/front/check.c` equals every save and
`src/front/eval.c` equals the FS save, and a final `make -B` build exits
0.

| Mutant | Edit | Killer | Verdict |
|---|---|---|---|
| M3b SYSTEM_COVER and PATH_END admit | check.c `end_fail`: PATH_END returns 1, and the Glue cover test `0 *` | kit row `glue-cover`: got `ok` | KILLED |
| HI1 HIT_INDEX admits no result | check.c `ctor_result`: no result returns 1 | fronttool `hiit-no-result`: got `ok` | KILLED |
| HI2 HIT_INDEX admits a wrong head | check.c `ctor_result`: the head test `0 *` | fronttool `hiit-wrong-head`: got `ok` | KILLED |
| BO BLOCK_ORDER admits | check.c `header_order`: `if (0 * named)` | fronttool `block-order`: got REFUSE_REC | KILLED (code) |
| BOb BO and the R3 loop | BO, and the `unknown_name` R3 loop starts at the end | fronttool `block-order`: got NAME_UNKNOWN | KILLED (code) |
| FA block fold arity admits | check.c `fold_block`: the arity test `0 *` | run 1 SURVIVED (both block rows and `bag-no-squash` stay TYPE_MISMATCH at a later site); run 2 fronttool `block-fold-extra`: got `ok` | KILLED |
| SR sort_res gives the result | check.c `sort_res` returns `res` | fronttool Even/Odd `one` and the USER Box row: TYPE_MISMATCH | KILLED |
| CF case_fields admits a recursive index | check.c `case_fields`: `&& !field->recursive` removed | `examples/qiit.lang`: HIT_BOUNDARY at `adoptRule` | KILLED |
| FS field_sort gives the family | eval.c `field_sort` returns `family` | `qiit adoptedTwo`, `adoptedLaw`: INTERNAL; fronttool Even/Odd, Box, L/R: INTERNAL | KILLED |

Score: 9 of 9 mutants killed (run 2). M3b, HI1, HI2 and FA are
admissions. SR and CF refuse a valid program (TYPE_MISMATCH,
HIT_BOUNDARY). FS gives INTERNAL in eval. BO and BOb are code kills: a
header cannot resolve a later sort of its block, so with BO the row meets
REFUSE_REC (rule R3), and with BOb it meets NAME_UNKNOWN. BLOCK_ORDER is
the first refusal of that chain and has no admission mutant, so the kill
is the changed code. Run 1 had 8 mutants, and FA SURVIVED: both block
rows and `bag-no-squash` fail with the same code at a later site. B4
added the row `block-fold-extra`, and run 2 killed FA. M3b closes the S5
open item: with both refusals of the S5 M3 row gone, `glue-cover` is
admitted. SR is equivalent on `qiit` (both targets are Trunc0 Size); the
row Even/Odd `one` has two different targets. The FS killer that the
prompt named (`qiit adoptedTwo`) kills FS: the fold of the `ratify` field
at Charter is INTERNAL, although the case does not use it. Open: the
`hiit-wrong-index` site (an end at another index) has no mutant in the
S6 run.

Deviations from the S6 builder call (6):

1. The fold targets are constant types, as PLAN "Fold" says. The B3
   prompt said a family over the index; PLAN wins.
2. Transport at an indexed or block sort stays neutral for every line
   (USER B2 safety, PLAN section 8). This is stronger than O25 and the
   PLAN "Eval" line, which keep it neutral only along an index line that
   is not constant. `transp_family` does not change.
3. The USER fixes in 8dd82a3 (+54 -8 on the B3 tree 35c2e2cb):
   `fold_block` checks that the parameters of the scrutinee inhabit the
   telescope of every block sort (a PLAN "Fold" line); eval `field_sort`
   reads the type under a path field and `fold_path_field` folds at
   `field_sort`, which closes B3 deviation 3 (a path field of a sibling
   sort folded at the constructor sort); `print_value` prints the block
   targets first; 7 fronttool rows.
4. The USER fixes in 6fa6a15 (B2 safety, PLAN section 8): `case_image`,
   `path_case_type` and `fold_family` pass the parameters only;
   `rule_fold` refused block folds until B3; `names_var` respects
   binders; block universes go to a fixed point; `check_state` refuses an
   index; eval `transp_data` keeps an indexed or block sort neutral.
5. `bag-no-squash` is refused at the `rule_fold` fallback (the case count
   is not the constructor count plus 1), not in `fold_family`. The code
   is TYPE_MISMATCH, as PLAN says.
6. PLAN B4 names FORMERS, SPEC and `formers/tcc-evm-contract.md`. The
   records also update the FORMERS section 5 matrix (an S6 row), the
   `family` line of `docs/host/README.md`, and the FORMERS F17 and S5
   citations of the equivalences, which S6 moved from
   `domain/domain.lang:50-113` to lines 80 to 142.

S6 adds one binder form (the index telescope) and one keyword (`and`),
and no universe. The Prop-like rows are `index-prop` (Prop in an index
telescope is NAME_UNKNOWN) and the squash cases: a sort without a squash
constructor is not a set by conversion (PATH_UIP), and a fold that omits
a squash case is TYPE_MISMATCH.

Builder deaths: none. S6 called no builder (the USER route). The main
loop did the work by hand in sessions 1 to 6
(`orbit-lang-work/S6-HANDOFF.md`): B3 in session 4, the B4 rows and the
mutant runs in session 5, and this section and the gate in session 6.

## S7

Slice S7 (the DAO prelude and laws 1 to 4, PLAN section 8) is complete on
commit 9c7b033 plus the B4 records. S7 is new work in this lineage (O2
(a)). USER commit 96c74e1 holds the S7 PLAN text (the builder call and the
USER ruling S7). USER commit 676c7a8 holds B1 (elim on a domain family
with path constructors: check.c `rule_elim` and `elim_on_family`, eval.c
`elim_family` and `stuck_elim`, `examples/elim.lang`, 5 check rows and 8
eval rows) with the USER fixes. USER commit eafa1df holds B2 (the DAO
prelude, the slots `outcomeSet` and `menu`, the first part of
`examples/dao.lang`, 4 check rows and 4 eval rows). USER commit ac157d0
holds B3 part A (laws 1 to 4 and the Charter history fold in
`examples/dao.lang`, the PLAN rows `constitution-noninvariant` and
`gov-unit-wrong`, the eval row `dao historyOne`). USER commit 9c7b033 holds
B3 part B (the domain families Pos and Word, `domain/domain.lang:64-75`,
and `examples/bagorbit.lang`). B4 adds the mutant run (9 mutants) and the
records, and no source change. The gate `make -B check` is GREEN, exit 0,
with the clang `-Werror` check (log `orbit-lang-S7-s10-gate.log`).

Rulings. The S7 builder call (PLAN section 8, 2026-10-18) gives the
eliminator, the prelude, the laws, the menu, the rows and the slices. The
USER ruling S7: Q1 full elim on families (block sorts stay outside elim);
Q2 the proof slot `outcomeSet`; Q3 the slot `menu` after the prelude; Q4
general n (`bagOrbit : (n : Size) -> Equiv (OrbitN n) (Bag Choice n)`, with
the domain family Pos over Size); Q5 Group as the words of adjacent swaps
(the domain family Word over Size). S7 adds no former: F16 now covers a
domain family with path constructors, so SPEC still says F1 to F19.

| Count | S6 (8b11846) | S7 | Reason |
|---|---|---|---|
| examples | 31 | 34 | `elim` (kit, Nat literals), `dao` and `bagorbit` (guest) |
| check refusals | 150 | 161 | B1 `elim-family-count`, `elim-motive-index`, `elim-block` (TYPE_MISMATCH), `elim-path-case` (PATH_END), `elim-family-prop` (NAME_UNKNOWN); B2 `outcome-set-type`, `menu-type`, `dao-redefine` (REFUSE_SIG), `dao-no-set` (NAME_UNKNOWN); B3 `constitution-noninvariant`, `gov-unit-wrong` (PATH_END) |
| evals | 80 | 93 | B1 `elim bagTwoCountE`, `replicateTwo`, `swapAt0`, `swapAt1`, `bagSwapE`, `quotThree`, `quotEqAt`, `hcompE`; B2 `dao tallied`, `flippedTally`, `nextRule`, `amended`; B3 `dao historyOne` |

The other counts do not change: parse refusals 19, kit-only refusals 22,
runs 5, run refusals 10, asmtool 8 of 8, builds 3, lower refusals 16,
build checks 33, dispatch 20, evm steps 43, precompile steps 18, mock 13
rows, diff 15 rows, emit 24, and the review, domain, path, fuel, output,
lower and hashing regressions. The USER fixes in 676c7a8 add fronttool
rows to the domain regressions.

Rows per mode. `examples/elim.lang` uses Nat literals, so it and its 8
eval rows are kit rows, and `elim-path-case` (motive `Trunc0 Nat`) is a
kit row. The other S7 rows and examples are guest rows.

| Gate | Kit rows | Guest rows |
|---|---|---|
| examples (`examples/kit.txt`) | 28 (27 S6 rows and `elim`) | 6 (`signature`, `orbit`, `hiit`, `qiit`, `dao`, `bagorbit`) |
| check refusals (`test/check/kit.txt`) | 90 (89 S6 rows and `elim-path-case`) | 71 (61 S6 rows and 10 S7 rows) |
| parse refusals (`test/parse/kit.txt`) | 17 | 2 |
| evals | 77 (69 S6 rows and the 8 `elim` rows) | 16 (11 S6 rows and the 5 `dao` rows) |
| lower refusals | 16 | none |

S7 rows. The Mutant column names the S7 mutant that kills the row, or
changes the result of its example. An older site has no S7 mutant.

| Row | Code | Mode | Form | Mutant |
|---|---|---|---|---|
| `elim-family-count` | TYPE_MISMATCH | guest | elim on a family with a third case at the scrutinee | EC (a code kill) |
| `elim-path-case` | PATH_END | kit | a path case at a constant line (motive `Trunc0 Nat`, so the other cases are valid) | none (the path end check fires first) |
| `elim-motive-index` | TYPE_MISMATCH | guest | a motive without the index binder | none |
| `elim-block` | TYPE_MISMATCH | guest | elim on the block sort `Charter Size` | none (older site) |
| `elim-family-prop` | NAME_UNKNOWN | guest | `Prop` in the motive telescope (rule R11) | none (older site) |
| `outcome-set-type` | REFUSE_SIG | guest | `outcomeSet` at a type that is not `isSet Outcome` | OS (a code kill) |
| `menu-type` | REFUSE_SIG | guest | `menu` at a type that is not `Outcome -> Constitution` | MN |
| `dao-redefine` | REFUSE_SIG | guest | a program `def aggregate` after the slots | PS (a code kill) |
| `dao-no-set` | NAME_UNKNOWN | guest | a program without `outcomeSet` has no `aggregate` | none |
| `constitution-noninvariant` | PATH_END | guest | refl as the invariance proof of the choice of member `inl unit` under the flip act (`Outcome := Trunc0 Choice`) | none (older site, the refl end) |
| `gov-unit-wrong` | PATH_END | guest | refl at the unit law of `keepCon` for a G that is the choice of member `inl unit` | none (older site, the refl end) |
| `elim` eval rows (8) | value | kit | elim on bagCons, on swap at i0, i1 and a neutral point, on quot, on quotEq at a neutral point, and on a formal hcomp (neutral, O22) | EV (`bagSwapE`, `quotEqAt`) |
| `dao` eval rows (5) | value | guest | the tally, the flipped tally, the next rule, the amended rule and the Charter history | PJ (the example) |
| `examples/dao.lang`, `examples/bagorbit.lang` | ok | guest | laws 1 to 4, the menu, the history fold; `bagOrbit` | EP, ES, PJ |
| fronttool `hiit-wrong-index` (S6) | HIT_INDEX | domain | an end of the path constructor `vbad` is the family at another index | HIW (a code kill) |

Mutants, one at a time (runner `orbit-lang-work/S7-B4-mut.sh`, log
`orbit-lang-work/S7-B4-mutants.log`). Each mutant is one line, made and
reverted by the Edit tool, with `make -B` for each build. The runner stops
unless `src` differs from HEAD by exactly one line. After each revert,
`git diff --quiet -- src` exits 0. The first EV row of the log is VOID (it
ran eval without `--kit`); the second EV row is the valid one. After the
loop, the gate on clean source exits 0 (log `orbit-lang-S7-s10-gate-pre.log`).

| Mutant | Edit | Killer | Verdict |
|---|---|---|---|
| EC | check.c `elim_on_family` (called from `rule_elim`): `uint32_t want = 0 * f->ctor_count;` | `elim-family-count`: TYPE_NOT_FUNCTION at `bad` ("a term of type Size is applied to an argument"); the row wants TYPE_MISMATCH | KILLED (code) |
| EP | check.c `path_case_type`: the line of the path case at `val_iend(m, 0)` (a constant line) | `examples/dao.lang`: TYPE_MISMATCH at `orbitExt` (the elim lemma under `desc`); `examples/bagorbit.lang`: TYPE_MISMATCH at `quotExt` | KILLED |
| ES | check.c `square_type`: the inner line at `val_iend(m, 0), val_iend(m, 0)` | `examples/bagorbit.lang`: TYPE_MISMATCH at `quotExt` (the quotSquash case) | KILLED |
| EV | eval.c `elim_family`: a path constructor value goes to `stuck_elim` | eval rows `elim bagSwapE` and `elim quotEqAt`: a stuck elim | KILLED |
| PJ | front.c `&& (1 \|\| splice_after(... DAO_AFTER ...))` | `examples/dao.lang`: NAME_UNKNOWN at `agreeCon` (Constitution) | KILLED |
| PS | check.c `kernel_sig`: `return 1 \|\| FAIL(...)` at SIG_DEFINED | `dao-redefine`: REFUSE_NAME at `aggregate` (rule R4, a domain definition); the row wants REFUSE_SIG | KILLED (code) |
| OS | check.c `slot_ok`: `if (strcmp(row->name, "outcomeSet") == 0) return 1;` | `outcome-set-type`: TYPE_MISMATCH at `aggregate` (the prelude uses `outcomeSet` as `isSet Outcome`); the row wants REFUSE_SIG | KILLED (code) |
| MN | check.c `slot_ok`: `if (strcmp(row->name, "menu") == 0) return 1;` | `menu-type`: got `ok` | KILLED |
| HIW | check.c:3132 `if (0 * (bs[i].body == NULL))` | fronttool: "FAIL domain: wanted HIT_INDEX, got TYPE_MISMATCH" (`vbad`) | KILLED (code) |

Score: 9 of 9 mutants killed. MN is an admission. EP, ES and PJ refuse a
valid program (TYPE_MISMATCH, NAME_UNKNOWN). EV gives a stuck elim in
eval. EC, PS, OS and HIW are code kills: the row stays refused, with
another code at a later site. HIW confirms the survey H prediction and
closes the S6 open item: the `hiit-wrong-index` site now has a mutant.

The design of `examples/bagorbit.lang` (Q4, general n). `OrbitN n := Quot
(Pos n -> Choice) (swapR n)`. f is a Quot fold into `Bag Choice n`, and g
(`fromBag`) is elim on Bag with the motive `fun k y => OrbitN k`. The two
homotopies (`quotExt`, `bagExt` and the lemmas under them) take g as a
variable G with its bagNil and bagCons rules, and `fromBagW sw` takes the
swap case as a variable sw. Then `fromBag := fromBagW swapO`, and `bagOrbit
:= bagOrbitW swapO`, whose type does not name sw. The reason: conversion
has no fast path (eval.c `conv_loop`, eval.c:3194). It does not test
pointer equality, and it compares two closures by eta. Two `fromBag`
values are separate closures, so each comparison unfolds `swapO`, and the
check did not fit in the arena (`langc: OOM`). With the variables, `langc
check examples/bagorbit.lang` takes 0.14 s. The example row at the end of
the file is a DAO program at n0: Member `Pos n0`, Group `Word n0`, act
`actW n0`, and `orbitRow : Path (Type 0) Orbit (OrbitN n0)` by refl (the
spliced Orbit converts to `OrbitN n0`). `examples/dao.lang` uses the same
shape: `orbitExt` takes the set proof `sO` and the second map M as
variables, and `desc` instantiates them.

Deviations from the S7 builder call (8):

1. EC, PS and OS were predicted `ok` (admissions), but they are code
   kills. With EC the third case is applied to the elim result
   (TYPE_NOT_FUNCTION). With PS the program `aggregate` meets REFUSE_NAME
   (rule R4) at the next site. With OS the prelude meets the wrong
   `outcomeSet` at `aggregate` (TYPE_MISMATCH).
2. The HIW site moved from check.c:2865 (the PLAN line) to check.c:3132:
   B1 and B2 added lines above it.
3. The EP killer in `examples/dao.lang` is `orbitExt`, not `desc`: `desc`
   is a path lambda over `orbitExt`, and the path case is in `orbitExt`.
4. EV is killed by 2 of the 4 eval rows that PLAN names. `swapAt0` and
   `swapAt1` do not change: at i0 and i1 the swap value is its boundary
   point before elim reaches it.
5. `elim-path-case` is PATH_END, not TYPE_MISMATCH: the path end check
   fires first, and PATH_END is the code of every path end refusal. It is
   a kit row, and `examples/elim.lang` is a kit example (Nat literals).
   PLAN said all S7 rows and examples are guest.
6. The USER fixes in 676c7a8 (on the B1 tree 573f7dec): `apply_known`
   takes the elim arity from the motive domain, `bare_motive` adds the
   annotations, and the fixes add fronttool rows. The prelude `gov` gives its proof as
   the path lambda `fun i => L (quotEq .. i)` (which is `cong L` by
   definition), not as `cong L ..`.
7. The prelude is an inline source (`kernel/dao.lang`, front.c:26), spliced
   by `splice_after` (the general form of `orbit_after_act`) after the
   last of `orbitProj` and `outcomeSet` (front.c:79-85). `orbitProj`
   follows `act`, so this is the PLAN position. `kernel_sig` refuses every
   SIG_DEFINED name (Orbit, orbitProj and the 7 prelude names), so no
   program declaration reaches the SIG_DEFINED branch of `check_sig`.
8. B3 part B added 12 lines to `domain/domain.lang` (Pos and Word at lines
   64 to 75). The records move the FORMERS citations: the equivalences
   from lines 80 to 142 to lines 92 to 154, and Charter and Ratified from
   lines 64 to 78 to lines 76 to 90.

S7 adds one binder form (the motive telescope over the indices and the
value) and no universe. The Prop-like row is `elim-family-prop` (Prop in
the motive telescope is NAME_UNKNOWN). Elim on a Path stays refused (no
K), and elim on a formal hcomp stays neutral (O22). O25 does not change:
the guest proofs do not transport along an index line.

Open items. O26 (S9): member permutation invariance of a menu entry, with
`Orbit ≃ Bag n` when Group is the member permutation group. O27: the full
permutation form of Group, `Equiv (Pos n) (Pos n)`, needs each
permutation split into swaps (decidable equality on Pos n), a later
slice. The S6 open item (`hiit-wrong-index` has no mutant) is closed by
HIW.

Builder deaths: none. S7 called no builder (the USER route). The main
loop did the work by hand in sessions 1 to 10b
(`orbit-lang-work/S7-HANDOFF.md`): B1 in sessions 2 to 5, B2 in session
6, B3 part A in sessions 7 and 8, B3 part B in sessions 9 and 9b, the
mutants in session 10, and this section and the gate in session 10b.

## S8

Slice S8 (the ZK layer, PLAN section 8) is complete on commit 81a9734
plus the B4 records. S8 is new work in this lineage (O2 (a)). USER commit
f662647 holds the S8 PLAN text (the builder call and the USER ruling S8).
USER commit c3ab788 holds B1 (BN254 G1 in C, `src/front/bn254.c` and
`bn254.h`: Fp and Fr with the modulus as a parameter, affine add and
double, a Jacobian scalar mul and the on-curve test; host `ecAdd` and
`ecMul` compute; `examples/bn254.lang`, 11 eval rows and the 2G words of
`test/precompile.sh`). USER commit 8bd87a7 holds B2a (the host bLSAG
prover and verifier with the static H_p table, `src/front/blsag.c` and
`blsag.h`, `test/blsagtool.c`, `examples/zk.lang` and 35 eval rows). USER
commit b61db98 holds B2b (the kernel private ballot and the witness
operations: the splice `kernel/zk.lang` in `src/front/front.c:50-55` with
`castPrivate`, `proveZK`, `project` and `verifyZK` over the internal
carriers `ZkBallot` and `ZkWitness`; `examples/ring3.lang`, 4 eval rows
and the check rows `zk-flip` and `zk-redefine`). USER commit adf9f45
holds B2c (`langc prove`, `src/main.c`, the prove rows and the prove
refusals in `test/gate.sh`). USER commit 92fb145 holds B3a (the `zkVerify`
lowering, `zk_unroll` in `src/front/eval.c`, `examples/zkcast.lang` with
the nullifier map, and the valid cast in `test/precompile.sh`). USER
commit 3fc7f50 holds B3b (the six refusal rows in `test/precompile.sh`),
and 81a9734 the `zkVerify` row of `docs/host/README.md`. B4 adds the
mutant run (8 mutants) and the records, and no source change. The gate
`make -B check` is GREEN, exit 0, with the clang `-Werror` check (log
`orbit-lang-S8-s14-gate.log`).

Rulings. The S8 builder call (PLAN section 8, 2026-10-18) gives the curve
port, the prover, the kernel names, the lowering, the gate rows and the
slices. The USER ruling S8: Q1 the C port of `fr.c` with the modulus as a
parameter, the host computes `ecAdd` and `ecMul`; Q2 the static H_p table;
Q3 the prover and `langc prove` in S8, the section 7 contract writer in
S9; Q4 the nullifier map keyed with the poll id. The USER ruling S8
session 3: Q5 poll id 0 in the host, `langc prove` takes the poll id as a
command argument; Q6 `verifyZK w c t` reads the choice from c (the
`zk-flip` row is PATH_END; the B4 records carry the signature into PLAN
section 6 and O11); B1 keeps the Jacobian scalar mul. S8 adds no former:
`kernel/zk.lang` adds types over F1 to F19, so SPEC still says F1 to F19.

| Count | S7 (049c04e) | S8 | Reason |
|---|---|---|---|
| examples | 34 | 38 | `bn254`, `zk` and `zkcast` (kit), `ring3` (guest) |
| check refusals | 161 | 163 | B2b `zk-flip` (PATH_END), `zk-redefine` (REFUSE_SIG) |
| evals | 93 | 143 | B1 11 rows (k G for k in 1, 2, 3, r minus 1 and r, the curve traps; the 2G words row changed), B2a 35 rows (H_p, the bLSAG vectors), B2b 4 rows (`zk`), B3a 1 row (`zkcast`) |
| proves | 0 | 2 | B2c `dao b0` and `ring3 b` (`test/prove/expect.txt`), with the prove refusals PROVE_BALLOT and EVAL_ARGS in `test/gate.sh` |
| precompile steps | 18 | 33 | B3a the valid cast, the count read and the reused key image; B3b the six refusal rows |

The other counts do not change: parse refusals 19, kit-only refusals 22,
runs 5, run refusals 10, asmtool 8 of 8, builds 3, lower refusals 16,
build checks 33, dispatch 20, evm steps 43, mock 13 rows, diff 15 rows,
emit 24, and the review, domain, path, fuel, output, lower and hashing
regressions. S8 adds the bLSAG regressions (`test/blsagtool.c`, B2a).

Mutants, one at a time (runner `orbit-lang-work/S8-B4-mut.sh`, log
`orbit-lang-work/S8-B4-mut.log`, one row per mutant with the killer
output). Each mutant is one site, made and reverted by the Edit tool,
with `make -B build/langc` for each build; NU is a guest edit
(`examples/zkcast.lang`) with no rebuild. The runner stops unless `src`
or `examples` differs from HEAD by exactly one site. After the loop,
`make -B build/langc` on clean source builds and `git diff` shows no
change. The host sites `blsag.c:140`, `141` and `258` (the poll id, the
choice and the final compare of the host chain) were not edited: the
kernel sites in `zk_unroll` were killed as recorded below. CC and PI
were killed at the valid cast, rather than at their named refusal rows;
this run does not establish mutation coverage of the host sites.

| Mutant | Edit | Killer | Verdict |
|---|---|---|---|
| FP | bn254.c:134 `fe_mul`: the conditional subtract is skipped on the last shift (`i > 1 &&`) | eval rows `twoG`, `threeG`, `rMinusOneG` and `rG`: wrong words; `negSum`: trap | KILLED |
| ON | bn254.c:197 `g1_on_curve`: `return 1;` | eval rows `addBad 2` gives 1 and `mulBad 2` gives 0 (want trap) | KILLED |
| HP | blsag.c:94 `blsag_hash_point`: the counter loop starts at 1 | eval row `hTwoGk 0` gives 0 (want 1) | KILLED |
| CH | eval.c:1651 `zk_unroll`: the final compare of the chain dropped (`return out;`) | precompile.sh: the wrong ring member cast want revert got ok (5 failures) | KILLED |
| NU | examples/zkcast.lang:19 `cast`: `flagIf flagNo` in place of the nullifier read | precompile.sh: the reused key image cast want revert got ok (1 failure) | KILLED |
| CC | eval.c:1639 `zk_unroll`: the choice hash line deleted | precompile.sh: the valid cast want ok got revert, count 1u reads 0 (2 failures); the flipped choice cast still reverts; `zk-flip` stays PATH_END | KILLED (code) |
| PI | eval.c:1638 `zk_unroll`: the poll id hash line deleted | precompile.sh: the valid cast want ok got revert, count 1u reads 0 (2 failures); the poll 7u cast still reverts | KILLED (code) |
| RG | eval.c:1650 `zk_unroll`: the s range test is `flag(m, 1)` | precompile.sh: the s_1 + r cast want revert got ok (2 failures) | KILLED |

Score: 8 of 8 mutants killed. CH, NU and RG are admissions at the named
EVM row (the wrong ring member, the reused key image, s_1 + r). FP, ON
and HP are killed at the named eval rows. CC and PI are code kills: the
gate fails at the valid cast, because the fixed `test/precompile.sh`
vectors (lines 102 to 120) are signed by the host with the choice and the
poll id in the hash chain. Dropping either hash changes the verifier's
chain, so these fixed vectors fail; the named row (the flipped choice,
the wrong poll id) still reverts. A signer using the altered chain can
produce signatures it accepts, and those signatures no longer bind the
omitted field. This run establishes detection of the lowering edits by
the valid vector, but does not exercise acceptance of such signatures
or their replay across choices or polls.

Open items. Q-B3b-1: `test/dispatch.sh:17` keeps `od -An -tx1` in `sel`,
the fold that broke the B3a selector in `test/evmchain.sh` (fixed there
with `od -v` in 92fb145). The EVM row for a key image off the curve has
no S8 mutant: it meets the older precompile site (PLAN section 8), and
ON covers the host side. The row I = (0, 0) has no mutant: it also fails
the compare. The guest one-use law (O13) and the section 7 contract
writer (Q3) are S9.

The S8 sessions are in the handoff (`orbit-lang-work/S8-HANDOFF.md`): the
mutants in session 13, and this section and the gate in session 14.

## S9

Slice S9 (the section 7 contract writer, contract end to end, PLAN row S9;
O15 (a), S8 Q3) is closed on USER commit 50fc1f5 (B1 to B3 and B5). B4,
the chain probe (O16), moves to S10 with O13 and O26 (USER ruling S9
session 10) and waits on the RPC URLs and a go (Q-S9-2). USER commit
45b4bd2 holds the S9 PLAN text. USER commit 2b02db1
holds the S9 rulings (PLAN `### USER ruling (S9 session 2, 2026-10-20)`,
O13, O15, O16 and O26) and the `od -v` fix in `test/dispatch.sh:17`
(Q-B3b-1). USER commit 7228663 holds B1 (the writer `langc contract`,
`src/front/writer.c` and `writer.h`: the ring closed in code from the host
key rule or `--ring`, START from `--start`, the entries `cast`, `enact`,
`count`, `outcome`, `selfConstituting`, `poll` and `constitution`;
`test/writertool.c`; the contract rows of `test/gate.sh`, the written DAO
block of `test/precompile.sh` and `test/contract/dao.expect`). USER commit
cee8237 holds B2 (the differential: `examples/dao3.lang`, the differential
definitions of `examples/dao.lang`, `test/contract/ballots3.lang` and the
14 rows of `test/contract/diff.txt`, read by `test/gate.sh`, `test/run.sh`
and `test/precompile.sh`). USER commit 55bd9c5 holds B3 (the ring size
rows `test/contract/ringN.lang`, the `ring_built` and `ring_refused` rows
of `test/gate.sh` and the gas table in docs/VALIDATION.md). USER commit
50fc1f5 holds B5 (the mutant run, 15 mutants, and the records; no source
change). The gate `make -B check` is GREEN, exit 0 (log
`orbit-lang-S9-B5-gate.log`), and the close gate of session 10 gives the
same counters (log `orbit-lang-S9-close-gate.log`).

Rulings. The USER ruling Q-S9-0 (2026-10-20) gives the slices B1 to B5.
The USER ruling S9 session 2: Q-S9-1 (a) O13 and O26 move to S10 with law
6, O26 stays a VALIDATION assumption until then; Q-S9-2 (a) a read only
`eth_call` probe from `probe/`, outside the gate (no RPC URL is named yet,
so B4 waits); Q-S9-3 (a) the ring is closed in code, with the `--ring` and
`--start` build arguments; Q-S9-4 (i) with fewer than n casts `outcome()`
reads m and `enact` (the amend of the design) reverts, (ii)
`selfConstituting()` reads 0 before the first amend, (iii) a constitution
is identified by its histogram table (equal tables share one entry id);
Q-B3b-1 yes. The USER ruling S9 session 10: B4 moves to S10 with O13 and
O26; Q-S9-5 (a) no row, the stuck code path stays; Q-S9-6 (a) the B3 state
stays (the two-choice fixtures refuse ring 23 with EVM_SIZE and ring 28
with EVM_SIGNATURE); Q-S9-7 (a) M10 stays recorded as equivalent on the
fixtures. S9 adds no former.

| Count | S8 (613bf43) | S9 | Reason |
|---|---|---|---|
| examples | 38 | 39 | B2 `dao3` (guest) |
| contracts | none | 2 written, 5 refused | B1 `dao.expect` and `--start 1` written; `nodao` and `open-choice` REFUSE_CONTRACT, `ring29` EVM_RING, `--ring 1 3 1 3` REFUSE_CONTRACT, `--kit contract` USAGE |
| rings | none | 6 built, 2 refused | B3 rings 2, 3, 4, 8, 16 and 22 build; ring 23 EVM_SIZE and ring 28 EVM_SIGNATURE at `langc --kit build` |
| differential | none | 28 guest evals, 14 host runs, 14 EVM rows | B2 the 14 rows of `test/contract/diff.txt` (the guest and the permuted eval per row, the written contract per row in the host and in the EVM) |
| precompile steps | 33 | 128 | B1 the written DAO block (17 steps), B2 the differential (78 steps: 6 rows of 5, 8 rows of 6) |
| writer regressions | none | passed | B1 `test/writertool.c` (the infinity and the duplicate ring key refusals) |

The other counts do not change: parse refusals 19, check refusals 163,
kit-only refusals 22, evals 143, proves 2, runs 5, run refusals 10,
asmtool 8 of 8, builds 3, lower refusals 16, build checks 33, dispatch 20,
evm steps 43, mock 13 rows, diff 15 rows, emit 24, and the review, domain,
bLSAG, path, fuel, output, lower and hashing regressions.

Mutants, one at a time (runner `orbit-lang-work/S9-B5-leg.sh`; the killer
lines are in the handoff `orbit-lang-work/S9-HANDOFF.md`, sessions 8 and
9). Each mutant is one site, made and put back by the Edit tool, with
`make -B -j 2` for each build and the killing leg run alone. The runner
prints `git diff --numstat` before each build, so each run also shows that
the mutant before it was put back, and it compares the written `dao.lang`
with `test/contract/dao.expect`. After the loop, `git diff` shows no
change and `make -B -j 2` on clean source exits 0.

| Mutant | Edit | Killer | Verdict |
|---|---|---|---|
| M1 CR | writer.c:368 the choice guard `u256Le k choice` at k + 1 | precompile.sh row 6: the cast of choice 2 want revert got ok (3 failures) | KILLED |
| M2 NP | writer.c:369 and 376 the nullifier key `hash (hash ix iy) (pollId s)` is `hash ix iy` | precompile.sh row 15: the cast at poll 1 want ok got revert, row 16 the count (2 failures) | KILLED |
| M3 PI | writer.c:382 `enact` keeps `pollId s` | precompile.sh row 12: `poll` want 1 got 0, row 15 the cast (2 failures) | KILLED |
| M4 TZ | writer.c:354 and 377 the tally key `hash (pollId s) c` is `c` | precompile.sh row 16: `count 0u` at poll 1 want 1 got 2 (1 failure) | KILLED |
| M5 CG | writer.c:381 the `enact` guard `u256Le n (casts s)` at 0 | precompile.sh row 2: `enact` want revert got ok (12 failures) | KILLED |
| M6 SC | writer.c:386 `u256Eq (active s) (prior s)` is `u256Eq (active s) (active s)` | precompile.sh rows 3 and 13: `selfConstituting` want 0 got 1, the differential rows `agree11` and `keep20` (10 failures) | KILLED |
| M7 HC | writer.c:105 the canonical configuration starts at `seen = 1` | precompile.sh row 10: `outcome` want 1 got 0, row 13 and the differential row `agree11` (13 failures) | KILLED |
| M8 RB | writer.c:422 the ring bound is `BLSAG_RING_MAX + 1u` | gate.sh `ring29.lang`: want exit 1 and EVM_RING, got exit 0 (1 failure) | KILLED |
| M9 TV | writer.c:294 the trie tests the votes of choice c against left minus v, not v | run.sh the `dao3` differential rows `maj30`, `maj21`, `maj12` and `maj03` (4 failures) | KILLED |
| M10 NX | writer.c:289 `nextOf` writes the outcome index `t`, not the entry id | none: run.sh exit 0, the written `dao.lang` equals `dao.expect` | EQUIVALENT on the fixtures |
| M11 RL | asm.h:14 `ASM_RUNTIME_MAX` is 24467 (one below the ring 22 runtime) | gate.sh `ring 22`: want a build, got exit 1 and EVM_SIZE (1 failure) | KILLED |
| M12 RH | asm.h:14 `ASM_RUNTIME_MAX` is 25600 (over the ring 23 runtime) | gate.sh `ring 23`: want exit 1 and EVM_SIZE, got exit 0 (1 failure) | KILLED |
| M13 SB | evm.c:12 `EVM_SIGNATURE` is 262 (the 261 byte signature of ring 28 fits) | gate.sh `ring 28`: want EVM_SIGNATURE, got EVM_SIZE (1 failure) | KILLED |
| M14 SZ | lower.c:163 the code EVM_SIZE is EVM_LONG | gate.sh `ring 23`: got `langc: EVM_LONG` (1 failure) | KILLED |
| M15 SN | lower.c:169 the code EVM_SIGNATURE is EVM_ABI | gate.sh `ring 28`: got `langc: EVM_ABI` (1 failure) | KILLED |

Score: 14 of 15 mutants killed, and M10 is equivalent on the fixtures.
M1 to M7 are killed at the named row of the written DAO block of
`test/precompile.sh`, M8 and M11 to M15 at the named row of `test/gate.sh`.
M9 is killed by the host differential at the `dao3` rows: on `dao` (k = 2)
the tables are symmetric in the two counts, so the swapped trie reads the
same outcome. M13 shows the order of the build checks: with the signature
bound raised, ring 28 meets the size limit. M14 shows that the
`langc --kit build` refusal line comes from `build_status` in
`src/lower.c`, not from `src/asm.c:98`. M10: `dedup`
(`src/front/writer.c:201`) gives the entry ids in the order of the first
equal table, so the entry id of outcome t is t when no two outcome tables
are equal. No fixture has two equal tables (the written headers: `dao` and
`dao3` have 2 outcomes and 2 constitutions, each ring fixture 1 and 1), so
the mutant writes the same text. A row that kills it needs a program in
which two outcomes have equal tables, a new example (Q-S9-7).

Open items, all carried to S10 (USER ruling S9 session 10): B4, the chain
probe (O16; Q-S9-2, the RPC URLs and a go), and O13 and O26 (Q-S9-1). The
USER ruled Q-S9-5, Q-S9-6 and Q-S9-7 (a), so they close with S9: no guest
program reaches the stuck table entry and the code path stays with no row;
`langc contract` writes the B3 fixtures for rings 23 and 28, but their
written programs do not build (EVM_SIZE and EVM_SIGNATURE, respectively),
and the B3 rows record it. The size limit depends on the generated
program, so 22 is not a general member cap (docs/VALIDATION.md).
M10 stays equivalent on the fixtures.

The S9 sessions are in the handoff (`orbit-lang-work/S9-HANDOFF.md`): the
plan and the rulings in sessions 1 and 2, B1 in sessions 3 and 4, B2 in 5
and 6, B3 in 7, the mutants in 8 and 9, this section and the gate in
session 9, and the close in session 10.

## S10

Slice S10 (law 6, the regimes, with O13, O26 and the final gate; PLAN
row S10; USER rulings Q-S10-0 and Q-S9-1 (a)) is DONE and CLOSED. B1
to B3 are complete on the USER commits 96274e9 (B1), 5dbae79 (B2) and
8779688 (B3). B5 (this section, the records and the final gate) is
complete on the USER commit 423de45. B6 (the mutants and their records)
kills 17 of 17 mutants on 2026-10-21. USER commit e13942f holds the S10
plan. USER commit
96274e9 holds B1, law 6: the family `Empty` in `domain/domain.lang`,
`examples/regimes.lang` (`Regime`, `regimeOf`, `betaGt`, `paraLaw`,
`forkLaw`, `noneLaw` and `noSelfCon`), the check refusals
`regime-para-wrong`, `regime-fork-wrong`, `beta-critical` and
`noselfcon-wrong`, and 6 evals. USER commit 5dbae79 holds B2, O13:
`examples/oneuse.lang` (the guest ledger model, `firstUse`, `otherUse`
and `oneUse`), the check refusals `oneuse-reuse-wrong`,
`oneuse-key-wrong` and `oneuse-nomark-wrong`, and 5 evals. USER commit
8779688 holds B3, O26: `agreePerm` and `keepPerm` in
`examples/dao.lang`, `majorityPerm` and `unanimityPerm` (with
`permFromAdj`) in `examples/dao3.lang`, the check refusal
`perm-member0-wrong` and 5 evals. B5 and B6 change only docs/PLAN.md,
docs/STATUS.md and docs/VALIDATION.md. S10 changes no C source and adds
no former.

Rulings. The USER ruling Q-S10-0 (2026-10-20) opens S10. The USER
rulings of S10 session 2: Q-S10-1 (a) the parts B1 to B6; Q-S10-2 (a)
S10 is the last slice, and the B5 gate is the final gate of orbit-lang;
Q-S10-3 (c) B4, the chain probe (O16), is dropped; Q-S10-4 (a) and, at
the B1 probe, Q-S10-9 (a) the empty family
`Empty := emptyLoop (emptyOf : Empty)`, because the parser needs one
constructor; Q-S10-5 (a) a guest classifier of the regime; Q-S10-6 (a)
β is a pair of `Size` values; Q-S10-8 (a) O26 is a guest theorem for
each menu entry of `dao` and `dao3`, with no kernel change. The USER
ruling of S10 session 3: Q-S10-7 (b) O13 is a state law on a guest
ledger model of the `used` map, with no signature change.

Findings. D1 (O26 drift): PLAN O26 and the Q-S9-1 ruling said that
docs/VALIDATION.md carries member permutation invariance as an
assumption until S10, but docs/VALIDATION.md had no such text, and the
S9 histogram table read F at one config per histogram with no stated
assumption. B3 proves O26 as a guest theorem for each menu entry. B5
records the theorem in docs/VALIDATION.md and restates PLAN O26 and
Q-S9-1 in place. D2 (O13 reading): as PLAN section 6 stated it, reading
(a), "the fibre of the verified ballots over one key image is a
proposition", is false. Under the static H_p table (S8 Q2, I = x H_i),
the key image does not depend on the choice, so that fibre has one
point for each choice. Reading (a) also needs
`keyImage : Ballot_priv -> KeyImage`, which is not one of the 9
operations (O4). The USER ruled (b) (Q-S10-7), and the law reads the
ledger state. D3 (B6 killers): 5 of the 17 mutants fail first at a row
that the design does not name. MR2 and MR3 fail at `paraLaw`, because
each mutant also changes the regime at β = 1, and `paraLaw` is the first
law in the file (the design names `beta-critical` and `forkLaw`). MP1
fails at `agreePerm`, MP2 at `majorityAdj` and MP3 at `permFromAdj`,
because these proofs use the changed function (the design names only
evals for MP1 and MP2, and `majorityAdj` for MP3). The check refusals
`beta-critical`, `oneuse-reuse-wrong` and `oneuse-key-wrong`, which the
design names for MR2, MO1 and MO3, do not fail. Each check refusal has
its own copy of the guest code, so a mutant of an example file does not
change it. When the examples gate refuses a file, each eval row of that
file also fails, because `langc eval` checks the file first. The table
below records the real killer of each mutant.

Gate. `make -B -j 2 check` is GREEN, exit 0 (log
`orbit-lang-S10-B5-gate.log`): 41 examples, 19 parse refusals, 171
check refusals, 22 kit-only refusals, 159 evals, 2 proves, the contracts
(2 written, 5 refused), the rings (6 built, 2 refused), the differential
(28 guest evals, 14 host runs and 14 EVM rows), 128 precompile steps and
the other rows of docs/VALIDATION.md, with 0 failures. Against the S10
session 1 gate, B1 moves examples 39 to 40, check refusals 163 to 167
and evals 143 to 149. B2 moves examples 40 to 41, check refusals 167 to
170 and evals 149 to 154. B3 moves check refusals 170 to 171 and evals
154 to 159. B5 moves no counter: the log is equal to
`orbit-lang-S10-B3-gate.log`. Each other counter stays as in the S10
session 1 gate. B6 moves no counter. The only diff of
`orbit-lang-S10-B6-gate.log` to `orbit-lang-S10-B5-gate.log` is the
place of one build command line (the `-j 2` build order).

Mutants (B6, `orbit-lang-work/S10-design.md` sections 5, 8 and 9). Each
mutant is one edit to the guest code: MR1 to MR5 (law 6), MO1 to MO6
(O13) and MP1 to MP6 (O26). `make -B -j 2 check` runs once for each
mutant (log `orbit-lang-S10-B6-<id>.log`), and `cp` restores the file
before the next mutant. A log line is the line of the first failure in
that log. Score: 17 of 17 killed.

| Mutant | Site | Change | Killer | Result |
|---|---|---|---|---|
| MR1 | `bump` in examples/regimes.lang (the step of `regimeOf`) | the first fixed point gives SI in place of AD, so 1 fixed point maps to SI | examples gate: `paraLaw` PATH_END (log line 36); the 6 regimes evals with `regimeAt11` (7 failures) | KILLED |
| MR2 | `betaGt` | num ≥ den in place of num > den (the zero case) | examples gate: `paraLaw` PATH_END (log line 36), D3; the 6 regimes evals with `beta11` (7 failures) | KILLED |
| MR3 | `isFix` | compares to `phaseZero` in place of o | examples gate: `paraLaw` PATH_END (log line 36), D3; the 6 regimes evals with `regimeAt21` (7 failures) | KILLED |
| MR4 | `nextPhase` (the step of `cycleMenu`) | the last phase selects itself | examples gate: `noneLaw` PATH_END (log line 36); the 6 regimes evals with `regimeCycle` (7 failures) | KILLED |
| MR5 | `Empty` in domain/domain.lang (Q-S10-9 (a)) | a base constructor `emptyBase` | examples gate: `exfalso` TYPE_MISMATCH, elim on Empty needs 2 cases (log line 36); the 6 regimes evals (7 failures) | KILLED |
| MO1 | `castWith` in examples/oneuse.lang | a spent mark records again | examples gate: `oneUseL` (a case of `oneUse`) PATH_END (log line 36); the 5 oneuse evals with `reuseCast` (6 failures) | KILLED |
| MO2 | `markAt` | k is not marked (`u j` in both branches) | examples gate: `oneUseL` PATH_END (log line 36); the 5 oneuse evals with `reuseCast` (6 failures) | KILLED |
| MO3 | `markAt` | each key image is marked (`spent` in both branches) | examples gate: `otherUse` PATH_END (log line 36); the 5 oneuse evals with `otherCast` (6 failures) | KILLED |
| MO4 | `bumpAt` | the count of the other choice moves | evals `countYes` (got `sz0`) and `countNo` (got `szS (szS sz0)`) (log line 40; 2 failures) | KILLED |
| MO5 | `castOr` | a revert gives `empty` in place of the ledger | eval `countYes` (got `szS sz0`) (log line 40; 1 failure) | KILLED |
| MO6 | `empty` | keyA starts spent | examples gate: `firstUse` PATH_END (log line 36); the 5 oneuse evals with `firstCast` (6 failures) | KILLED |
| MP1 | `permute` in examples/dao.lang | the swap case is the identity | examples gate: `agreePerm` TYPE_MISMATCH (log line 36), D3; the dao evals with `permSwap0`, the contract and the differential rows (27 failures) | KILLED |
| MP2 | `swap12` in examples/dao3.lang | the identity | examples gate: `majorityAdj` TYPE_MISMATCH (log line 36), D3; the dao3 evals with `adjS12` and the differential rows (21 failures) | KILLED |
| MP3 | `adj` | s01 gives `swap12` | examples gate: `permFromAdj` TYPE_MISMATCH (log line 36), D3; the dao3 evals with `adjS01` and the differential rows (21 failures) | KILLED |
| MP4 | `other` | the identity (a, then a) | eval `permTwo` (got `inr unit`) (log line 40; 1 failure) | KILLED |
| MP5 | `perm3` | the 3 letter word drops a letter (`swap01 (swap12 m)`) | examples gate: `permFromAdj` TYPE_MISMATCH (log line 36); the dao3 evals with `permThree` and the differential rows (21 failures) | KILLED |
| MP6 | `sameSym` in examples/dao.lang | refl at the (inl, inl) case | examples gate: `sameSym` PATH_END (log line 36); the dao evals, the contract and the differential rows (27 failures) | KILLED |

Open items. None in S10. B4 is dropped (Q-S10-3 (c)), so S10 runs no
chain probe (O16). A link of
`dao3` to `Orbit ≃ Bag n` (USER ruling S7, Q3) needs Member = `Pos n0`.
S10 does not build it. It is a later item if the USER asks.

The S10 sessions are in the handoff (`orbit-lang-work/S10-HANDOFF.md`):
the plan in session 1, B1 in session 2, B2 in session 3, B3 in sessions
4 and 4b, this section and the final gate (B5) in session 5, and the
mutants and their records (B6) in session 6.
