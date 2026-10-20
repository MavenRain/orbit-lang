# orbit-lang plan

orbit-lang is a Cubical TT language for a zero-knowledge self-referential
DAO. A program declares one DAO. The compiler checks its laws and writes the
EVM bytecode of one contract. The contract deploys to any EVM chain, for
example Telcoin Network or Ethereum mainnet. `design/DESIGN.md` holds the
request and the design verbatim.

## 1. Rulings

USER rulings (the request, 2026-10-16):

- Type formers: products, coproducts, monads (map/bind), algebras and
  coalgebras (fold/unfold), quotients, paths, loops, HITs, QITs, HIITs,
  QIITs, as in Cubical TT.
- Base types and operations: only the ones in the pasted design (section
  "Types" and section "Operations"). The laws are a minimal list. We can add
  laws. We do not add base types or operations.
- Host: TinyCC.
- Target: EVM-deployable bytecode for any EVM chain.

USER rulings (S1, 2026-10-16):

- Every default in section 9 is RULED. Where an item has options, (a) is
  the ruling.
- Proof irrelevance is prohibited. orbit-lang has no Prop-like types: no
  Prop, no SProp, no strict propositions, no irrelevant arguments or
  fields, no definitional proof irrelevance, no K, no UIP. A proposition is
  a type with an isProp proof, and that proof is data the kernel checks.
- The kit is hosts/tcc-evm-contract (O2). Tests are guest-language files
  plus `langc run` scripts. The kit gate scripts stay. We add no new
  harness.

## 2. The reference design

Source: MavenRain/self-referential-dao, local clone
`~/Documents/2026-06-uat-dao` at a965183, and its dependency
`~/Documents/unified-aggregation-theory` (U below). The Lean code has no
`axiom` and no `sorry`. Three definitions are `noncomputable`
(SelfReferentialDAO.lean:332, :544, :671).

| Design name | Reference definition |
|---|---|
| Group | `SymmetryGroup` (U/ConfigSpace.lean:25) |
| act | `GAction G Obj`, `act : G.carrier -> (Obj ⥤ Obj)` (U/ConfigSpace.lean:43) |
| Config | a category `Obj`; the witnesses are `Discrete (Fin n -> Spin)` and `Discrete (Fin n -> Fin q)` (U/Bridge/SchellingIsing.lean:143, U/Bridge/Potts.lean:105) |
| Orbit | `OrbitGroupoid act`: all configs, a morphism is `(g, f : X ⟶ g·Y)` (U/Aggregation.lean:44-56) |
| orbitProj | `orbitProjection act : Obj ⥤ OrbitGroupoid act` (U/Aggregation.lean:179) |
| Constitution | `ChoiceRule Obj D := Obj ⥤ D` (U/ChoiceRule.lean:23) |
| Aggregation | `LeftKanExtension (orbitProjection act) F` (U/Aggregation.lean:223) |
| unit, descent, uniqueness | Lan fields `unit`, `desc`, `fac`, `uniq`; `desc_unique` (comp-cat-theory KanExtension.lean:93-135) |
| gov | `Gov L := orbitProjection act ⋙ L.functor` (SelfReferentialDAO.lean:146) |
| amend | no def; `GovPhi (canonicalAmendment act) L = Gov L := rfl` (SelfReferentialDAO.lean:186, :418) |
| IsSelfConstituting | `∃ L, ∀ X, (Gov L).obj X = F.obj X` (SelfReferentialDAO.lean:288) |
| regimes | `IsArrowDebreuRegime`, `IsArrowImpossibilityRegime`, `IsSchellingIsingRegime` (U/Regimes.lean:37-75) |
| β | `β : Rat`; `IsMeanFieldFixedPoint β m := m * (1 + |β m|) = β m` (U/Bridge/SchellingIsing.lean:432) |

Facts that the plan uses:

1. Over a discrete config category and a discrete target, a Lan exists iff
   the rule is constant on orbits (U/Characterization.lean:81). In Cubical
   TT this is the premise of the quotient eliminator. So the Lan facts
   become eliminator rules: unit = the computation rule
   `rec F [x] ≡ F x`, descent = the recursor, uniqueness = induction on
   `[x]` into a set.
2. The reference group acts on choices, not on members: Z2 flips all spins
   (U/Bridge/SchellingIsing.lean:121), S_q relabels colours
   (U/Bridge/Potts.lean:88). This is neutrality. Member permutation (S_n,
   anonymity) is not formalized in the reference. The pasted design calls
   law 1 "anonymity". See O5.
3. The regime count is the number of object-distinct Lans into an
   Indiscrete target (DENOTATIONAL-DESIGN.md:112). Univalently,
   `Indiscrete α ≃ 1` for an inhabited α, so this count collapses. See O8.
4. The reference constitutions are constant (SchellingIsing.lean:526,
   Potts.lean:190). The amendment loop of the Arcium port is not machine
   checked (arcium-self-referential-dao DENOTATIONAL-CORRESPONDENCE.md:81).
5. The reference has no cryptographic layer. The Arcium port encrypts
   ballots with MPC and has no nullifier and no eligibility check
   (arcium-self-referential-dao encrypted-ixs/src/lib.rs:53-56). The
   Assay/EVM port (MavenRain/assay-self-referential-dao) has no local clone.

## 3. The design on the kit basis

The design gives a signature: 11 types and 9 operations. orbit-lang reads it
as the interface of one DAO module. Each name has one of three kinds (O4):

| Kind | Names | Meaning |
|---|---|---|
| slot | Member, Choice, Outcome, Group | The DAO program gives each one as a type built from formers (finite coproducts, products, algebras), and gives `act`. |
| defined | Config, Orbit, Constitution, Aggregation, Tally | The prelude defines each one from the slots with formers. |
| primitive | Ballot_priv, ZKWitness | Opaque types. Only castPrivate, proveZK, verifyZK and project use them. The kit implements them. |

The defined types (default readings, O4 to O7):

```text
Config        := Member → Choice
Orbit         := Config / act                     -- QIT: [c], glue g c : [act g c] ≡ [c], set-truncated
Constitution  := Σ (F : Config → Outcome), (g : Group) (c : Config) → Path Outcome (F (act g c)) (F c)
Aggregation   := Orbit → Outcome
Tally         := Orbit
orbitProj c   := [c]
aggregate F   := Orbit-rec F.1 F.2                -- the quotient recursor; needs isSet Outcome
gov L         := (L ∘ orbitProj , λ g c i. L (glue g c i))
amend         := aggregate ∘ gov
```

On this reading the laws become kernel facts or guest theorems:

| Law | Form in orbit-lang |
|---|---|
| 1 anonymity | `λ i. aggregate F (glue g c i)`; the strong form is the second field of F |
| 2 legitimacy | unit: the computation rule (a refl path); descent: the recursor; uniqueness: a guest theorem by Orbit induction into a set |
| 3 self-constitution | `(aggregate F , p) : Σ (L : Aggregation), Path Constitution (gov L) F`, with p from funext and the unit, and the second field by isProp of the invariance type |
| 4 amendment | refl, by the definition of amend; also `amend (amend L) ≡ amend L` as a guest theorem |
| 5 ZK | section 6 |
| 6 regimes | section 7 and O8, O9 |

The contract never sees a Config. A ballot carries one public choice. The
contract adds it to the tally. The tally is the normal form of an Orbit
under member permutation: a count per choice. So the run time carrier of
Tally is a histogram, and Config is ghost (erased at lowering).

## 4. The base span

Guest-visible base types: the 11 design types, plus the Cubical TT
primitives (the interval I, Path, the universes, Glue). Guest-visible base
operations: the 9 design operations, plus the Cubical TT primitives (path
abstraction and application, transp, hcomp, Glue and unglue).

Kit items (default O3, the strict reading):

| Kit item | S3 action (DONE 2026-10-17) |
|---|---|
| Nat, U256, Addr, Flag and their operations | Not guest-visible. They stay inside the lowerer as carriers for Tally counts, member keys, poll ids and proof words. |
| sealed-lang base ops natDiv, natMod, hash, ecrecover, ecAdd, ecMul, ecPairing, addrToU256 (guest-visible since the S2 port of sealed-lang bc8e168) | Not guest-visible. The lowerer keeps them for the tree index, the nullifier hash and the S8 curve code. |
| Map (mapEmpty, mapGet, mapSet) | Not guest-visible. The lowerer uses it for the nullifier set. |
| `state`, Env, entries, views, pay, pull, events | Not guest-visible. The compiler writes the contract from the DAO declaration (section 7, O15). |
| Eq formers (refl, symm, trans, cong, transport) | None. The S2 port removed them (sealed-lang R3). Path replaces them. |
| Former terms (pair, inl, fold, unfold, map, bind, and the rest) | Kept. These are formers, not base items. |
| sample domain Color, Item, Stack, Lot | None. The S2 port removed it (sealed-lang R21). |

S3 is DONE (2026-10-17): the kit names and the 8 sealed-lang base ops
are INTERNAL, the literals and the `state` and `event` declarations are
kit-mode forms, and guest code that uses one of them is REFUSE_KIT
(section 8 "Builder call (S3, 2026-10-16)", docs/STATUS.md "## S3").

