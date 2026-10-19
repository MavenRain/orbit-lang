/* Domain declaration regressions use the same parser and checker as langc. */
#include <stdio.h>
#include <string.h>

#include "front/check.h"
#include "front/lexer.h"
#include "front/parser.h"

/* checked parses TEXT with ORIGIN and checks it in MODE. A domain row is
   trusted text in kit mode; a guest row (orbit-lang S3) is program text in
   guest mode. */
static int checked(const char *text, Origin origin, CheckMode mode, const char *want) {
  Arena arena;
  Diag diag;
  TokenList tokens = {0};
  DeclList decls = {0};
  Machine machine;
  arena_init(&arena, (size_t)1 << 24);
  diag_init(&diag);
  int ok = lex_source(&arena, "test-domain", text, strlen(text), &tokens, &diag)
    && parse_source(&arena, "test-domain", origin, &tokens, &decls, &diag)
    && check_program(&arena, &decls, &machine, &diag, mode);
  int passed = want == NULL ? ok : !ok && diag.code != NULL && strcmp(diag.code, want) == 0;
  if (passed && want == NULL && machine.has_state) {
    const CtorInfo *ci = &machine.ctors[machine.families[machine.state_family].first_ctor];
    for (uint32_t i = 0; i < ci->field_count; i++)
      passed &= eval_core(&machine, NULL, ci->fields[i].type) != NULL;
  }
  if (!passed) {
    fprintf(stderr, "FAIL %s: wanted %s, got %s: %s\n", mode == CHECK_KIT ? "domain" : "guest",
            want == NULL ? "ok" : want, ok ? "ok" : diag.code == NULL ? "no diagnostic" : diag.code, text);
  }
  arena_release(&arena);
  return passed;
}

static int domain(const char *text, const char *want) {
  return checked(text, ORIGIN_DOMAIN, CHECK_KIT, want);
}

static int guest(const char *text, const char *want) {
  return checked(text, ORIGIN_PROGRAM, CHECK_GUEST, want);
}

static int normal_roundtrip_in(const char *domain_text, const char *type, const char *body) {
  char source[4096];
  char normal[2048];
  Arena arena;
  Diag diag;
  TokenList tokens = {0};
  DeclList decls = {0};
  Machine machine;
  snprintf(source, sizeof source, "%s\ndef value : %s := %s", domain_text, type, body);
  arena_init(&arena, (size_t)1 << 24);
  diag_init(&diag);
  int ok = lex_source(&arena, "test-roundtrip", source, strlen(source), &tokens, &diag)
    && parse_source(&arena, "test-roundtrip", ORIGIN_DOMAIN, &tokens, &decls, &diag)
    && check_program(&arena, &decls, &machine, &diag, CHECK_KIT);
  if (ok)
    ok = value_print(&machine, NULL, 0, def_value(&machine, 0), normal, sizeof normal);
  if (!ok) {
    fputs("FAIL normal roundtrip: ", stderr);
    diag_print(&diag, stderr);
  }
  arena_release(&arena);
  if (!ok)
    return 0;
  snprintf(source, sizeof source, "%s\ndef value : %s := %s", domain_text, type, normal);
  return domain(source, NULL);
}

static int normal_roundtrip(const char *type, const char *body) {
  return normal_roundtrip_in("", type, body);
}

static int zk_trap(const char *type, const char *body) {
  char source[2048];
  Arena arena;
  Diag diag;
  TokenList tokens = {0};
  DeclList decls = {0};
  Machine machine;
  const Value *value = NULL;
  snprintf(source, sizeof source,
           "def max : U256 := 115792089237316195423570985008687907853269984665640564039457584007913129639935u\n"
           "def bad : U256 := u256Add max 1u\n"
           "def value : %s := %s", type, body);
  arena_init(&arena, (size_t)1 << 24);
  diag_init(&diag);
  int ok = lex_source(&arena, "test-zk-trap", source, strlen(source), &tokens, &diag)
    && parse_source(&arena, "test-zk-trap", ORIGIN_DOMAIN, &tokens, &decls, &diag)
    && check_program(&arena, &decls, &machine, &diag, CHECK_KIT);
  if (ok)
    value = def_value(&machine, 2u);
  ok = ok && value != NULL && value->kind == VAL_TRAP;
  if (!ok)
    fprintf(stderr, "FAIL zk nested trap: %s\n", body);
  arena_release(&arena);
  return ok;
}

