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

int main(void) {
  int ok = 1;
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