Counts, β and member indices are built from formers: Nat is the initial
algebra of `1 + X`. The lowerer maps a Nat-shaped algebra to a word (the
zkpolicy-lang P7b-nat precedent).

## 5. The cubical kernel

The kit front end is the tcc-wasm front end plus about 1293 diff lines.
sealed-lang src/ at its seed beae7ec is byte-identical to the current kit
src/. So the sealed-lang kernel commits apply as cherry-picks.

| Slice | Source | Commits | Gives |
|---|---|---|---|
| S2 (was S3) | sealed-lang | 872b0b4, 9394ffd, 705ab9f | I, PathP, Path, refl, sym, cong, funExt, transp, hcomp; path-word lowering refusal. Ported in S2. |
| S2 (was S4) | sealed-lang | cdfc751, 3460a4a, 3187312, fc7fb7f, 0c72c03, b4a5dce | HIT decls, path constructors, folds with path cases, formal hcomp, Trunc1, Trunc0, Quot, isProp, isSet. Ported in S2. |
| S5 | coin-lang | 44c917f, 91d7091, 34acff5 | Fin, equivalences, Glue, ua, uaBeta, hcomp at Glue and U, uaArrow (the hunks that sealed-lang R36 dropped) |
| S6 | new | (design reference: zkpolicy-lang a1780a9, f176ac6) | indexed families with path constructors (HIIT), inductive-inductive blocks with path constructors (QIIT) |

S5 needs a three-way merge, as sealed-lang did for each coin-lang commit.
S6 is new work in this lineage: no coin-lang or sealed-lang commit has
indexed path families. zkpolicy-lang has them on the tcc-wasm kit, in a
different kernel lineage. We do not mix the two lineages in one tree. So S6
uses zkpolicy-lang as a design reference only. O2 gives the other route.

Guest examples for S6:

- HIIT: `Bag : Nat → U` (USER ruling S6: `Bag A : Size → U`), with `nil : Bag zero`,
  `cons : Choice → Bag n → Bag (suc n)`,
  `swap : cons a (cons b t) ≡ cons b (cons a t)` and a set truncation. Then
  `Orbit ≃ Bag n` for n members is a guest theorem (Glue, S5). It needs
  O21, so it moves to S7 (USER ruling S6).
- QIIT: `Charter : U` and `Ratified : Charter → U` in one block.
  `genesis : Charter`, `adopt : (c : Charter) → Ratified c → Charter`, and a
  path constructor that identifies two adoptions with the same rule. This
  types the amendment history: each constitution is ratified by the one
  before it.

Proof irrelevance is prohibited (USER ruling, section 1): no Prop-like
types, no SProp, no irrelevant arguments, no definitional proof
irrelevance, no K, no UIP (O17). isProp and isSet are ordinary guest types.
Each slice adds a refusal row for each Prop-like form it could admit.

S2 result: the ported kernel refuses each form. `Prop` and `SProp` are not
types (NAME_UNKNOWN). They are reserved names (rule R4,
src/front/check.c:171), so no declaration can take them. The lexer and the
parser have no irrelevance marker. Two different elements of Trunc1 or
Trunc0 are not convertible, also with an isProp hypothesis (PATH_END). A
path between two paths needs a squash path, never refl (PATH_UIP, rule R9).

### Kernel rules carried from sealed-lang

The R-numbers in code comments come from three sources. The numbers
overlap, so the form of the cite tells the source.

- "rule Rn" with n from 1 to 5: the kit rules in formers/FORMERS.md:45-57.
  R1 no new data type (REFUSE_DATA). R2 no unproved fact (REFUSE_AXIOM).
  R3 no general recursion (REFUSE_REC). R4 no new core name (REFUSE_NAME).
  R5 host forms (not cited in code).
- "Rn (a)", "(Sx Rn (a))" and "rule R9" or "rule R12": coin-lang slice
  rulings. sealed-lang took them with the coin-lang kernel. The numbers
  restart in each coin-lang slice. The sources are
  ~/Documents/lang-template-work/coin-lang-S1-kickoff.md,
  coin-lang-S2-kickoff.md and coin-lang-S4-kickoff.md.
  - S1: R4 the interval is the free De Morgan algebra in normal form; R5
    interval substitution in values; R7 JSON rules 5 and 6; R9 the refusal
    codes PATH_END, PATH_UIP, INTERVAL_TYPE; R11 path eta and the De Morgan
    laws hold by conversion; R12 the PathP line is a syntactic interval
    abstraction.
  - S2: R1 face formulas are interval terms; R2 the hcomp surface; R3 the
    transp surface; R4 transp is the base on a data type; R6 transport,
    subst and J are guest definitions in domain.lang; R11 the closure shape
    of a LAM or PLAM value.
  - S4: R1 the HIT declaration form, in domain.lang only; R2 a path
    constructor reduces at its ends; R3 a formal hcomp at a HIT stays a
    value; R4 fold has one path case per path constructor; R5 Trunc1 and
    Trunc0 with squash cases; R6 the set quotient Quot A R; R7 transp at a
    family with parameters; R8 the Trunc0 fold needs the isSet RES case.
- Other Rn: sealed-lang rulings (sealed-lang docs/PLAN.md). R3 the kit Eq
  is removed and Path replaces it (PLAN:266). R14 natDiv and natMod, a zero
  divisor traps (PLAN:128). R16 hash is 2-ary over U256 (PLAN:315). R17 an
  ecAdd or ecMul result is a heap block, a stuck curve value lowers to
  PRECOMPILE, ecPairing takes a closed list (PLAN:317). R18 a STATICCALL
  that returns 0 reverts, an empty ecrecover return is address 0
  (PLAN:320). R19 natMul, u256Mul, u256Div, u256Min are pruned (PLAN:322).
  R20 addrToU256 is the one conversion from Addr (PLAN:324). R21 the sample
  domain is removed (PLAN:326). R23 ENTRY_PARAMS_MAX is 32 (PLAN:330). R27
  Nat, Flag, U256, Addr, Unit and Map are base lines in the Kan rules
  (PLAN:344). R31 a point constructor lowers like a family value, a path
  residual gives REFUSE_LOWER (PLAN:364). R33 no new base line, fold_path
  and fold_hcomp (PLAN:374). R36 the Glue hunks and Fin are dropped
  (PLAN:387).
- "R0" at src/front/check.c:948 is not a rule. It names the interval end
  value r0.

Do not renumber these cites. New orbit-lang rulings use O-numbers.

## 6. The zero-knowledge ballot layer

Default scheme (O10): a linkable ring signature over BN254 G1 (bLSAG form),
with the Fiat-Shamir challenge from keccak256. Reasons:

- The TinyCC host can run the prover, so proveZK is an operation the host
  can evaluate. A Groth16 prover in C is too large for one slice.
- The EVM verifier needs only ecAdd (0x06), ecMul (0x07) and modexp (0x05).
  The S2 port gives the runner 0x01, 0x06, 0x07 and 0x08 (sealed-lang
  bc8e168), not 0x05. 0x05 is an S8 item: S8 adds it to the runner and to
  test/precompile.sh if the bLSAG hash-to-curve needs it.
  These precompiles exist on Ethereum mainnet. Telcoin Network needs a probe
  (O16).
- There is no trusted setup and no circuit compiler.
- The key image is the nullifier: one per member per poll.

Cost: verification is linear in the ring size n, about 2n ecMul and 2n ecAdd
calls. The S9 gas table gives the numbers.

| Design operation | Host (evaluator) | EVM (lowered) |
|---|---|---|
| castPrivate m c | ballot := (secret key of m, c, poll id, ring) | not lowered (client side) |
| proveZK b | ring signature over (poll id, project b), key image | not lowered (client side); `langc prove` writes the calldata |
| project b | the choice field | the choice word in calldata |
| verifyZK w c t | a decidable check; the check reads the choice c (USER ruling S8 Q6), t is unread in S8 | ring verify, then key image unused, then tally increment; a failure reverts |

Readings of law 5 (O11 to O13):

- Completeness: `verifyZK (proveZK b) (project b) t ≡ Unit` is a Path in U through ua
  (needs S5). The host checks it on test vectors. The kernel does not prove
  it, because the check runs over BN254 arithmetic.
- Privacy: `Ballot_priv ≃ Choice × ZKWitness` is false as written, because a
  ballot holds a secret key that the choice and the witness do not fix. The
  default reading: `Ballot_pub := Ballot_priv / (same project, same key
  image)`, then `Ballot_pub ≃ Choice × ZKWitness` (Glue), and verifyZK
  factors through Ballot_pub (a guest theorem). Hiding of the secret key
  rests on discrete log hardness in the random oracle model. That is an
  assumption in docs/VALIDATION.md, not a guest theorem.
- One use: the contract stores each key image. A second ballot with the
  same key image reverts. The guest law: the fibre of verified ballots over
  one key image is a proposition (two such proofs are path-equal).

## 7. The DAO contract

The compiler writes one contract from a DAO declaration (default O15).

| Entry | Effect |
|---|---|
| constructor (ring, charter index) | stores the member ring (G1 points), poll id 0, the active constitution index |
| `cast(choice, ring signature, key image)` | verifyZK; a used key image reverts; adds 1 to the count of `choice` |
| `outcome()` view | `aggregate F` at the current tally, F the active constitution |
| `amend()` | `amend`: installs the constitution that the outcome selects, increments the poll id, sets the tally to zero |
| `selfConstituting()` view | 1 iff the installed constitution equals the prior one (the Arcium port reading of IsFixedPointObj) |

