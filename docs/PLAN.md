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

- HIIT: `Bag : Nat → U`, with `nil : Bag zero`,
  `cons : Choice → Bag n → Bag (suc n)`,
  `swap : cons a (cons b t) ≡ cons b (cons a t)` and a set truncation. Then
  `Orbit ≃ Bag n` for n members is a guest theorem (Glue, S5).
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
| verifyZK w t | a decidable check | ring verify, then key image unused, then tally increment; a failure reverts |

Readings of law 5 (O11 to O13):

- Completeness: `verifyZK (proveZK b) t ≡ Unit` is a Path in U through ua
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
| S4 | Orbit as a quotient of the voter orbits (the HIT, QIT and truncation formers came with the S2 port) | Orbit fold without isSet, wrong Orbit boundary, Orbit path at run time |
| S5 | Glue, ua, uaBeta, hcomp at U (coin-lang merge) | Glue boundary mismatch, ua on a non-equivalence |
| S6 | HIITs and QIITs | wrong index in a path constructor, inductive-inductive order, missing coherence case |
| S7 | DAO prelude and laws 1 to 4 as guest theorems; menu of constitutions | a non-invariant constitution is refused; a wrong gov unit is refused |
| S8 | ZK layer: BN254 G1 in C, castPrivate, proveZK, project, verifyZK lowering, nullifier map | known-answer vectors; mutants: wrong ring member, reused key image, flipped choice, wrong poll id, point not on the curve |
| S9 | contract end to end: deploy, cast, outcome, amend; host and EVM differential; gas table per ring size; chain probe | differential rows, revert rows, size limits EIP-170 and EIP-3860 |
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
  evaluable in the host.
- O11 verifyZK: a decidable family; "≡ 1" is a Path in U to Unit.
- O12 Privacy equivalence: the Ballot_pub reading of section 6.
- O13 Nullifier: the key image; the one-use law as a proposition.
- O14 Membership: a fixed ring at deployment; no join and no leave.
- O15 Contract surface: (a, default) written by the compiler from the DAO
  declaration. (b) guest-written entries in the kit `state` syntax, which
  needs the kit word types (O3 b).
- O16 Chains: Ethereum mainnet has 0x05 to 0x08. Telcoin Network: confirm
  the precompile set and the EIP-170 limit from the chain spec or an RPC
  probe. The repo gate runs only the kit EVM runner.
- O17 Proof irrelevance: PROHIBITED (USER ruling 2026-10-16). No Prop-like
  types of any kind (section 1).
- O18 Off-chain commands: `langc prove` writes the cast calldata; `langc
  run` evaluates ballots and tallies in the host.
- O19 Scope of the claim: orbit-lang proves the laws on the quotient
  reading. It does not claim the category-level Lean statements beyond the
  discrete case of U/Characterization.lean:81.
- O20 HIIT and QIIT examples: Bag and Charter of section 5.
