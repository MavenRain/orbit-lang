# orbit-lang

orbit-lang is a Cubical TT language for a zero-knowledge self-referential
DAO. Its meaning follows MavenRain/self-referential-dao: a DAO is the left
Kan extension of a choice rule along the voter-orbit quotient, and
self-reference is a fixed point of the governance operation. orbit-lang adds
private ballots with a zero-knowledge check. The host is TinyCC. The
compiler writes EVM bytecode that deploys to any EVM chain, for example
Telcoin Network or Ethereum mainnet.

orbit-lang comes from lang-template with the tcc-evm-contract host.

- `design/DESIGN.md`: the request and the domain design, verbatim. It is the
  only source of the core types and the core operations.
- `docs/PLAN.md`: the rulings, the reading of the design on the kit, the
  kernel route, the zero-knowledge layer, the contract, the slices and the
  open items.
- `SPEC.md`: the specification. Fill each section.
- `formers/FORMERS.md`: the type formers. `formers/tcc-evm-contract.md` gives their
  status on the tcc-evm-contract host.
- `probe/CAPABILITY.md`: the host probe.
- `docs/STATUS.md` and `docs/VALIDATION.md`: the status and the gate results.
- `docs/host/README.md`: how to build and test the host kit, and how to
  replace the sample domain in `domain/`.