A constitution is code, so the contract holds a finite menu of compiled
constitutions, and `amend` selects an index (defunctionalization, O6).
Each menu entry carries its invariance proof at compile time. The proof is
erased in the bytecode. The poll id is a history field: it does not
decrease.

Law 6 (regimes) is optional in the design. Default (O8, O9): the regime is a
three-point coproduct `Regime := AI + AD + SI` that the compiler computes
from the menu and the phase set. β is a pair (num, den) of former-built Nat
values, and β > 1 iff num > den (the Arcium `BetaPressure` reading). The
law `¬ IsSelfConstituting F` for an empty Aggregation is a guest theorem.

## 8. Slices and gates

Each slice ends with `make check` GREEN, mutants one at a time (`make -B`
for each build), the counts in docs/STATUS.md, and a USER commit.

| Slice | Content | Gate additions |
|---|---|---|
| S0 | seed: design, plan, kit scaffold | kit gate GREEN |
| S1 | rulings on O1 to O20: DONE 2026-10-16, all defaults, proof irrelevance prohibited | none |
| S2 | DONE 2026-10-16: port of the sealed-lang S2 to S4 kernel (paths, Kan operations, HITs, QITs, truncations), sample domain removed, EVM runner precompiles 0x01, 0x06, 0x07, 0x08 (sealed-lang bc8e168) | Prop and SProp as a type and as a name; 4 irrelevance marker spellings; refl between different Trunc1, Trunc0 and isProp elements; a path of paths between different loops; precompile vectors (EIP-196, EIP-197) |
| S3 | DONE 2026-10-17: base-span pruning: the kit items and the sealed-lang base ops natDiv, natMod, hash, ecrecover, ecAdd, ecMul, ecPairing, addrToU256 made internal (section 4); the 11 types and 9 operations declared | refusals for a kit name and for a sealed-lang base op in guest code (22 REFUSE_KIT rows, each `ok` under `langc --kit`); 6 REFUSE_SIG rows for the design signature |
| S4 | DONE 2026-10-17: Orbit as a quotient of the voter orbits (the HIT, QIT and truncation formers came with the S2 port), as a kernel declaration after `act`; the dependent eliminator `elim` on Sum and Unit (USER ruling 2026-10-17) | Orbit fold without isSet, wrong Orbit boundary, Orbit path at run time (REFUSE_LOWER); elim case type, case count and scrutinee kind; Prop-like: a motive into Prop, elim on a path (no K), no Unit eta |
| S5 | DONE 2026-10-17: Glue, ua, uaBeta, hcomp at U (coin-lang merge of 44c917f, 91d7091 and 34acff5, builder call S5); Glue, glue and unglue guest formers (F17); ua and the equivalences domain definitions; Fin internal (O23) | Glue boundary mismatch, ua on a non-equivalence; Glue tube, cover and binder, unglue type; Fin range and case; hcomp cap and universe; Prop-like: a Glue at Prop, two isEquiv proofs, two ua proofs |
| S6 | DONE 2026-10-18: HIITs and QIITs (new work, builder call S6): index telescopes and constructor results (HIT_INDEX), `and` blocks (BLOCK_ORDER), indexed and block folds, the domain families Size, Bag, Charter and Ratified (F18, F19); USER commits 6fa6a15 (B1, B2) and 8dd82a3 (B3); B4 rows, M3b and 9 of 9 mutants killed (STATUS "## S6") | wrong index in a path constructor (`hiit-wrong-index`, `hiit-no-result`, `hiit-wrong-head`), inductive-inductive order (`block-order`), missing coherence case (`bag-no-squash`, `block-fold-no-squash`); block fold arity (`block-fold-target`, `block-fold-extra`) |
| S7 | DONE 2026-10-18: DAO prelude and laws 1 to 4 as guest theorems; menu of constitutions (new work, builder call S7): elim on domain families (F16), the prelude after `orbitProj` and `outcomeSet`, the slots `outcomeSet` and `menu`, the domain families Pos and Word, `bagOrbit : (n : Size) -> Equiv (OrbitN n) (Bag Choice n)`; USER commits 676c7a8 (B1), eafa1df (B2), ac157d0 (B3 part A) and 9c7b033 (B3 part B); B4 records, 9 of 9 mutants killed (STATUS "## S7") | a non-invariant constitution is refused (`constitution-noninvariant`); a wrong gov unit is refused (`gov-unit-wrong`); elim rows (`elim-family-count`, `elim-path-case`, `elim-motive-index`, `elim-block`, `elim-family-prop`); slot and prelude rows (`outcome-set-type`, `menu-type`, `dao-redefine`, `dao-no-set`) |
| S8 | DONE 2026-10-20: ZK layer (BN254 G1 in C, castPrivate, proveZK, project, verifyZK lowering, nullifier map; PLAN section 8, builder call S8); USER commits c3ab788 (B1), 8bd87a7 (B2a), b61db98 (B2b), adf9f45 (B2c), 92fb145 (B3a) and 3fc7f50 (B3b); B4 records, 8 of 8 mutants killed (STATUS "## S8") | known-answer vectors (k G, H_p, bLSAG for n = 2 and n = 3); check rows `zk-flip` (PATH_END) and `zk-redefine` (REFUSE_SIG); EVM revert rows in `test/precompile.sh`: wrong ring member, flipped choice, wrong poll id, key image off the curve, key image (0, 0), s_1 + r, reused key image |
| S9 | PLANNED (USER ruling Q-S9-0, 2026-10-20): the section 7 contract writer, contract end to end (O15 (a); S8 Q3): B1 the writer `langc contract` (deploy, cast, outcome, amend and the `selfConstituting()` view; the contract binds the poll id, S8 Q5); B2 the host and EVM differential; B3 the gas table per ring size (O28: n + 4 words per cast, 28 members at most) and the code size rows; B4 the chain probe (O16, outside the repo gate); B5 mutants and records | differential rows (guest eval, host run and EVM outcome per histogram); revert rows (a used key image, a choice out of range, a cast at an old poll id, amend before a complete tally); writer refusals (no DAO prelude, a carrier that is not finite, a ring above 28 members); size limits EIP-170 and EIP-3860 |
| S10 | law 6 (regimes), docs, final gate | regime rows |

Builder call (S2, 2026-10-16): port route Q. A probe in scratch clones
compared two port orders on 6cd1104. Route P (sealed-lang 872b0b4,
9394ffd, cdfc751, 3460a4a, 3187312, fc7fb7f, 0c72c03 straight) gave 10
conflict hunks in 8 files. Route Q (sealed-lang bc8e168 first, then the
same list) gave 0 conflict hunks. 705ab9f and b4a5dce conflict only in
docs, so S2 applies their code paths and kit docs by hand. The rule is
"port first, prune second": S2 ports the sealed-lang S2 to S4 kernel, S3
prunes the base span, S4 makes Orbit a quotient. No slice is dropped.

Builder call (S3, 2026-10-16): mechanism (a), a visibility tag at name
resolution. Each BUILTINS row and each core type has the tag GUEST or
INTERNAL. INTERNAL rows: Nat, U256, Addr, Flag, Map and all their
operations, natDiv, natMod, hash, ecrecover, ecAdd, ecMul, ecPairing,
addrToU256, mapEmpty, mapGet, mapSet. The contract prelude (contract.c:
Env, makeEnv, now, caller, Out, pay, pull, emit) gets its own origin
ORIGIN_KIT, and its names are INTERNAL. GUEST rows: the formers, the
interval, Path, the Kan operations, the universes and the domain.lang
names. In guest mode a declaration from the program cannot resolve an
INTERNAL name, cannot use a Nat or word literal, and cannot declare
`state` or `event`: each is REFUSE_KIT. INTERNAL names resolve in the
trusted prelude scope (ORIGIN_DOMAIN, ORIGIN_KIT) and under `langc --kit`,
the kit mode that the kit gate scripts pass. The kit names stay reserved
(rule R4) in both modes, because the lowerer finds `Env`, `Out` and
`State` by name. Reasons against (b), a guest pass after elaboration: a
literal or a `state` declaration has no name to find after elaboration,
and one refusal site at resolution is one mutant target. A kit row (one
that uses a kit name or a literal) is listed in `examples/kit.txt` or
`test/check/kit.txt` and runs under `--kit`. Its expected text does not
change. Each REFUSE_KIT row must also give `ok` under `--kit`, so the
refusal comes from the mode only. This does not change O3, O4 or O15:
the kit mode is a trusted test scope, not a guest form.

The design signature in S3 (section 3, O4) is the table SIGNATURE in
check.c: 11 types and 9 operations, each with its kind. Slots (Member,
Choice, Outcome, Group, act): a program definition with a slot name must
have the slot type: `Type 0` for the four types, and
`Group -> (Member -> Choice) -> Member -> Choice` for act (Config
expanded), after the four type slots. Otherwise REFUSE_SIG. Primitives
(Ballot_priv, ZKWitness, castPrivate, proveZK, verifyZK, project): the
names are reserved (REFUSE_SIG). The kit gives their carriers and
operations in S8, because their types need the slots of the DAO
declaration and the ring-signature words. Defined names (Config, Orbit,
Constitution, Aggregation, Tally, orbitProj, aggregate, gov, amend): the
names are reserved (REFUSE_SIG). S4 gives Orbit, and the S7 prelude
gives the rest. S3 adds no binder, no universe and no declaration form,
so no new Prop-like form opens.

