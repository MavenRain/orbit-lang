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
