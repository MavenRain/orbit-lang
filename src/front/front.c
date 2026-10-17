#include <string.h>

#include "front/contract.h"
#include "front/front.h"

static int load_one(Arena *arena, const char *source, Origin origin, const char *text, size_t len, DeclList *out, Diag *diag) {
  TokenList toks;
  if (!lex_source(arena, source, text, len, &toks, diag)) return 0;
  return parse_source(arena, source, origin, &toks, out, diag);
}

/* Orbit as a kernel declaration (orbit-lang S4, PLAN section 8): the set
   quotient of the voter assignments by act. R is inline, so the kernel adds
   no other name. */
#define ORBIT_SOURCE_NAME "kernel/orbit.lang"
#define ORBIT_R "(fun (c : Member -> Choice) (d : Member -> Choice) => Sigma (g : Group) (Path (Member -> Choice) (act g d) c))"

static const char orbit_source[] =
  "def Orbit : Type 0 := Quot (Member -> Choice) " ORBIT_R "\n"
  "def orbitProj : (Member -> Choice) -> Orbit := fun (c : Member -> Choice) => quot (Member -> Choice) " ORBIT_R " c\n";

/* When the program from FIRST on defines act, put Orbit and orbitProj
   directly after it, with the trusted origin ORIGIN_DOMAIN. Check order is
   declaration order, so a refused act stops before them. */
static int orbit_after_act(Arena *arena, size_t first, DeclList *out, Diag *diag) {
  DeclList orbit = {NULL, 0, 0};
  Decl *items;
  size_t at;
  for (at = first; at < out->count && (out->items[at].kind != DECL_DEF || strcmp(out->items[at].name, "act") != 0); at++)
    ;
  if (at == out->count)
    return 1;
  if (!load_one(arena, ORBIT_SOURCE_NAME, ORIGIN_DOMAIN, orbit_source, sizeof orbit_source - 1u, &orbit, diag))
    return 0;
  items = arena_alloc(arena, (out->count + orbit.count) * sizeof *items);
  if (items == NULL)
    return 0;
  memcpy(items, out->items, (at + 1u) * sizeof *items);
  memcpy(items + at + 1u, orbit.items, orbit.count * sizeof *items);
  memcpy(items + at + 1u + orbit.count, out->items + at + 1u, (out->count - at - 1u) * sizeof *items);
  out->items = items;
  out->count += orbit.count;
  out->cap = out->count;
  return 1;
}

int front_load(Arena *arena, const char *source, const char *text, size_t len, DeclList *out, Diag *diag) {
  size_t first;
  out->items = NULL;
  out->count = 0;
  out->cap = 0;
  if (!load_one(arena, "domain/domain.lang", ORIGIN_DOMAIN, (const char *)domain_source, domain_source_len, out, diag)
      || !load_one(arena, CONTRACT_SOURCE_NAME, ORIGIN_KIT, contract_source, contract_source_len, out, diag))
    return 0;
  first = out->count;
  return load_one(arena, source, ORIGIN_PROGRAM, text, len, out, diag)
      && orbit_after_act(arena, first, out, diag);
}

int front_load_domain(Arena *arena, const char *source, const char *domain, const char *text, size_t len, DeclList *out, Diag *diag) {
  size_t first;
  out->items = NULL;
  out->count = 0;
  out->cap = 0;
  if (!load_one(arena, "domain/domain.lang", ORIGIN_DOMAIN, (const char *)domain_source, domain_source_len, out, diag)
      || !load_one(arena, CONTRACT_SOURCE_NAME, ORIGIN_KIT, contract_source, contract_source_len, out, diag)
      || !load_one(arena, source, ORIGIN_DOMAIN, domain, strlen(domain), out, diag))
    return 0;
  first = out->count;
  return load_one(arena, source, ORIGIN_PROGRAM, text, len, out, diag)
      && orbit_after_act(arena, first, out, diag);
}