Builder call (S4, 2026-10-17): Orbit is the set quotient of the voter
assignments by act, as a kernel declaration. When the program defines
the slot `act` at its slot type, the front end puts two trusted
definitions (ORIGIN_DOMAIN) directly after it:

```text
def Orbit : Type 0 := Quot (Member -> Choice) R
def orbitProj : (Member -> Choice) -> Orbit := fun (c : Member -> Choice) => quot (Member -> Choice) R c
R = fun (c : Member -> Choice) (d : Member -> Choice) => Sigma (g : Group) (Path (Member -> Choice) (act g d) c)
```

R is written inline, so the kernel adds no new name. The glue path
`[act g c] ≡ [c]` of section 3 is `quotEq (Member -> Choice) R (act g c)
c (pack g refl)`. quotSquash makes Orbit a set, so an Orbit fold needs
the quotSquash case, of type isSet RES (R8 (a)). S4 gives orbitProj with
Orbit, because orbitProj is the point constructor [c]. Config stays
reserved, and the guest writes it as `Member -> Choice`. The S7 prelude
gives Config, Constitution, Aggregation, Tally, aggregate, gov and amend.
A program without `act` has no Orbit (NAME_UNKNOWN). A guest definition
of Orbit or orbitProj stays REFUSE_SIG, also after `act`.
test/check/sig-defined.lang has no `act`, so it stays a guest-only row
with the code REFUSE_SIG. A guest can give R its own name, and
conversion unfolds that name.

USER ruling (S4, 2026-10-17): S4 adds a dependent eliminator. A probe on
3a1ba76 showed that no Orbit value can get to a word position. Each
Orbit fold needs an isSet proof of its target. The kernel has no
dependent eliminator and Unit has no eta, so no word carrier and no Sum
has an isSet proof. The former `elim P c1 ... cn x` takes a motive
`P : X -> Type k`, one case for each constructor and the scrutinee
`x : X`, and has the type `P x`. In S4, X is `Sum A B` (cases
`(a : A) -> P (inl a)` and `(b : B) -> P (inr b)`) or `Unit` (case
`P unit`). `elim P f g (inl a)` reduces to `f a`, `elim P f g (inr b)`
to `g b` and `elim P u unit` to `u`. On a different value, elim stays
neutral (O22). The kernel refuses elim on a type with path constructors
(O21), on Nat, List or a Path (no K), and elim with a wrong case count
or a wrong case type. With elim, a guest proves isSet Unit, isSet
(Sum A B) from isSet A and isSet B, and isSet Choice, and folds Orbit
into Choice. So the run-time row is a lower row: an Orbit path at a
word position gives REFUSE_LOWER. elim adds a binder (the motive), so
S4 adds Prop-like rows: a motive into Prop (NAME_UNKNOWN), elim on a
path (no K), and two Unit variables that are not convertible (no eta,
no strict proposition). S4 adds no empty type: `either` into `Type 0`
gives the disjointness cases.

Builder call (S5, 2026-10-17): port the three coin-lang commits in
order, 44c917f, 91d7091, 34acff5, with `cherry-pick -n` onto 9e28399,
and resolve each by hand before the next. The repos have no common git
ancestor. The file base of 44c917f is its coin-lang parent 582ecef, the
last coin-lang commit that sealed-lang ported before R36 dropped the S3
chain. A probe in a scratch clone gave 44 conflict hunks (26 in code)
and 6 modify/delete when each commit goes onto 9e28399 alone, and 21
hunks when they go in order. The code conflicts are at the Rule enum,
the `rule` switch, BUILTINS (each orbit-lang row has the seventh field
VIS), transp_data, reduce_op, kan_apply, hcomp and rule_hcomp. Each new
Op, Rule and closure tag gets an arm in each switch (clang
`-Wswitch-enum`) and a row in OP_NAMES and OP_ARITY. orbit-lang has no
JSON emitter and its emit gate is bytecode, so the hunks of src/json.c,
test/json and test/emit are not ported (a deviation). The hunks of
test/path-regressions.c go into test/pathtool.c.

Guest forms: Glue, glue and unglue are formers with VIS_GUEST, not
INTERNAL names. ua, uaArrow, isContr, fiber, isEquiv, Equiv, idEquiv,
isoCap, isoPath and isoToEquiv are domain definitions (ORIGIN_DOMAIN):
guest code uses them and cannot define them again (R4). uaBeta and
finAll are not names: uaBeta is the reduction `transp (ua e) x` to the
function of e applied to x (transp_glue), and finAll is the check of a
function on `Fin n` by its n literal cases. Fin, finAdd and finNeg are
VIS_INTERNAL (O23): `Fin n` takes `n : Nat` and its elements are Nat
literals, which guest code cannot write (REFUSE_KIT, O3 (a)). So the
Fin rows and examples/equiv.lang run under `langc --kit`. A guest
example on the span shows univalence: the flip of Choice (by elim) as
an equivalence by isoToEquiv, and transp along its ua reduces. The
domain definitions that sealed-lang R36 dropped with Glue (its O36
list) come back where orbit-lang S3 kept the definitions that they
extend, in kit mode if they use Fin.

Refusal rows: a Glue boundary mismatch (GLUE_BOUNDARY), ua on a
non-equivalence (TYPE_MISMATCH), a Glue tube that is not an
equivalence (GLUE_EQUIV), a Glue system that does not cover its face
(SYSTEM_COVER), unglue on a value that is not of a Glue type
(UNGLUE_TYPE), a used tube binder (GLUE_BINDER), uaBeta on a neutral
equivalence (PATH_END), ua of an equivalence equal to refl (PATH_UIP),
hcomp at U with a wrong cap (HCOMP_CAP), hcomp above Type 0
(HCOMP_UNIVERSE), and in kit mode a Fin literal out of range
(FIN_RANGE) and a missing or stuck Fin case (FIN_CASE). A Glue type or
glue value at a word position gives REFUSE_LOWER. With hcomp at U the
row hcomp-universe (hcomp at Type 0) is accepted, so it goes out of
the check rows and kit.txt. Glue opens one binder form, the tube
`fun (_ : I) =>`, and no universe, so S5 adds Prop-like rows (R11): a
Glue at Prop (NAME_UNKNOWN), two different isEquiv proofs of one map
that are not convertible, and the ua of one map with two different
equivalence proofs that are not convertible (no proof irrelevance for
isContr or isEquiv). Each refusal site gets one admission mutant.

O22 with hcomp at U: it still holds. hcomp at U gives a type (a Glue
type), hcomp at a Glue type gives a glue element, and unglue of a glue
element over a Sum base reduces by the hcomp of data (a constructor or
an `hcomp_stuck` value). So `blocked` sees a constructor or a stuck
value before the formal hcomp test of `reduce_elim`. One eval row pins
it: elim on the unglue of an hcomp in a Glue type over a Sum.

The build is by hand (the USER route of S2 to S4), one commit at a
time with the gate after each.

### Builder call (S6, 2026-10-17)

Source: new work in this lineage (O2 (a)). zkpolicy-lang a1780a9 (indexed
families, HIIT) and f176ac6 (mutual blocks) are a design reference only.
Survey: ~/Documents/orbit-lang-work/S6-survey.md (kernel) and
S6-survey-ref.md (reference). Facts: a family has parameters only
(FamilyInfo param_types and param_count, core.h:281), a constructor has no
result annotation, there is no mutual block, fold has no motive (the result
type comes from the hint), and only domain/domain.lang declares a family
(REFUSE_DATA, check.c:3219). Nat is VIS_INTERNAL (check.c:116).

Surface. Family declarations stay in domain.lang (kit rule R1 and coin-lang
S4 R1 hold; a program family is REFUSE_DATA).

- Index telescope: `family Bag (A : Type 0) : Size -> Type 0 := ...`. The
  type after `:` is a Pi telescope that ends in `Type 0`. Its depth is the
  index count. A family without `:` is uniform, as before.
- Each constructor of an indexed family states its result after its
  fields: `nil : Bag A sz0`, `cons (n : Size) (a : A) (t : Bag A n) : Bag A
  (szS n)`. A path constructor states it after its interval fields and
  before its boundary. An application takes the parameters and the fields;
  the result index is the annotation at the fields.
- Block: `family Charter (R : Type 0) := ... and Ratified (R : Type 0) :
  Charter R -> Type 0 := ...`. New keyword `and`. All headers of a block
  bind first, then the point constructors of all sorts, then the path
  constructors of all sorts. So a boundary can name a point constructor of
  a later sort.
- Sets: each sort that must be a set has a 2-dimensional squash
  constructor at each index, in the form of Trunc0 squash0.

Guest examples (O20, USER ruling S6):

