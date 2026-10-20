#!/bin/sh
# The run gate (kit slice K3b). `make check` runs it from the kit directory
# after test/gate.sh.
set -eu
fail=0
runs=0
tmp=${TMPDIR:-/tmp}/langc-run.$$
mkdir -p "$tmp"

# For each test/run/NAME.script, `langc run examples/NAME.lang` (examples/contract.lang
# when there is no examples/NAME.lang) must exit 0 and print test/run/NAME.out
# byte for byte.
for script in test/run/*.script; do
  prog=examples/$(basename "${script%.script}").lang
  [ -f "$prog" ] || prog=examples/contract.lang
  status=0
  build/langc --kit run "$prog" "$script" >"$tmp/out" 2>"$tmp/err" || status=$?
  runs=$((runs + 1))
  if [ "$status" -ne 0 ] || ! cmp -s "${script%.script}.out" "$tmp/out"; then
    echo "FAIL run $script: exit $status: $(head -n 1 "$tmp/err")"
    fail=$((fail + 1))
  fi
done
echo "runs: $runs checked"

# The differential (S9 B2) on the host. For each row of test/contract/diff.txt
# (`PROG START GUEST PERMUTED BALLOTS NAME... => OUTCOME SELF`), `langc --kit run`
# of the written contract `langc contract examples/PROG.lang --start START` over
# a cast of each NAME (the words of `langc prove test/contract/BALLOTS.lang NAME
# 0u`), then outcome, enact and selfConstituting, must print `I cast ok` for each
# cast, `outcome = OUTCOMEu`, `enact ok` and `selfConstituting = flagYes` when
# SELF is 1 (else flagNo). The rows are read on fd 3, so langc cannot read them.
diffs=0
dcaller=0x00000000000000000000000000000000000000aa
while read -r line <&3; do
  case $line in --*|'') continue ;; esac
  set -- ${line%% => *}
  want=${line#* => }
  diffs=$((diffs + 1))
  if ! build/langc contract "examples/$1.lang" --start "$2" >"$tmp/dao.lang" 2>"$tmp/err"; then
    echo "FAIL differential $line: contract: $(head -n 1 "$tmp/err")"
    fail=$((fail + 1))
    continue
  fi
  ballots=$5
  shift 5
  : >"$tmp/diff.script"
  : >"$tmp/diff.want"
  n=0
  for name in "$@"; do
    n=$((n + 1))
    words=$(build/langc prove "test/contract/$ballots.lang" "$name" 0u | tr '\n' ' ')
    echo "$n $dcaller cast" $words >>"$tmp/diff.script"
    echo "$n cast ok" >>"$tmp/diff.want"
  done
  flag=flagNo
  if [ "${want#* }" = 1 ]; then flag=flagYes; fi
  printf '%s\n' "$((n + 1)) $dcaller outcome" "$((n + 2)) $dcaller enact" \
    "$((n + 3)) $dcaller selfConstituting" >>"$tmp/diff.script"
  printf '%s\n' "$((n + 1)) outcome = ${want%% *}u" "$((n + 2)) enact ok" \
    "$((n + 3)) selfConstituting = $flag" >>"$tmp/diff.want"
  status=0
  build/langc --kit run "$tmp/dao.lang" "$tmp/diff.script" >"$tmp/out" 2>"$tmp/err" || status=$?
  head -n $((n + 3)) "$tmp/out" >"$tmp/diff.got"
  if [ "$status" -ne 0 ] || ! cmp -s "$tmp/diff.want" "$tmp/diff.got"; then
    echo "FAIL differential $line: exit $status: $(head -n 1 "$tmp/err") $(cmp "$tmp/diff.want" "$tmp/diff.got" 2>&1 | head -n 1)"
    fail=$((fail + 1))
  fi
done 3<test/contract/diff.txt
echo "differential: $diffs host runs checked"

# run_refused STATUS CODE PROG LINE: `langc run PROG` on the one-line script
# LINE must exit STATUS and print `langc: CODE: ...` on stderr.
refused=0
run_refused() {
  printf '%s\n' "$4" >"$tmp/bad.script"
  status=0
  build/langc --kit run "$3" "$tmp/bad.script" 2>"$tmp/err" >/dev/null || status=$?
  refused=$((refused + 1))
  case "$status:$(head -n 1 "$tmp/err")" in
    "$1:langc: $2: "*) ;;
    *) echo "FAIL run '$4': want exit $1 and $2, got exit $status: $(head -n 1 "$tmp/err")"; fail=$((fail + 1)) ;;
  esac
}
caller=0x00000000000000000000000000000000000000bb
run_refused 1 RUN_SCRIPT examples/contract.lang "1 0xbb deposit 5u"
run_refused 1 RUN_SCRIPT examples/contract.lang "x $caller deposit 5u"
run_refused 2 EVAL_ARGS examples/contract.lang "1 $caller deposit 5"
run_refused 2 EVAL_ARGS examples/contract.lang "1 $caller deposit 5u 6u"
run_refused 1 EVAL_ENTRY examples/contract.lang "1 $caller checkpoint init 7u"
run_refused 1 EVAL_ENTRY examples/contract.lang "1 $caller nope"
run_refused 1 RUN_INIT examples/entries.lang "1 $caller addPrice 1 2"
run_refused 1 EVAL_ENTRY examples/map.lang "1 $caller putAll 3"
run_refused 1 EVAL_ENTRY examples/map.lang "1 $caller allBooks"

# A NUL cannot hide extra words. Calls before the bad line keep their output.
printf '1 %s balance\n2 %s deposit 5u\000 6u\n' "$caller" "$caller" >"$tmp/bad.script"
status=0
build/langc --kit run examples/contract.lang "$tmp/bad.script" >"$tmp/out" 2>"$tmp/err" || status=$?
printf '1 balance = 0u\n' >"$tmp/want"
refused=$((refused + 1))
case "$status:$(head -n 1 "$tmp/err")" in
  "1:langc: RUN_SCRIPT: -: $tmp/bad.script:2: "*)
    if ! cmp -s "$tmp/want" "$tmp/out"; then
      echo 'FAIL run NUL: output must stop before the malformed call'
      fail=$((fail + 1))
    fi ;;
  *) echo "FAIL run NUL: want exit 1 and RUN_SCRIPT, got exit $status"; fail=$((fail + 1)) ;;
esac
echo "run refusals: $refused checked"

status=0
build/langc --kit run examples/contract.lang 2>/dev/null || status=$?
[ "$status" -eq 2 ] || { echo "FAIL run usage: want exit 2, got $status"; fail=$((fail + 1)); }
rm -rf "$tmp"
echo "run gate: $fail failures"
[ "$fail" -eq 0 ]