static int zk_kernel(const char *member_type, const char *member,
                     const char *choice_type, const char *choice,
                     const char *checked_choice, const char *want) {
  char source[4096];
  snprintf(source, sizeof source,
           "family Size := sz0 | szS (szPred : Size)\n"
           "family Pos : Size -> Type 0 := posFirst (firstSize : Size) : Pos (szS firstSize)\n"
           "  | posNext (nextSize : Size) (nextPos : Pos nextSize) : Pos (szS nextSize)\n"
           "def M : Type 0 := %s\n"
           "def C : Type 0 := %s\n"
           "def b : ZkBallot := zkCast M C (%s) (%s)\n"
           "def complete : Path (Type 0) (zkCheck M C (zkProve b) (%s)) Unit := refl\n",
           member_type, choice_type, member, choice, checked_choice);
  return domain(source, want);
}

int main(void) {
  int ok = 1;
  /* Projection only reads the stored choice, including under binders. */
  ok &= domain("def projected : (M : Type 0) -> (C : Type 0) -> (m : M) -> (c : C) -> "
               "Path C (zkProject C (zkCast M C m c)) c := "
               "fun (M : Type 0) (C : Type 0) (m : M) (c : C) => refl", NULL);
  /* A choice index must not wrap to that of a different constructor. */
  ok &= zk_kernel("Unit", "unit", "Sum (Fin 18446744073709551615) (Sum Unit Unit)",
                  "inl 0", "inr (inr unit)", "PATH_END");
  ok &= zk_kernel("Unit", "unit", "Sum (Fin 18446744073709551615) Unit",
                  "inr unit", "inr unit", NULL);
  /* Pos is the member type of the bag-orbit DAO, with zero-based positions. */
  ok &= zk_kernel("Pos (szS sz0)", "posFirst sz0", "Sum Unit Unit",
                  "inl unit", "inl unit", NULL);
  ok &= zk_kernel("Pos (szS (szS (szS sz0)))", "posNext (szS (szS sz0)) (posNext (szS sz0) (posFirst sz0))",
                  "Pos (szS (szS sz0))", "posNext (szS sz0) (posFirst sz0)",
                  "posNext (szS sz0) (posFirst sz0)", NULL);
  ok &= zk_kernel("Pos (szS (szS (szS sz0)))", "posNext (szS (szS sz0)) (posNext (szS sz0) (posFirst sz0))",
                  "Pos (szS (szS sz0))", "posNext (szS sz0) (posFirst sz0)",
                  "posFirst (szS sz0)", "PATH_END");
  ok &= domain("family Size := sz0 | szS (szPred : Size)\n"
               "family Pos : Size -> Type 0 := posFirst (firstSize : Size) : Pos (szS firstSize)\n"
               "  | posNext (nextSize : Size) (nextPos : Pos nextSize) : Pos (szS nextSize)\n"
               "def sameEncoding : Path ZkWitness "
               "(zkProve (zkCast (Pos (szS (szS (szS sz0)))) (Pos (szS (szS sz0))) "
               "(posNext (szS (szS sz0)) (posNext (szS sz0) (posFirst sz0))) "
               "(posNext (szS sz0) (posFirst sz0)))) "
               "(zkProve (zkCast (Sum Unit (Sum Unit Unit)) (Sum Unit Unit) "
               "(inr (inr unit)) (inr unit))) := refl", NULL);
  ok &= zk_trap("Prod U256 U256", "zkHp (pair bad 2u)");
  ok &= zk_trap("U256", "zkDigest (cons (pair bad 2u) nil)");
  ok &= zk_trap("U256", "zkDigest (cons (flagIf (u256Eq bad 0u) (pair 1u 2u) (pair 1u 2u)) nil)");
  ok &= zk_trap("U256", "zkDigest (cons (pair 1u 2u) (flagIf (u256Eq bad 0u) nil nil))");
  ok &= zk_trap("Flag", "zkVerify (cons (pair 1u 2u) nil) 0u 1u (cons bad (cons 1u (cons 1u (cons 2u nil))))");
  ok &= normal_roundtrip("Prod U256 U256 -> Prod U256 U256", "fun (p : Prod U256 U256) => zkHp p");
  ok &= normal_roundtrip("List (Prod U256 U256) -> U256", "fun (ring : List (Prod U256 U256)) => zkDigest ring");
  ok &= normal_roundtrip("List U256 -> Flag", "fun (sig : List U256) => zkVerify (cons (pair 1u 2u) nil) 0u 1u sig");
  /* The empty domain (sealed-lang S2, R21) lexes, parses and checks. */
  ok &= domain("", NULL);
  /* Elimination consumes its scrutinee before applying a functional result. */
  ok &= guest("def applyUnit : Unit -> Unit -> Unit := fun (u : Unit) (v : Unit) => "
              "elim (fun (w : Unit) => Unit -> Unit) (fun x => x) u v", NULL);
  ok &= guest("def applySum : Sum Unit Unit -> Unit -> Unit := fun (s : Sum Unit Unit) (v : Unit) => "
              "elim (fun (t : Sum Unit Unit) => Unit -> Unit) (fun a x => x) (fun b x => x) s v", NULL);
  ok &= guest("def choose : Sum Unit Unit -> Unit := "
              "elim (fun (s : Sum Unit Unit) => Unit) (fun u => u) (fun u => u)", NULL);
  ok &= normal_roundtrip("Unit -> Unit", "fun (u : Unit) => elim (fun (v : Unit) => Unit) unit u");
  ok &= normal_roundtrip("Sum Unit Unit -> Unit", "fun (s : Sum Unit Unit) => "
                         "elim (fun (t : Sum Unit Unit) => Unit) (fun u => u) (fun u => u) s");
  ok &= normal_roundtrip("(A : Type 0) -> (s : Sum A Unit) -> Path (Sum A Unit) s s", "fun (A : Type 0) (s : Sum A Unit) => "
                         "elim (fun (t : Sum A Unit) => Path (Sum A Unit) t t) (fun a => refl) (fun u => refl) s");
  ok &= guest("def s : Sum Unit Unit := inl unit\n"
              "def left : (v : Unit) -> Path Unit (elim (fun (t : Sum Unit Unit) => Unit -> Unit) "
              "(fun a x => x) (fun b x => unit) s v) v := fun v => refl\n"
              "def t : Sum Unit Unit := inr unit\n"
              "def right : (v : Unit) -> Path Unit (elim (fun (t : Sum Unit Unit) => Unit -> Unit) "
              "(fun a x => x) (fun b x => unit) t v) unit := fun v => refl\n"
              "def one : (v : Unit) -> Path Unit (elim (fun (u : Unit) => Unit -> Unit) "
              "(fun x => x) unit v) v := fun v => refl", NULL);
  ok &= guest("def bad : Unit -> Unit := fun (u : Unit) => elim (fun v => unit) unit u", "TYPE_MISMATCH");
  ok &= guest("def bad : Unit -> Unit := fun (u : Unit) => "
              "elim (fun (t : Sum Unit Unit) => Unit) (fun a => a) (fun b => b) u", "TYPE_MISMATCH");
  ok &= guest("def bad : Unit -> Unit := fun (u : Unit) => "
              "elim (fun (v : Unit) => Unit) unit (inl u)", "TYPE_INFER");
  /* Family eliminators can await their scrutinee. The arity comes from
     the final motive domain, including for named and indexed motives. */
  ok &= domain("family N := zero | succ (pred : N)\n"
               "def id : N -> N := elim (fun (n : N) => N) zero (fun n ih => succ ih)", NULL);
  ok &= domain("family N := zero | succ (pred : N)\n"
               "def P : N -> Type 0 := fun n => N\n"
               "def id : N -> N := elim P zero (fun n ih => succ ih)", NULL);
  ok &= normal_roundtrip_in("family N := zero | succ (pred : N)", "N -> N",
                            "elim (fun n => N) zero (fun n ih => succ ih)");
  ok &= domain("family N := zero | succ (pred : N)\n"
               "family V : N -> Type 0 := nilV : V zero | consV (len : N) (tailV : V len) : V (succ len)\n"
               "def id : (n : N) -> V n -> N := fun n => "
               "elim (fun (k : N) (v : V k) => N) zero (fun k t ih => succ ih)", NULL);
  ok &= domain("family N := zero | succ (pred : N)\n"
               "family V : N -> Type 0 := nilV : V zero | consV (len : N) (tailV : V len) : V (succ len) | altV (altLen : N) : V altLen\n"
               "def id : (n : N) -> V n -> N := fun n => "
               "elim (fun (k : N) (v : V k) => N) zero (fun k t ih => succ ih) (fun k => k)", NULL);
  /* A saturated eliminator with a functional result still consumes the
     scrutinee before the result arguments. */
  ok &= domain("family N := zero | succ (pred : N)\n"
               "def id : N -> N -> N := fun n => elim (fun (k : N) => N -> N) (fun x => x) (fun k ih x => succ (ih x)) n\n"
               "def reduction : Path N (id (succ zero) zero) (succ zero) := refl", NULL);
  /* Bare indexed motives accept and check annotations on later binders. */
  ok &= domain("family N := zero | succ (pred : N)\n"
               "family V : N -> Type 0 := nilV : V zero | consV (len : N) (tailV : V len) : V (succ len)\n"
               "def id : (n : N) -> V n -> N := fun n v => "
               "elim (fun k => fun (t : V k) => N) zero (fun k t ih => succ ih) v", NULL);
  ok &= domain("family N := zero | succ (pred : N)\n"
               "family V : N -> Type 0 := nilV : V zero | consV (len : N) (tailV : V len) : V (succ len)\n"
               "def bad : (n : N) -> V n -> N := fun n v => "
               "elim (fun k => fun (t : N) => N) zero (fun k t ih => succ ih) v", "TYPE_MISMATCH");
  ok &= domain("family F := makeF [ i1 -> missing ]", "DOMAIN_FAMILY");
  ok &= domain("family F := makeF [ ]", "DOMAIN_FAMILY");
  /* A path constructor's fields are not record projections: projection
     would discard its boundary at a neutral interval. */
  ok &= domain("family Only := arc (arcFrom : Only) (arcTo : Only) (i : I) "
               "[ ~i -> arcFrom, i -> arcTo ]\n"
               "def bad : (x : Only) -> (y : Only) -> Path Only (arcFrom x) (arcFrom y) := "
               "fun x y => fun (i : I) => arcFrom (arc x y i)", "NAME_UNKNOWN");
  ok &= domain("family T := pt (value : Nat) | arc (previous : T) (i : I) "
               "[ ~i -> pt (fold (fun n => n) 1 previous), i -> pt (fold (fun n => n) 1 previous) ]", "HIT_BOUNDARY");
  /* Indexed families and blocks (S6). hiit-wrong-index: an end of a path
     constructor is the family at another index. hiit-no-result: a
     constructor of an indexed family states no result. block-order: a
     header names a later sort of its block. index-prop: Prop is not a type
     in an index telescope (rule R4). block-ok: a field names a later sort. */
  ok &= domain("family Sz := zs | ss (szPrev : Sz)\n"
               "family V : Sz -> Type 0 := vnil : V zs | vcons (vSize : Sz) (vTail : V vSize) : V (ss vSize) "
               "| vbad (badSize : Sz) (badTail : V badSize) (i : I) : V (ss badSize) "
               "[ ~i -> vcons badSize badTail, i -> badTail ]", "HIT_INDEX");
  ok &= domain("family Sz := zs | ss (szPrev : Sz)\n"
               "family V : Sz -> Type 0 := vnil | vcons (vSize : Sz) (vTail : V vSize) : V (ss vSize)", "HIT_INDEX");
  ok &= domain("family P0 : Q0 -> Type 0 := p0 : P0 q0 and Q0 := q0", "BLOCK_ORDER");
  /* hiit-wrong-head: the result of a constructor is not its family. */
  ok &= domain("family Sz := zs | ss (szPrev : Sz)\n"
               "family V : Sz -> Type 0 := vnil : Sz | vcons (vSize : Sz) (vTail : V vSize) : V (ss vSize)", "HIT_INDEX");
  ok &= domain("family W : Prop -> Type 0 := w0", "NAME_UNKNOWN");
  ok &= domain("family Ch := gen | adopt (adoptOf : Ch) (adoptBy : Rt adoptOf) "
               "and Rt : Ch -> Type 0 := rat (ratOf : Ch) : Rt ratOf", NULL);
  /* Index arguments are not constructor parameters in point or path cases. */
  ok &= domain("family Sz := zs | ss (pred : Sz)\n"
               "family V (A : Type 0) : Sz -> Type 0 := vn : V A zs "
               "| vc (item : A) : V A (ss zs) "
               "| seg (i : I) : V A zs [ ~i -> vn A, i -> vn A ]\n"
               "def result : Path Nat (fold 0 (fun a => a) refl (vc Nat 7)) 7 := refl", NULL);
  /* A transport's printed result must still inhabit the destination fibre. */
  ok &= normal_roundtrip_in("family Sz := zs | ss (pred : Sz)\n"
                            "family V (A : Type 0) : Sz -> Type 0 := vn : V A zs",
                            "V Nat zs", "transp (fun (i : I) => V Nat zs) i0 (vn Nat)");
  ok &= normal_roundtrip_in("family Sz := zs | ss (pred : Sz) "
                            "| edge (i : I) [ ~i -> zs, i -> ss zs ]\n"
                            "family V : Sz -> Type 0 := vn : V zs | vc : V (ss zs)",
                            "V (ss zs)", "transp (fun (i : I) => V (edge i)) i0 vn");
  ok &= normal_roundtrip_in("family Even (A : Type 0) := ez | es (oddPrev : Odd A) "
                            "and Odd (A : Type 0) := os (evenPrev : Even A)",
                            "Even Nat", "transp (fun (i : I) => Even Nat) i0 (es Nat (os Nat (ez Nat)))");
  ok &= domain("family Even := ez | es (oddPrev : Odd) and Odd := os (evenPrev : Even)\n"
               "def result : Nat := fold 0 (fun n => natAdd n 1) (es (os ez))", "TYPE_MISMATCH");
  /* Block folds print their targets before their constructor cases. */
  ok &= normal_roundtrip_in("family Even := ez | es (oddPrev : Odd) and Odd := os (evenPrev : Even)",
                            "Even -> Nat", "fun x => fold Nat Unit 0 (fun u => 1) (fun n => unit) x");
  ok &= normal_roundtrip_in("family Even := ez | es (oddPrev : Odd) and Odd := os (evenPrev : Even)",
                            "Odd -> Unit", "fun x => fold Nat Unit 0 (fun u => 1) (fun n => unit) x");
  ok &= domain("family Even := ez | es (oddPrev : Odd) and Odd := os (evenPrev : Even)\n"
               "def point : Path Nat (fold Nat Unit 0 (fun u => 1) (fun n => unit) (es (os ez))) 1 := refl\n"
               "def other : Path Unit (fold Nat Unit 0 (fun u => 1) (fun n => unit) (os ez)) unit := refl", NULL);
  /* A recursive field of a sibling sort is typed at the target of that
     sort, and it folds at that sort. */
  ok &= domain("family Even := ez | es (oddPrev : Odd) and Odd := os (evenPrev : Even)\n"
               "def one : Path Nat (fold Nat Unit 0 (fun (u : Unit) => 1) (fun (n : Nat) => unit) (es (os ez))) 1 := refl", NULL);
  ok &= domain("family Even := ez | es (oddPrev : Odd) and Odd := os (evenPrev : Even)\n"
               "def two : Path Nat (fold Nat Nat 0 (fun (u : Nat) => natAdd u 1) (fun (n : Nat) => natAdd n 1) (es (os ez))) 2 := refl", NULL);
  /* block-fold-extra: a block fold with an extra argument before the
     value. Without the arity test the extra argument is never read. */
  ok &= domain("family Even := ez | es (oddPrev : Odd) and Odd := os (evenPrev : Even)\n"
               "def bad : Nat := fold (Even -> Nat) Unit (fun (e : Even) => 0) (fun (u : Unit) => fun (e : Even) => 1) "
               "(fun (g : Even -> Nat) => unit) unit ez", "TYPE_MISMATCH");
  /* A path field over a sibling sort folds at that sibling. */
  ok &= domain("family Box := box (boxLeft : Item) (boxRight : Item) (boxPath : Path Item boxLeft boxRight) "
               "and Item := item\n"
               "def result : Path Nat 7 7 := fun (i : I) => "
               "fold Nat Unit (fun x y p => elim (fun (u : Unit) => Nat) 7 (p @ i)) unit (box item item refl)", NULL);
  /* Every sort's cases must use the scrutinee's parameter telescope. */
  ok &= domain("family F (A : Type 0) := makeF (fValue : A) and G (n : Nat) := makeG\n"
               "def result : Unit := fold Unit Unit (fun x => unit) unit (makeG 7)", "TYPE_MISMATCH");
  ok &= domain("family F (A : Type 0) := makeF (fValue : A) and G := makeG\n"
               "def result : Unit := fold Unit Unit (fun x => unit) unit makeG", "TYPE_MISMATCH");
  ok &= domain("family L (A : Type 0) (a : A) := leftL (lValue : A) "
               "and R (A : Type 0) (a : A) := rightR (rLeft : L A a)\n"
               "def firstL : Path Nat (fold Nat Unit (fun n => n) (fun n => unit) (leftL Nat 7 8)) 8 := refl\n"
               "def secondR : Path Unit (fold Nat Unit (fun n => n) (fun n => unit) (rightR Nat 7 (leftL Nat 7 8))) unit := refl", NULL);
  /* Header folds must not see the zeroed, reserved constructor signatures. */
  ok &= domain("family A := a (n : Nat) and B (x : A) (y : fold Unit x) := b\n"
               "def value : B (a 0) unit := b (a 0) unit", "HIT_BOUNDARY");
  /* A later sort's universe propagates backwards through the block. */
  ok &= domain("family Low := low (high : High) and High := highCtor (big : Type 0)\n"
               "def small : Type 0 := Low", "TYPE_MISMATCH");
  ok &= domain("family Low := low (high : High) and High := highCtor (big : Type 0)\n"
               "def large : Type 1 := Low", NULL);
  ok &= domain("family F (A : Type 0) : ((G : Type 0) -> G -> G) -> Type 0 := "
               "mk (x : (T : Type 0) -> T -> T) : F A x and G := g", NULL);
  ok &= domain("family F (G : Type 0) : G -> Type 0 := mk (x : G) : F G x and G := g", NULL);
  ok &= domain("state State : Nat -> Type 0 := st (counter : Nat) : State counter", "REFUSE_STATE");
  ok &= domain("family U := u\n"
               "family T := pt (value : Nat) | arc (i : I) "
               "[ ~i -> pt (fold 0 u), i -> pt (fold 0 u) ]", NULL);
  ok &= domain("family T (A : Type 0) := leafT (v : A) | nodeT (l : T Nat) (r : T A)", "DOMAIN_FAMILY");
  ok &= domain("family F := makeF (field : Nat) (field : Flag)", "REFUSE_NAME");
  ok &= domain("family F := makeF (makeF : Nat)", "REFUSE_NAME");
  ok &= domain("family F := makeF (firstField : Nat) (secondField : Flag)", NULL);
  ok &= domain("family F := a (field : Nat) | b (field : Nat)", "REFUSE_NAME");
  ok &= domain("def s : Sum Nat Nat := inl 7\ndef bad : Sum Flag Nat := bind s (fun x => inr x)", "TYPE_MISMATCH");
  ok &= domain("family Lot (n : Nat) := makeLot (amount : Nat)\n"
               "def s : Sum Nat Nat := inl 0\n"
               "def f : (n : Nat) -> Nat -> Lot n := fun n x => makeLot n x\n"
               "def bad : (n : Nat) -> Lot n := first (pair (either f f s) 0)", "TYPE_MISMATCH");
  ok &= domain("def bad : Type 1 := Nat", "TYPE_MISMATCH");
  ok &= domain("def bad : Type 2 := Type 1", "TYPE_UNIVERSE");
  ok &= domain("family U := makeU (decode : Type 0)\ndef bad : Type 0 := U", "TYPE_MISMATCH");
  ok &= domain("family U := makeU (decode : Type 0)\nfamily W := makeW (wrapped : U)\ndef bad : Type 0 := W", "TYPE_MISMATCH");
  ok &= domain("family Box (A : Type 1) := box (unbox : A)\ndef bad : Type 0 := Box (Type 0)", "TYPE_MISMATCH");
  ok &= domain("family U := makeU (decode : Type 0)\ndef good : Type 1 := U", NULL);
  ok &= domain("def bad : Type 0 := Path (Type 0) Nat Nat", "TYPE_MISMATCH");
  ok &= domain("def good : Type 1 := Path (Type 0) Nat Nat", NULL);
  ok &= domain("def ConstNat : Nat -> Type 0 := fun n => Nat\n"
               "state State := makeState (count : Nat) (other : ConstNat count)\n"
               "def init : State := makeState 3 7\n"
               "def value : Path Nat (other init) 7 := refl", NULL);
  ok &= domain("state State := makeState (count : Nat) (proof : Path Nat count count)", "REFUSE_STATE");
  ok &= domain("family U := makeU (decode : Type 0)\n"
               "def bad : Type 0 := Path U (makeU Nat) (makeU Nat)", "TYPE_MISMATCH");
  ok &= domain("family U := makeU (decode : Type 0)\n"
               "def good : Type 1 := Path U (makeU Nat) (makeU Nat)", NULL);
  ok &= domain("family U := makeU (decode : Type 0)\n"
               "family W := makeW (proof : Path U (makeU Nat) (makeU Nat))\n"
               "def bad : Type 0 := W", "TYPE_MISMATCH");
  ok &= domain("family U := makeU (decode : Type 0)\n"
               "family W := makeW (proof : Path U (makeU Nat) (makeU Nat))\n"
               "def good : Type 1 := W", NULL);
  ok &= domain("def eta : (p : Prod Nat Nat) -> Path (Prod Nat Nat) (pair (first p) (second p)) p := fun p => refl", NULL);
  ok &= domain("def eta : (p : Sigma (n : Nat) (Path Nat n n)) -> Path (Sigma (n : Nat) (Path Nat n n)) (pack (witness p) (payload p)) p := fun p => refl", NULL);
  ok &= normal_roundtrip("Nat -> (A : Type 0) -> A -> A",
    "fun (i0 : Nat) (A : Type 0) (a : A) => transp (fun (i : I) => A) (~i1) a");
  /* Guest mode (orbit-lang S3): a program with no kit name checks, and a Nat
     literal is a kit item. The second row passes in kit mode only if the mode
     is wrong. */
  ok &= guest("def keep : Sum Unit Unit -> Sum Unit Unit := fun c => c", NULL);
  ok &= guest("def three : Nat := 3", "REFUSE_KIT");
  if (ok) puts("domain regressions: passed");
  return ok ? 0 : 1;
}