- `family Size := sz0 | szS (s : Size)`: a domain index family.
- `family Bag (A : Type 0) : Size -> Type 0 := nil : Bag A sz0 | cons (n :
  Size) (a : A) (t : Bag A n) : Bag A (szS n) | swap (n : Size) (a : A) (b
  : A) (t : Bag A n) (i : I) : Bag A (szS (szS n)) [ ~i -> cons (szS n) a
  (cons n b t), i -> cons (szS n) b (cons n a t) ] | bagSquash ...`.
- `Charter` and `Ratified`: `genesis : Charter R`, `adopt (c : Charter R)
  (r : Ratified R c)`, `ratify (c : Charter R) (rule : R) : Ratified R c`,
  and the path constructor `adoptTwice (c : Charter R) (rule : R) (i : I)`
  from `adopt c (ratify c rule)` to the adoption of the same rule a second
  time (law 4 in the type: amend (amend L) is amend L). Squash
  constructors for both sorts.

Check. New refusal codes; one site each.

- HIT_INDEX: a constructor of an indexed family has no result annotation,
  or its head is not the family at its parameters, or an end of a path
  constructor has the family at a different index (gate row "a wrong index
  in a path constructor").
- BLOCK_ORDER: the header of a block sort (parameters or index types) names
  that sort or a later sort of the block (gate row "inductive-inductive
  order").
- A fold that omits a squash case (gate row "a missing coherence case")
  is TYPE_MISMATCH at the S2 case-count site (check.c:1184): no new code.
- Recursive fields stay syntactic: the head is a block sort at its
  parameters (DOMAIN_FAMILY otherwise).
- Prop-like forms (R11 and USER ruling, section 1): the index telescope is
  a new binder. `Prop` and `SProp` there are NAME_UNKNOWN (reserved, rule
  R4). The grammar has no irrelevant index. A sort without a squash
  constructor is not a set by conversion (PATH_UIP).

Fold. Non-dependent only.

- Indexed family: the target is a constant type from the hint, as for a
  uniform family. A case takes the fields; a recursive field at any index
  becomes the target. Index fields stay.
- Block of n sorts: the first n arguments are the target types, one for
  each sort, then one case for each constructor of each sort in
  declaration order, then the value. The sorts must share a parameter
  telescope at the scrutinee's arguments. A one-sort family keeps the hint form.
- Eval: fold_family, fold_path and fold_square apply at each index. A
  field of a sibling sort folds with the same case vector at that sort.
  transp at an indexed family with an index line that is not constant
  stays neutral (O25).
- Lowering: no change. A value of an indexed or block sort at a word
  position is REFUSE_LOWER (lower.c:645).

Meets. The S2 HIT and QIT kernel: uniform families do not change. Orbit
(S4): Orbit is a Quot instance, not a block. elim (S4): Sum and Unit only;
S6 folds do not give O21 (USER ruling S6: O21 and `Orbit ≃ Bag n` go to
S7). Glue and hcomp at U (S5): a formal hcomp at an indexed sort is a
value (coin-lang S4 R3), and its fold is an hcomp at the target.

Guest or kit. Size, Bag, Charter, Ratified, their constructors and their
folds are ORIGIN_DOMAIN, so guest code can use them (kit_head,
check.c:2148). Nat and its operations stay VIS_INTERNAL.

Slices. B1 lexer `and`, parser index telescope, constructor result and
block, AST (no new rows). B2 check: FamilyInfo indices, CtorInfo result
index, HIT_INDEX, BLOCK_ORDER, Size and Bag in domain.lang, refusal rows
by domain override (front.c:67). B3 fold and eval for indexed and block
folds, Charter and Ratified, examples/hiit.lang and qiit.lang, eval rows.
B4 M3b mutant, FORMERS F18 HIIT and F19 QIIT, SPEC, tcc-evm-contract.md.
Then mutants (one admission mutant for each new site, a "b" mutant for a
row with a second refusal class) and the close.

B2 safety: indexed folds pass only family parameters to constructor field
types and boundary images. Mutual folds are refused with TYPE_MISMATCH
until B3 supplies the targets and cases for every sort. Transport at an
indexed or mutual sort stays neutral until its field lines retain the
recursive fibre and sibling sort. Block headers cannot fold a sort whose
constructors are still reserved. Block universes are rechecked to a fixed
point before use, and BLOCK_ORDER checks free sort names, respecting local
binders. State declarations retain their unindexed form.

### USER ruling (S6, 2026-10-17)

- Bag index: "Domain index family". domain.lang declares `Size := sz0 |
  szS (s : Size)` before Bag; Bag is indexed by Size. Nat stays
  VIS_INTERNAL (S3 holds).
- O21 timing: "Equivalence in S7". S6 ships the Bag and Charter formers,
  their folds and the three gate rows. O21 and `Orbit ≃ Bag n` move to S7.
- M3b: "Yes, in S6". One mutant that removes both refusals of the S5 M3
  row, as S4 N1b did.

### Builder call (S7, 2026-10-18)

Source: new work in this lineage (O2 (a)). Survey:
~/Documents/orbit-lang-work/S7-survey.md (kernel at 8b11846). Facts: elim
takes Sum and Unit only, with one refusal site (check.c:872). PathP is in
the kernel (rule_pathp check.c:663, check_plam check.c:2463, transp along
a PathP eval.c:2241). A fold has a constant target from the hint
(check.c:1149, check.c:1233), and case_fields drops the index
(check.c:1001). Quot is a domain family (domain.lang:46-49).
orbit_after_act (front.c:22-45) splices kernel/orbit.lang after `act`.
check_sig skips ORIGIN_DOMAIN declarations (check.c:3400), and kernel_sig
refuses a program Orbit or orbitProj (check.c:3415). domain.lang has
trans, isProp, isSet, Equiv and isoToEquiv, and no funext lemma (a path
lambda gives it, as uaArrow does). The parser has no surface for a DAO, a
menu or a constitution.

O21 and `Orbit ≃ Bag n`. The equivalence holds when Group acts on an
n-member Member by member permutations: then an orbit is the bag of the n
choices. It does not hold for the neutrality group (the Z2 flip of
examples/orbit.lang). The index n is a Size. Scope (Q4): n = 2,
`szS (szS sz0)`, in a new guest example `examples/bagorbit.lang` with
Member and Group both `Sum Unit Unit` (identity and member swap), and
`orbitBag : Equiv Orbit (Bag Choice (szS (szS sz0)))` by isoToEquiv.

What the S6 fold gives (constant targets only, no dependent fold):

- f : Orbit -> Bag Choice 2 is a Quot fold into Bag Choice 2: the quot
  case lists the two choices, the quotEq case is elim on the group element
  (refl or swap), and the quotSquash case is bagSquash at index 2. This is
  the form of `agree` (examples/orbit.lang:91).
- g : Bag Choice 2 -> Orbit is a Bag fold into the constant target
  `Sum Unit (Sum Choice Orbit)` (size 0, size 1, size 2; one fixed orbit
  for a larger size), then a projection to Orbit. The swap case is elim on
  the folded tail, and the bagSquash case is isSet of the target (isSetSum,
  isSetUnit, isSetChoice, quotSquash).
- The two homotopies are dependent, so no S6 fold gives them.
  `(x : Orbit) -> Path Orbit (g (f x)) x` is induction on Quot into a
  family of propositions. `(k : Size) -> (y : Bag Choice k) -> Path (Bag
  Choice k) (rebuild k (fold y)) y` is induction on Bag with a motive over
  the index and the value (rebuild by induction on Size). Its swap case is
  a square in a set, and its bagSquash case is a cube in a set.

So O21 needs a dependent eliminator over Bag, with a motive over the
index, and over Quot. This is USER question Q1. Law 2 uniqueness needs
the same eliminator on Quot. Default (a): elim takes a domain family with
path constructors (survey A). The motive is `(idx) -> F params idx ->
Type k`. A point case takes the fields and one induction binder for each
recursive field, and gives `P idx (c params fields)`. A path case gives
`PathP (fun i => P idx (c .. i)) img0 img1` (path_case_type with the
motive line). A squash case is the square of square_type over P. Eval:
elim on a constructor applies the case to the fields and to the elims of
the recursive fields; on a path constructor at r it gives the case at r
(as fold_path); on a squash at r and s it gives the case at r and s (as
fold_square). On a formal hcomp or a neutral value, elim stays neutral
(O22). Block sorts (Charter, Ratified) stay outside elim in S7
(TYPE_MISMATCH at check.c:872), because no law needs them.

DAO prelude. `kernel/dao.lang` (ORIGIN_DOMAIN) is spliced after the last
of `act` and `outcomeSet` (Q2), by the orbit_after_act mechanism. The
program gives `menu` (Q3) after the prelude, because the menu type names
Constitution. The prelude adds no name that section 3 does not give:

```text
def Config : Type 0 := Member -> Choice
def Constitution : Type 0 := Sigma (F : Config -> Outcome) ((g : Group) -> (c : Config) -> Path Outcome (F (act g c)) (F c))
def Aggregation : Type 0 := Orbit -> Outcome
def Tally : Type 0 := Orbit
def aggregate : Constitution -> Aggregation := a Quot fold of witness F: the quotEq case is trans of (sym (cong (witness F) (payload r))) and (payload F (witness r) b), the quotSquash case is outcomeSet (the form of agree)
def gov : Aggregation -> Constitution := fun L => pack (fun c => L (orbitProj c)) (fun g c => cong L (quotEq (act g c) c (pack g refl)))
def amend : Aggregation -> Aggregation := fun L => aggregate (gov L)
```

Laws, as guest theorems in a new guest example `examples/dao.lang`:

| Law | Guest theorem |
|---|---|
| 1 anonymity | `anon F g c : Path Outcome (aggregate F (orbitProj (act g c))) (aggregate F (orbitProj c))` by cong of `aggregate F` on the quotEq path; the strong form is `payload F` |
| 2 unit | `unit F : Path (Config -> Outcome) (fun c => aggregate F (orbitProj c)) (witness F)` is refl (the fold rule and function eta) |
| 2 descent | aggregate |
| 2 uniqueness | `desc F L h : Path Aggregation L (aggregate F)` for `h : Path (Config -> Outcome) (fun c => L (orbitProj c)) (witness F)`: a path lambda over elim on Orbit into `fun x => Path Outcome (L x) (aggregate F x)` (O21) |
| 3 self-constitution | `selfCon F : Sigma (L : Aggregation) (Path Constitution (gov L) F)` is `pack (aggregate F) p`; the first fields convert, and p is a path lambda over isPropInv, a guest lemma from outcomeSet (no PathP, no connection) |
| 4 amendment | `amendLaw F : Path Aggregation (amend (aggregate F)) (aggregate (gov (aggregate F)))` is refl; `amendTwice L : Path Aggregation (amend (amend L)) (amend L)` is cong of aggregate on the isPropInv path from `gov (amend L)` to `gov L`; a Charter block fold into Aggregation (the adopt case is amend of the base) has the adoptTwice case `sym (amendTwice L)`, which is law 4 in the type |

The menu (Q3, default): the program gives `def menu : Outcome ->
Constitution`. The outcome selects the next constitution (section 7,
`amend()`). Each entry carries its invariance proof at compile time.
Member permutation invariance of an entry (O5 (a), the run time tally) is
O26, in S9.

Gate rows. PLAN S7 rows, guest mode:

- `constitution-noninvariant`: `pack F (fun g c => refl)` at Constitution
  for a rule that is not invariant (the choice of the first member, under
  the member swap). The refl end site (check.c:741-752) refuses it; the
  row pins the code.
- `gov-unit-wrong`: `refl` at `Path (Config -> Outcome) (fun c =>
  aggregate F (orbitProj c)) G` for a G that is not `witness F`. The same
  refl site refuses it.
- New rows: `outcome-set-type` and `menu-type` (REFUSE_SIG at slot_ok),
  `dao-redefine` (a program `def aggregate` after the slots, REFUSE_SIG at
  kernel_sig), `dao-no-set` (a program without outcomeSet has no
  aggregate, NAME_UNKNOWN), `elim-family-count`, `elim-path-case` (a path
  case at a constant line), `elim-motive-index` (a motive without the
  index binder), `elim-block` (all TYPE_MISMATCH), and `elim-motive-prop`
  (Prop in the motive telescope, NAME_UNKNOWN, Prop-like rule R11).
- Eval rows: elim on bagCons, on swap at i0, i1 and a neutral point, on
  quot, on quotEq at a neutral point, and on a formal hcomp (neutral, O22).

The two PLAN rows meet an older site, so they have no S7 mutant (as S6
`bag-no-squash`).

Meets. S4 elim on Sum and Unit does not change. O22 holds: elim on an
hcomp stays neutral. O25 does not change: the guest proofs do not
transport along an index line. S5 Glue: no change. Lowering: no change; a
neutral elim or a prelude value of a non-word type at a word position is
REFUSE_LOWER (lower.c:645), and Config is ghost. Guest or kit: all S7 rows
and examples are guest (no literal, no kit name). The motive telescope is
the only binder form; no universe is added.

Slices. B1 kernel O21: the family branch of rule_elim and reduce_elim,
the elim rows and the eval rows. B2 prelude: kernel/dao.lang, the front.c
splice, the SIGNATURE rows of outcomeSet and menu, the kernel_sig names,
the prelude rows. B3 guest: examples/dao.lang (laws 1 to 4, the menu, the
Charter history fold), examples/bagorbit.lang (`Orbit ≃ Bag 2`), the PLAN
S7 rows. B4 mutants, one at a time, and the records (FORMERS, SPEC,
STATUS "## S7", VALIDATION, PLAN S7 row):

| Mutant | Edit | Expected killer |
|---|---|---|
| EC | rule_elim family case count `0 *` | `elim-family-count`: ok |
| EP | path case at the constant line | `examples/dao.lang` desc, `examples/bagorbit.lang`: refused |
| ES | squash case at the constant line | `examples/bagorbit.lang`: refused |
| EV | reduce_elim on a path constructor stays stuck | the swap and quotEq eval rows |
| PJ | no dao.lang splice | `examples/dao.lang`: NAME_UNKNOWN |
| PS | kernel_sig admits a prelude name | `dao-redefine`: ok |
| OS | slot_ok admits any outcomeSet | `outcome-set-type`: ok |
| MN | slot_ok admits any menu | `menu-type`: ok |
| HIW | S6 open item: check.c:2865 (check.c:3132 at 9c7b033, the B4 run) `if (0 * (bs[i].body == NULL))` | fronttool `hiit-wrong-index`: TYPE_MISMATCH (survey H prediction, a code kill as BO; the run confirms or records it) |

### USER ruling (S7, 2026-10-18)

- Q1 O21: "Full elim on families". S7 builds default (a) of the builder
  call: elim on a domain family with path constructors, a motive over the
  indices and the value, PathP path cases and square squash cases. Block
  sorts stay outside elim.
- Q2: "Proof slot outcomeSet". The program gives `def outcomeSet : isSet
  Outcome` after Outcome. SIGNATURE gets a slot row, and slot_ok checks
  the type (REFUSE_SIG).
- Q3: "Slot menu". The program gives `def menu : Outcome -> Constitution`
  after the prelude. slot_ok checks the type (REFUSE_SIG). Member
  permutation invariance of an entry is O26 (S9).
- Q4: "General n". This replaces the n = 2 scope of the builder call. The
  theorem is `bagOrbit : (n : Size) -> Equiv (OrbitN n) (Bag Choice n)`,
  with the guest quotient `OrbitN n := Quot (Pos n -> Choice) (swapR n)`.
  domain.lang adds the family `Pos : Size -> Type 0` (a constructor at
  `szS n` for the first position and one that lifts `Pos n` to `Pos (szS
  n)`; no constructor at sz0). Pos is a former instance, as Size (O24), so
  O3 (a) holds, and Fin stays VIS_INTERNAL (O23).
- Q5 Group at general n: "Adjacent swap words". domain.lang adds a family
  of words of adjacent swaps over Size, and act applies a word. The orbits
  are the S_n orbits. The full permutation form `Equiv (Pos n) (Pos n)` is
  O27.

Consequences for B3. f : OrbitN n -> Bag Choice n is a Quot fold: the
quot case lists the choices in position order, the quotEq case is
induction on the word (one bag swap for each letter, under bagCons), and
the quotSquash case is bagSquash. g : Bag Choice n -> OrbitN n is elim on
Bag with the motive `fun k y => OrbitN k`: the bagCons case puts a choice
in front by a Quot fold, the swap case is a path in OrbitN at the index
`szS (szS k)` by elim on Quot into a family of propositions and the swap
at the first position, and the bagSquash case comes from isSet (OrbitN k).
The two homotopies are elim on Quot and elim on Bag (a motive over the
index and the value). S7 s9 build: the homotopies take g as a variable G
with its bagNil and bagCons rules, and `fromBagW sw` takes the swap case
as a variable sw, with `bagOrbit := bagOrbitW swapO`. The kernel compares
two closures by eta, so a comparison of two g values unfolds the swap path
and does not fit in the arena. A DAO program with Member `Pos n0`, Group
the words at n0 and act the word action has its spliced Orbit equal to
OrbitN n0 by conversion (an example row at the end of bagorbit.lang).
B3 adds Pos and the word family to
domain.lang before the examples. They are domain families at older sites,
so they add no mutant.

### Builder call (S8, 2026-10-18)

Source: new work in this lineage (O2 (a)), with one port. Survey:
~/Documents/orbit-lang-work/S8-survey.md (kernel at 049c04e). Facts: the
host has no field arithmetic (no mulmod, inverse or modexp in src/).
ecAdd, ecMul, ecPairing and ecrecover are VIS_INTERNAL (check.c:193-196)
and stay stuck in the host (opaque, eval.c:3029-3033;
test/eval/expect.txt:53). They lower to a STATICCALL to address 1, 6, 7
or 8 (lower.c:602), and a call that returns 0 reverts (evm.c:372-373).
The gate runs go-ethereum `evm` 1.14.12, so the repo has no C precompile
code (test/precompile.sh:2-9) and no modexp op (ir.h:126). hash is
keccak256 of two U256 words (check.c:192, eval.c:1358-1372). u256Le and
u256Eq exist (check.c:181-182). A Map lowers only with a word key and a
word value, at the slot keccak256(key . field) (lower.c:36, 634, 664).
Entry parameters are words, at most 32 (check.h:8, lower.c:214).
check_sig skips a name that is not of ORIGIN_PROGRAM (check.c:3728), so
a kernel splice can give the six primitive names (check.c:3617-3630)
with no change to SIGNATURE. `langc prove` does not exist
(main.c:40-47). No sibling has C Fp or G1 code or a ring signature.
glue-lang src/front/fr.c at cded21d (last changed in 4f0e083) has BN254
Fr add, sub, mul and the Fermat inverse on 8 limbs of 32 bits
(fr.c:8-11, 118-185), MIT OR Apache.

Scheme (O10 (a)): bLSAG over BN254 G1. For the ring P_1 to P_n with
H_i := H_p(P_i), the signer j with secret key x (P_j = x G) gives the key
image I = x H_j, c_1 and s_1 to s_n. The verifier computes
L_i = s_i G + c_i P_i, R_i = s_i H_i + c_i I and
c_(i+1) = hash chain of (tag, D, poll id, choice, I, L_i, R_i), and
accepts iff c_(n+1) = c_1. D is the ring digest: the compiler computes it
once as the hash chain of P_1 to P_n, and the contract stores it with the
H_i table, so a cast binds its ring (O30). ecMul takes any 256-bit
scalar, so the chain needs no reduction mod the group order r on chain.
The host prover computes s_j as α minus c_j x, mod r (Fr). Facts that
bind the design:

- H_p must have no known discrete log to G. H_p(P) = hash(P) G is
  refused: then I = hash(P) P links each ballot to its member.
- H_p must be canonical. With a y or a counter from the signer, one
  member can make two key images (y or p minus y, a later counter). So
  the cast takes no H_p data: the compiler computes each H_i once by try
  and increment with the host square root (p is 3 mod 4), and the
  contract stores the table (O14: the ring is fixed at deployment).
- A point not on the curve reverts at 0x06 (precompile.sh:57-58) and,
  by EIP-196 (no gate row yet), at 0x07. ecMul by 0 gives (0, 0) with no
  revert (precompile.sh:54-56). So the verifier refuses I = (0, 0) and
  an s_i at or above r (u256Le). It does not range check c_1: c_1 must
  equal c_(n+1), a keccak256 word, and about 81% of such words are at or
  above r (r / 2^256 is about 0.19), so that check refuses most valid
  casts. A word c_1 + r gives the same chain (ecMul works mod r), so the
  compare refuses it.
- A cast takes the choice, c_1, s_1 to s_n and I: n + 4 words, so the
  ring size is at most 28 (O28).

Carriers (O3 (a) and O11, ruled S1). Ballot_priv and ZKWitness are core
types with guest-visible names and internal carriers: no guest
constructor and no guest eliminator. The guest gets values only from
castPrivate and proveZK, and reads them only by project and verifyZK. A
ballot is (member index, choice, poll id) with the key of that member; a
witness is (choice, poll id, c_1, s_1 to s_n, I). The member index of a
closed Member value is its place in the constructor order (Sum: inl
first; Pos: the first position is 0). Test keys are x_i = hash(tag, i)
mod r. Deterministic scalars hash (tag, x, D, poll id, choice, i, role),
with role 0 for the private signer nonce and role 1 for public decoy
responses. The ring digest and separate roles prevent private key recovery
when the same vote is signed in different rings or positions. Each vector
is reproducible (test only, docs/VALIDATION.md). verifyZK w t
is a decidable family (O11): at a closed w it reduces to Unit when the
host check passes, else it stays neutral. So `refl : Path U (verifyZK
(proveZK b) t) Unit` is the completeness row of law 5 on a test ring,
and a wrong witness gives PATH_END. In S8 the check does not read t: the
one-use part is the run time map, and the guest one-use law is O13 (S9).
The six names come from a kernel splice after Member, Choice and the DAO
prelude, as orbit_after_act does (front.c:53-86).

Lowering. verifyZK lowers as a kit definition (VIS_INTERNAL) over ecAdd,
ecMul, hash, u256Le and u256Eq, unrolled for the ring size n of the
program (n is fixed at compile time). A kit-mode example
examples/zkcast.lang has the state (the ring, the H_i table, the poll id,
the nullifier map, the counts) and the entry `cast`, which reverts on a
failed check or a used key image. S9 writes the section 7 contract from
the DAO declaration over the same definition.

Gate rows. Known-answer: eval rows k G for k in 1, 2, 3, r minus 1 and r,
equal to the go-ethereum 0x07 words (twoGx and twoGy,
precompile.sh:48-49); H_p rows for the test ring; bLSAG vectors for
n = 2 and n = 3; an eval row of ecMul at a point not on the curve (a
trap). Check rows: the completeness row and `zk-flip` (the witness of a
ballot for one choice, read for the other choice: PATH_END). EVM rows in
test/precompile.sh (an old harness): a valid cast, then the PLAN
mutants as revert rows (wrong ring member, reused key image, flipped
choice, wrong poll id, point not on the curve: an I off the curve, so
the 0x07 refusal gets a gate row), the row I = (0, 0), and the row s_1 + r: a valid cast with
s_1 + r in place of s_1 (2r < 2^256), on a state with no used key image.
That cast passes the chain, so only the range test refuses it. A row
with s_1 = r fails the compare too, so it does not kill RG. The row
I = (0, 0) also fails the compare (each R_i changes), so it has no S8
mutant.

Slices. B1 host curve: the C port (Q1), G1 affine add, double, scalar
mul and the on-curve test, host ecAdd and ecMul compute
(test/eval/expect.txt:53 changes to the 2G words), the known-answer
eval rows. B2 kernel and host prover: the core types and ops, the
splice, H_p, the test keys, castPrivate, proveZK, project and verifyZK
in the host, the check rows, and `langc prove` (Q3). B3
lowering: the verify definition, examples/zkcast.lang, the nullifier map
(Q4), the EVM rows. B4 mutants, one at a time, and the records (FORMERS,
SPEC, STATUS "## S8", VALIDATION, PLAN S8 row):

| Mutant | Edit | Expected killer |
|---|---|---|
| FP | Fp mul without the final subtract | eval k G rows: wrong words |
| ON | the host on-curve test returns 1 | eval row of a point not on the curve: no trap |
| HP | the H_p counter starts at 1 | eval H_p row |
| CH | the verifier drops the compare c_(n+1) = c_1 | EVM wrong ring member: no revert |
| NU | `cast` skips the nullifier read | EVM reused key image: no revert |
| CC | the choice is not in the challenge | EVM flipped choice: no revert; check row `zk-flip`: ok |
| PI | the poll id is not in the challenge | EVM wrong poll id: no revert |
| RG | the s range test admits any word | EVM s_1 + r: no revert |

The EVM row for a point not on the curve meets an older site (the
precompile), so it has no S8 mutant (as S6 `bag-no-squash`); ON covers
the host side.

### USER ruling (S8, 2026-10-18)

- Q1 curve code: "Port fr.c, host computes". B1 ports glue-lang
  src/front/fr.c at cded21d with the modulus as a parameter (Fp and Fr),
  writes G1 add, double, scalar mul and the on-curve test as new C, and
  makes host ecAdd and ecMul compute. ecPairing and ecrecover stay
  opaque. 0x05 stays out of S8.
- Q2 key image: "Static H_p table". The compiler computes each H_i once,
  the contract stores the table, and I = x H_i. One member's ballots link
  across polls (O29).
- Q3 scope: "Prover + langc prove in S8". S8 gives the host prover and
  verifier, the known-answer vectors, the verify lowering, the nullifier
  map and a thin `langc prove` that writes the cast calldata (O18). The
  section 7 contract writer stays in S9.
- Q4 nullifier: "Map keyed with the poll". The state field
  `used : Map U256 Flag`, keyed by hash (hash I.x I.y) pollId, at the
  Solidity slot keccak256(key . field) (O13).

### USER ruling (S8 session 3, 2026-10-18)

- Q5 poll id: "Poll id 0 in host". castPrivate : Member -> Choice ->
  Ballot_priv, as before. The guest has no poll id (O3 (a)), so the host
  ballot has poll id 0 (the constructor stores 0, section 7). `langc
  prove` takes the poll id as a command argument. S9 binds the poll id of
  the contract.
- Q6 zk-flip: "verifyZK takes the choice". verifyZK w c t, as
  `cast(choice, ring signature, key image)` of section 7: the check reads
  the choice from c, so a witness is (poll id, c_1, s_1 to s_n, I). The
  completeness row is `refl : Path U (verifyZK (proveZK b) (project b)
  t) Unit`, and the zk-flip row gives the other choice as c (PATH_END).
  This changes the signature of verifyZK in section 6 and O11 (the B4
  records).
- B1 g1_mul: "Keep Jacobian". The double and add loop keeps a Jacobian
  point (one inverse per product). One ecMul is 0.015 s under tcc.

### USER ruling (S9 session 2, 2026-10-20)

- Q-S9-1 O13 and O26: "(a) S10 with law 6". O13 (the guest one use law)
  and O26 move to S10. Until O26 lands, docs/VALIDATION.md carries the
  member permutation invariance of each menu entry as an assumption
  (beside O29 and O30), and B2 compares one permuted config per histogram
  as a witness.
- Q-S9-2 chain probe: "(a) read only eth_call probe". B4 runs by hand
  from probe/, outside the gate: an eth_call of 0x06, 0x07 and 0x08 with
  the S2 vectors and an eth_call create of the written contract (a state
  override for the cast path). The result is a dated row in
  docs/VALIDATION.md. The USER names the RPC URLs and says go before any
  network call (not named on 2026-10-20).
- Q-S9-3 ring at deploy: "(a) ring closed in code". The writer closes the
  ring in code, from the host key rule (member i is zkKey i G, the ring
  `langc prove` signs over) or from `langc contract FILE --ring X1 Y1 ...
  Xn Yn` (a point off bn254 is refused). START comes from `--start O` (an
  Outcome constructor index, default 0). The section 7 constructor (ring,
  charter index) reads as build arguments, not calldata.
- Q-S9-4 readings: "all recommended". (i) While casts < n, outcome()
  returns m (no outcome) and amend() reverts. (ii) selfConstituting() is 0
  before the first amend (prior is no entry). (iii) Two menu entries are
  one constitution id when their histogram tables are equal.
- Q-B3b-1: "yes". test/dispatch.sh:17 reads `od -v -An -tx1`, so a
  repeated 16 byte line of a selector text is not folded into `*`.

## 9. Open items

USER ruling (S1, 2026-10-16): every default below is RULED; where an item
has options, (a) is the ruling. O17 is now a USER ruling, not a default.

- O1 Name: orbit-lang, after the orbit quotient that carries anonymity.
- O2 Kernel route: (a, default) the sealed-lang lineage (S3, S4 cherry-pick;
  S5 coin-lang merge; S6 new). (b) the zkpolicy-lang lineage (c8cd8f8,
  f7da09c, 89cc8ee, 94a0b2a, 701f11d, 243bcd8, dab441b, 99887b3, a1780a9,
  f176ac6), which has Glue and indexed path families, but each commit needs
  a three-way merge onto this kit, and the sealed-lang EVM erasure must then
  be ported again.
- O3 Base span: (a, default) strict: kit word types are lowering carriers
  only. (b) substrate: admit Nat, U256, Addr, Flag and Map as guest types,
  as sealed-lang did. (b) is less work, but it adds base types that the
  request does not admit.
- O4 Signature kinds: the slot, defined and primitive split of section 3.
- O5 Group: (a, default) the Group slot takes any action. The run time
  tally forgets members, so each constitution must also be invariant under
  member permutation; the compiler checks this. The examples give both the
  reference neutrality group (Z2 flip, S_q) and member permutation (S_n,
  as `Member ≃ Member`). (b) S_n only. (c) choice relabelling only.
- O6 Constitution: Σ of a rule and its invariance proof; on chain a finite
  compiled menu.
- O7 Aggregation: `Orbit → Outcome`, not indexed by F. With a set Outcome,
  the type of lifts of one F is a proposition, so the unique case (AD) is
  automatic.
- O8 Regime fork: the reference counts object-distinct Lans into an
  Indiscrete target, and that count collapses univalently. (a, default)
  count the self-constituting constitutions of a phase set (for β > 1 the
  fixed points 0 and ±(β-1)/β give several). (b) take Outcome as a 1-type,
  so that the lifts of one F form a set with two elements ("two distinct
  paths" in the design).
- O9 β: a pair of former-built Nat values. The design's "path witnessing
  the bifurcation at β_c = 1" has no default form yet.
- O10 ZK scheme: (a, default) bLSAG ring signature over BN254, prover in the
  host. (b) Groth16 (Semaphore form), verifier on chain over 0x06 to 0x08,
  proofs only as fixtures from an external prover, so proveZK is not
  evaluable in the host. USER ruling S8: bLSAG with a static H_p table
  that the compiler computes (section 8, Q2).
- O11 verifyZK: a decidable family; "≡ 1" is a Path in U to Unit. USER
  ruling S8 (Q6): `verifyZK w c t` reads the choice from c (the check row
  `zk-flip` is PATH_END); t stays unread in S8.
- O12 Privacy equivalence: the Ballot_pub reading of section 6.
- O13 Nullifier: the key image; the one-use law as a proposition. USER
  ruling S8 (Q4): at run time `used : Map U256 Flag`, keyed by hash (hash
  I.x I.y) pollId. The guest one-use law is S10, with law 6 (USER ruling
  S9 session 2, Q-S9-1).
- O14 Membership: a fixed ring at deployment; no join and no leave.
- O15 Contract surface: (a, default) written by the compiler from the DAO
  declaration. (b) guest-written entries in the kit `state` syntax, which
  needs the kit word types (O3 b). S9 B1: `langc contract FILE` writes the
  kit program; the ring and START are build arguments (USER ruling S9
  session 2, Q-S9-3 (a)).
- O16 Chains: Ethereum mainnet has 0x05 to 0x08. Telcoin Network: confirm
  the precompile set and the EIP-170 limit from the chain spec or an RPC
  probe. The repo gate runs only the kit EVM runner. USER ruling S9
  session 2 (Q-S9-2 (a)): a read only eth_call probe from probe/, outside
  the gate, on RPC URLs the USER names.
- O17 Proof irrelevance: PROHIBITED (USER ruling 2026-10-16). No Prop-like
  types of any kind (section 1).
- O18 Off-chain commands: `langc prove` writes the cast calldata; `langc
  run` evaluates ballots and tallies in the host. USER ruling S8 (Q3):
  `langc prove` is in S8.
- O19 Scope of the claim: orbit-lang proves the laws on the quotient
  reading. It does not claim the category-level Lean statements beyond the
  discrete case of U/Characterization.lean:81.
- O20 HIIT and QIIT examples: Bag and Charter of section 5. USER ruling
  (S6): Bag is indexed by the domain family Size, not Nat.
- O21 Dependent elimination on a family with path constructors: the
  cases of the path constructors are PathP over the motive. Law 2
  uniqueness ("Orbit induction into a set", section 3) needs it, so S7
  needs it at the latest. S4 refuses elim on S1, Trunc1, Trunc0, Quot
  and Orbit. S6 folds are non-dependent and do not give O21 (USER ruling
  S6: O21 and `Orbit ≃ Bag n` in S7). USER ruling S7: S7 builds elim
  on domain families with path constructors (PLAN section 8, Q1); block
  sorts stay outside elim.
- O22 elim on a neutral scrutinee stays neutral in S4 (no comp in the
  motive). A Sum or Unit scrutinee is never a formal hcomp: `blocked`
  (src/front/eval.c) makes elim on an `hcomp_stuck` value neutral before
  the formal hcomp test of `reduce_elim`, and that test meets only a value
  of a type with path constructors, which elim refuses (O21). A later
  slice adds the Kan rule for elim or keeps the neutral form. The lowerer
  refuses a neutral elim at a word position (REFUSE_LOWER, src/lower.c).
  S5: O22 still holds with hcomp at U and at Glue (builder call S5), and
  an eval row pins it.
- O23 Fin is internal (builder call S5, ruled by O3 (a)): Fin, finAdd and
  finNeg have VIS_INTERNAL, because `Fin n` takes a Nat and its elements
  are Nat literals. A guest form of a finite type on the span (for S_q
  on Choice, O5) is a later slice. USER ruling S7 (Q4, general n): the
  domain family `Pos : Size -> Type 0` is the guest finite type of the
  members. Fin stays VIS_INTERNAL.
- O24 S6 index family: Size (sz0, szS) in domain.lang is the index of Bag
  (USER ruling S6). It is a former instance, not a base type, so O3 (a)
  holds.
- O25 transp at an indexed family along an index line that is not
  constant stays neutral in S6, as O22. S7 or later rules a reduction.
- O26 Member permutation invariance of a menu entry (O5 (a)): the run
  time tally forgets members, so each entry must also be invariant under
  member permutation. S10 checks it, with `Orbit ≃ Bag n` when Group is
  the member permutation group (USER ruling S7, Q3), with law 6 (USER
  ruling S9 session 2, Q-S9-1). Until then docs/VALIDATION.md carries it
  as an assumption, and the S9 histogram table reads F at one canonical
  config per histogram.
- O27 Group at general n: S7 uses words of adjacent swaps (USER ruling S7,
  Q5). The full permutation form `Equiv (Pos n) (Pos n)` needs each
  permutation split into swaps (decidable equality on Pos n): a later
  slice.
- O28 Ring size (builder call S8): a cast takes the choice, c_1, s_1 to
  s_n and the key image, n + 4 words, and an entry takes at most 32
  words (check.h:8). So a ring has at most 28 members. A larger ring
  needs a calldata array: a later slice.
- O29 Link across polls (USER ruling S8, Q2): the key image I = x H_i
  does not change with the poll, so the ballots of one member in two
  polls carry the same I. The member stays hidden in the ring, but the
  link is an assumption in docs/VALIDATION.md. A per-poll H_p is a later
  slice.
- O30 Replay across deployments (review S8): the survey default (item 9)
  put the contract address in the challenge, but the kernel has no op
  for the address of the contract. The ring digest D binds the ring
  only, so a cast replays on a second deployment with the same ring and
  poll id. This is an assumption in docs/VALIDATION.md. An address op is
  a later slice.
