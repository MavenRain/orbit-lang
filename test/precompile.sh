#!/bin/sh
# The precompile gate (slice S2, kickoff 1.5). Deploy examples/precompile.lang,
# then do one `evm run --prestate` step for each view and compare the result
# word with a constant. Each constant was confirmed at build (2026-10-15, evm
# 1.14.12) with a direct `evm run --input HEX --receiver 0x..0N` to the
# precompile address N (1 ecrecover, 6 ecAdd, 7 ecMul, 8 ecPairing) before it
# entered this file. The evaluator computes a closed ecAdd or ecMul (S8 B1):
# examples/bn254.lang gives k G in the host for each k of the mulX and mulY rows,
# with the same words. ecPairing and ecrecover stay stuck (R17). This gate
# compares the chain result only. It needs evm (go-ethereum), bc, od and
# build/slottool. Run it from the kit root.
set -u
fail=0
steps=0
tmp=${TMPDIR:-/tmp}/langc-precompile.$$
mkdir -p "$tmp"
. test/evmchain.sh

prog=examples/precompile.lang
caller=0x00000000000000000000000000000000000000aa

# call WANT NAME ARGS...: one step of the view or entry NAME with ARGS at
# TIMESTAMP 1. The result must equal WANT: `out HEX` (HEX as hexnorm), `ok` or
# `revert`. An entry changes the storage of the next step.
call() {
  _want=$1 _name=$2
  shift 2
  steps=$((steps + 1))
  _input=$(input "$prog" "$_name" "$@")
  if ! step 1 "$caller" "$_input"; then
    echo "FAIL precompile $_name: evm: $(head -n 1 "$tmp/dump")"
    fail=$((fail + 1))
    return
  fi
  case $result in
    "out "*) _hex=${result#out }
      [ ${#_hex} -eq 64 ] && result="out $(hexnorm "$_hex")" ;;
  esac
  if [ "$result" != "$_want" ]; then
    echo "FAIL precompile $_name: want $_want, got $result"
    fail=$((fail + 1))
  fi
}

if ! deploy "$prog"; then
  echo "FAIL precompile deploy: $(head -n 1 "$tmp/dump")"
  fail=$((fail + 1))
else
  # 2G on BN254 (EIP-196): ecAdd (1,2) (1,2) and ecMul (1,2) 2.
  twoGx=30644e72e131a029b85045b68181585d97816a916871ca8d3c208c16d87cfd3
  twoGy=15ed738c0e0a7c92e7845f96b2ae9c0a68a6a449e3538fc7ff3ebf7a5a18a2c4
  call "out $twoGx" addX 1u 2u 1u 2u
  call "out $twoGy" addY 1u 2u 1u 2u
  call "out $twoGx" mulX 1u 2u 2u
  call "out $twoGy" mulY 1u 2u 2u
  # ecMul (1,2) 0 is the point at infinity, (0, 0).
  call "out 0" mulX 1u 2u 0u
  call "out 0" mulY 1u 2u 0u
  # 3G, (r minus 1) G = (1, p minus 2) and r G = (0, 0), r the order of G1 (S8 B1;
  # confirmed 2026-10-18 with a direct `evm run` to 0x..07, evm 1.14.12).
  threeGx=769bf9ac56bea3ff40232bcb1b6bd159315d84715b8e679f2d355961915abf0
  threeGy=2ab799bee0489429554fdb7c8d086475319e63b40b9c5b57cdf1ff3dd9fe2261
  pMinusTwo=30644e72e131a029b85045b68181585d97816a916871ca8d3c208c16d87cfd45
  rMinusOne=0x30644e72e131a029b85045b68181585d2833e84879b9709143e1f593f0000000u
  rOrder=0x30644e72e131a029b85045b68181585d2833e84879b9709143e1f593f0000001u
  call "out $threeGx" mulX 1u 2u 3u
  call "out $threeGy" mulY 1u 2u 3u
  call "out 1" mulX 1u 2u "$rMinusOne"
  call "out $pMinusTwo" mulY 1u 2u "$rMinusOne"
  call "out 0" mulX 1u 2u "$rOrder"
  call "out 0" mulY 1u 2u "$rOrder"
  # (1, 1) is not on the curve: the precompile fails and the call reverts (R18).
  call revert addBad 2u
  # e(G1, G2) e(-G1, G2) = 1 (EIP-197), and the empty product is 1.
  call "out 1" pairOk
  call "out 1" pairNil
  # The go-ethereum ecrecover vector (core/vm/testdata/precompiles/ecRecover.json, CallEcrecover0).
  h=0x38d18acb67d25c8bb9942764b62f18e17054f66a817bd4295423adf9ed98873eu
  sgood=0x789d1dd423d25f0772d2748d60f7e4b81bb14d086eba8e8e8efb6dcff8a4ae02u
  addr=ceaccac640adf55b2028469bd36ba501f28b699d
  call "out $addr" recover "$h" 27 "$h" "$sgood"
  # s = 0 gives an empty output: the address is 0 (R18).
  call "out 0" recover "$h" 27 "$h" 0u
  # The same ecrecover statement twice (a fold over the pushed words, the end of
  # the list first): the valid s, then s = 0. The sum is the one address (M8).
  call ok sign "$sgood"
  call ok sign 0u
  call "out $addr" recoverSum
  # keccak256 of 64 zero bytes; `langc eval examples/precompile.lang hashZero` gives the same word.
  call "out ad3228b676f7d3cd4284a5443f17f1962b36e491b30a40b2405849e597ba5fb5" hashv 0u 0u
  # leafStep i = natDiv i 2 + natMod i 2 (the parent index plus the side bit).
  call "out 4" leafStep 7
  call "out 3" leafStep 6
  call "out aa" addrWord 0x00000000000000000000000000000000000000aa
fi

# The private cast of examples/zkcast.lang (S8 B3a): the ring (p0, p1) at poll 0.
# The six words are the `dao b0` row of test/prove/expect.txt (choice, c_1, s_1,
# s_2, I.x, I.y): six refusals revert (S8 B3b), the valid cast enters, count 1
# reads 1, the same key image reverts.
prog=examples/zkcast.lang
c1=39381386088514315497347125331661662002366806678711758872472619958305300356361u
s1=10556691046619535408078082753355190806958972782948105030448836431921968396625u
s2=5075046572376896310100317466143363171635445558286582457308097647558945948934u
ix=10761449505403964036076757638925780734653286855769319211451479689235022075489u
iy=7813043007448227325775284969956458831967279497483528366357925175793623591926u
# The refusal words: bad1 to bad5 = c_1, s_1, s_2, I.x, I.y of a signature by
# zkKey 2 over the ring (p0, p2) at poll 0 (the host refuses it on the cast
# ring); p71 to p75 = the b0 member signed at poll 7u; s1r = s_1 + r.
bad1=114233635948204567937611136005807964635379552110520686947477480399687734882175u
bad2=4606903119986625213584159387736508097390161799663041052171944740195199651320u
bad3=19405452480297225842144819244991036144939704859909575586366782468791467929709u
bad4=12143800741274053729678695342025405071161578280145823471976847353119162962133u
bad5=5478805016161928785310155891281398518466491099139863695584469452713860577342u
p71=115326832689243201939208783265390243471483834046477764896127408546428269123821u
p72=6848704374829197146497408100537308754776627187049601801861139456475064325019u
p73=5324371756747053231067432293918824366492735010707968715625973090356803969703u
p74=10761449505403964036076757638925780734653286855769319211451479689235022075489u
p75=7813043007448227325775284969956458831967279497483528366357925175793623591926u
s1r=32444933918458810630324488498612465895507337183364139374147040618497776892242u
if ! deploy "$prog"; then
  echo "FAIL zkcast deploy: $(head -n 1 "$tmp/dump")"
  fail=$((fail + 1))
else
  # Six refusals before the valid cast: the entry checks the used key first, so
  # a vector with the b0 key image placed after the cast would revert for the key.
  # A signer outside the ring (p0, p1): the hash chain does not close.
  call revert cast 1u "$bad1" "$bad2" "$bad3" "$bad4" "$bad5"
  # The b0 words with the choice flipped to 0: the choice is in the hash.
  call revert cast 0u "$c1" "$s1" "$s2" "$ix" "$iy"
  # The b0 member signed at poll 7u: the poll id is in the hash.
  call revert cast 1u "$p71" "$p72" "$p73" "$p74" "$p75"
  # I = (1, 1) is not on bn254: the ecMul precompile returns 0, the code reverts.
  call revert cast 1u "$c1" "$s1" "$s2" 1u 1u
  # I = (0, 0): the precompile takes infinity, the chain fails, the last flag refuses.
  call revert cast 1u "$c1" "$s1" "$s2" 0u 0u
  # s_1 + r: the precompile reduces mod r and the chain closes; u256Le s_1 rmax refuses.
  call revert cast 1u "$c1" "$s1r" "$s2" "$ix" "$iy"
  call ok cast 1u "$c1" "$s1" "$s2" "$ix" "$iy"
  call "out 1" count 1u
  call revert cast 1u "$c1" "$s1" "$s2" "$ix" "$iy"
fi

# The written DAO contract (S9 B1): the text of `langc contract examples/dao.lang`.
# The cast words come from `langc prove test/contract/ballots.lang NAME POLL`
# (choice, c_1, s_1, s_2, I.x, I.y): the prove ring depends only on the Member
# cardinality, 2 as in examples/dao.lang. Before 2 casts the outcome is 2 (no
# outcome). m0 votes 0 and m1 votes 1, so agree0 gives outcome 1; enact adopts
# constitution 1 and opens poll 1, where the poll 0 words of m0 revert.
build/langc contract examples/dao.lang >"$tmp/dao.lang"
prog="$tmp/dao.lang"
ballot() {
  build/langc prove test/contract/ballots.lang "$1" "$2" | tr '\n' ' '
}
if ! deploy "$prog"; then
  echo "FAIL contract deploy: $(head -n 1 "$tmp/dump")"
  fail=$((fail + 1))
else
  m0p0=$(ballot m0c0 0u)
  m0p1=$(ballot m0c0 1u)
  m1p0=$(ballot m1c1 0u)
  m1c2=$(ballot m1c2 0u)
  call "out 2" outcome
  call revert enact
  call "out 0" selfConstituting
  call ok cast $m0p0
  call revert cast $m0p0
  # A valid signature over choice 2: the contract has 2 choices.
  call revert cast $m1c2
  call ok cast $m1p0
  call "out 1" count 0u
  call "out 1" count 1u
  call "out 1" outcome
  call ok enact
  call "out 1" poll
  call "out 1" constitution
  call "out 0" selfConstituting
  call revert cast $m0p0
  call ok cast $m0p1
  call "out 1" count 0u
fi

# The differential (S9 B2) on the EVM: for each row of test/contract/diff.txt,
# the calls of test/run.sh on a new deploy of the written contract `langc
# contract examples/PROG.lang --start START`. The SELF word is 1 for flagYes and
# 0 for flagNo. The rows are read on fd 3, so evm and langc cannot read them.
diffs=0
while read -r line <&3; do
  case $line in --*|'') continue ;; esac
  set -- ${line%% => *}
  want=${line#* => }
  diffs=$((diffs + 1))
  if ! build/langc contract "examples/$1.lang" --start "$2" >"$prog" || ! deploy "$prog"; then
    echo "FAIL differential deploy $line: $(head -n 1 "$tmp/dump")"
    fail=$((fail + 1))
    continue
  fi
  ballots=$5
  before=$fail
  shift 5
  for name in "$@"; do
    call ok cast $(build/langc prove "test/contract/$ballots.lang" "$name" 0u | tr '\n' ' ')
  done
  call "out ${want%% *}" outcome
  call ok enact
  call "out ${want#* }" selfConstituting
  [ "$fail" -eq "$before" ] || echo "FAIL differential row: $line"
done 3<test/contract/diff.txt
echo "differential: $diffs EVM rows checked"

rm -rf "$tmp"
echo "precompile: $steps steps checked"
echo "precompile gate: $fail failures"
[ "$fail" -eq 0 ]
