# orbit-lang domain design

> Paste the domain design below the line, verbatim. Do not edit it.

This file is the only source of the core types and the core operations of
orbit-lang. `SPEC.md` sections 5 and 6 cite its sections. Do not add a core
type or a core operation that this file does not give.

When the design changes, paste the new version. Then record the change as an
open item in `SPEC.md` section 9.

---

## The request (verbatim, 2026-10-16)

I want to write a new programming language with the following features:

Type constructors/higher types includes are products, coproducts, monads (map/bind), algebras/coalgebras (fold/unfold), quotients/paths/loops/HITs/QITs/HIITs/QIITs as in Cubical TT

The only base types and operations allowed are as described in the following (however, the laws should solve as a minimal, but not complete, list):

## The pasted design (verbatim)

**Minimal span in Cubical TT (CCHM-style) for a zero-knowledge self-referential DAO**, following the denotational design in [MavenRain/self-referential-dao](https://github.com/MavenRain/self-referential-dao).

The reference treats a DAO not as storage or a contract, but as the universal lift of anonymous local choices along the voter-orbit quotient:

```text
⟦DAO⟧ = Aggregation act F ≔ LeftKanExtension (orbitProjection act) F
```

Self-reference is the fixed point of the governance endo-operation

```text
Gov L ≔ orbitProjection act ⋙ L.functor
IsSelfConstituting F ≔ ∃ L. ∀ X. (Gov L).obj X ≡ F.obj X
```

Anonymity and legitimacy are inherited from the orbit quotient and the Lan universal property (unit, descent, uniqueness). The three UAT regimes (Arrow-Impossibility / Arrow-Debreu / Schelling-Ising) classify the cardinality of such aggregations. The original design has no cryptographic ZK; the Assay/EVM port is public. We therefore add a minimal ZK layer for private ballots while preserving the categorical core.

Cubical TT supplies Path types (equalites as functions out of the interval \(I\)), path abstraction \(\langle i\rangle t\), path application, composition/filling (Kan operations), and Glue for equivalences. We work in a universe \(U\) and write Path \(A\) \(a\) \(b\) (or \(a\equiv b\)) for paths.

### Types

```text
Member, Choice, Outcome, Group : U

Config      : U          -- configurations of local choices
Orbit       : U          -- orbit space of Config under the group action
Constitution : U         -- choice rules (roughly Config → Outcome, or functors)
Aggregation  : U         -- the semantic domain (realizations of the Lan)
ZKWitness    : U         -- zero-knowledge witness of a well-formed private ballot
Tally        : U         -- public aggregate (outcome of Aggregation)
Ballot_priv  : U         -- private ballot (commitment + choice + nullifier / proof data)
```

### Operations

```text
orbitProj   : Config → Orbit
act         : Group → Config → Config          -- group action
aggregate   : Constitution → Aggregation
gov         : Aggregation → Constitution
amend       : Aggregation → Aggregation        -- self-amendment (⟦amend⟧ = gov)
castPrivate : Member → Choice → Ballot_priv
proveZK     : Ballot_priv → ZKWitness
verifyZK    : ZKWitness → Tally → U            -- verification predicate (or a path type)
project     : Ballot_priv → Choice             -- public choice projection
```

(The Kan structure of Aggregation is packaged inside `aggregate`; the unit and descent of the Lan are the mediating paths below.)

### Laws

1. **Anonymity (orbit invariance)**  
   For all \(g : \mathsf{Group}\), \(c : \mathsf{Config}\),
   ```text
   ⟨i⟩ aggregate F (act g c)  ≡  aggregate F c     : Path Tally …
   ```
   (or the stronger statement that the constitution is constant on orbits: \(F.(\mathsf{act}\, g\, x) \equiv F.x\)).

2. **Legitimacy (Lan universal property)**  
   There is a unit path
   ```text
   unit : Path (Constitution → …) (F ⋙ orbitProj) (aggregate F)
   ```
   and a unique mediating path (descent) for any other candidate extension. Uniqueness is expressed by a path between any two such mediators (the cubical form of \(\mathsf{desc\_unique}\)).

3. **Self-constitution (fixed point)**  
   ```text
   IsSelfConstituting F ≔ Σ (L : Aggregation), Path Constitution (gov L) F
   ```
   Equivalently, a path \(\langle i\rangle\,\mathsf{gov}\,(\mathsf{aggregate}\, F)\, \equiv\, F\).  
   In the constant/zero-phase constitutions of the reference this path exists uniquely; in the Schelling-Ising regime the Aggregation type itself may fork (two distinct paths), while the self-constituting witness remains single-valued.

4. **Amendment homomorphism**  
   ```text
   ⟨i⟩ amend (aggregate F)  ≡  aggregate (gov (aggregate F))
   ```
   (i.e. \(\llbracket\mathsf{amend}\rrbracket = \mathsf{Gov}\)).

5. **Zero-knowledge (completeness & soundness)**  
   - Completeness: for any well-formed private ballot \(b\),
     ```text
     ⟨i⟩ verifyZK (proveZK b) (aggregate F (project b))  ≡  1
     ```
   - Soundness / privacy: verification depends only on the public projection and the tally. Using Glue (or a partial element / face restriction) one obtains an equivalence
     ```text
     Ballot_priv  ≃  Choice × ZKWitness
     ```
     such that the path is constant on the private data; extracting the private data from a verified witness is impossible (the fibre over a public choice is a contractible or empty higher type after verification).  
   - Non-malleability / one-use: a nullifier path that is unique per Member (or a higher path enforcing that two proofs for the same nullifier are path-equal).

6. **Regime trichotomy (optional but faithful to the reference)**  
   The type Aggregation is classified by a sum (or a dependent family indexed by a coordination parameter \(\beta\)):
   ```text
   Aggregation ≡ Empty          +   (unique L)   +   (L₀ × L₁)
   --            Arrow-Impossibility   Arrow-Debreu   Schelling-Ising
   ```
   with a path witnessing the bifurcation at the critical value \(\beta_c = 1\), and the law
   ```text
   ¬ IsSelfConstituting F   whenever   Aggregation ≡ Empty.
   ```

These types, operations and path-laws are the minimal cubical signature that reifies the denotational meaning of the referenced DAO (Lan along the orbit projection, governance fixed point, anonymity/legitimacy) while adding a zero-knowledge ballot layer whose privacy is expressed by path-independence and Glue equivalences. All equalities are computational paths; the Kan operations of Cubical TT supply the fillers needed for composition of amendments and transport along privacy equivalences.

## The request, continued (verbatim)

---------

Finally, the host language shall be TinyCC, and the compilation target should be EVM-deployable bytecode to any EVM chain, such as Telcoin network or mainnet Ethereum.
