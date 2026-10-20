/* The contract writer (orbit-lang S9 B1, docs/PLAN.md section 9): the kit
   program text of the DAO in a checked guest program. The entry amend of
   the design is named enact, as amend is a name of the design signature. */
#include <errno.h>
#include <string.h>

#include "front/blsag.h"
#include "front/bn254.h"
#include "front/front.h"
#include "front/writer.h"

/* The most table rows: histograms times outcomes. */
#define TABLE_MAX 4096u

typedef struct {
  uint32_t n;       /* the members */
  uint32_t k;       /* the choices */
  uint32_t m;       /* the outcomes */
  uint32_t rows;    /* the histograms */
  uint32_t *hist;   /* rows times k counts, in hist_fill order */
  uint32_t *table;  /* m times rows outcome indices of witness (menu o) */
  uint32_t *entry;  /* the entry id of menu o, one per outcome */
  uint32_t *rep;    /* the first outcome of each entry id */
  uint32_t entries;
  uint32_t start;
  U256 *ring;       /* 2n words from --ring, or NULL */
  char prefix[32];  /* a fresh prefix for the guest table definitions */
} Dao;

static uint32_t def_index(const Machine *m, const char *name) {
  uint32_t i = 0;
  while (i < m->def_count && strcmp(m->defs[i].name, name) != 0)
    i++;
  return i;
}

static const Value *slot(Machine *m, const char *name) {
  uint32_t d = def_index(m, name);
  return d < m->def_count ? def_value(m, d) : NULL;
}

static void table_prefix(const Machine *m, Dao *d) {
  uint32_t attempt = 0;
  uint32_t i;
  strcpy(d->prefix, "contract");
  for (;;) {
    size_t len = strlen(d->prefix);
    for (i = 0; i < m->def_count; i++) {
      if (strncmp(m->defs[i].name, d->prefix, len) == 0)
        break;
    }
    if (i == m->def_count)
      return;
    snprintf(d->prefix, sizeof d->prefix, "contract%u", attempt++);
  }
}

/* The constructor count of the closed carrier Sum Unit (Sum Unit ... Unit),
   or 0 for any other shape. */
static uint32_t chain_count(const Value *t) {
  uint32_t n = 1;
  while (t != NULL && val_is(t, OP_SUM) && val_is(t->args[0], OP_UNIT) && n < UINT32_MAX) {
    t = t->args[1];
    n++;
  }
  return t != NULL && val_is(t, OP_UNIT) ? n : 0;
}

/* The histograms of LEFT members over the choices from C on, with a_c from
   LEFT down to 0, into D->hist from row *ROW. */
static void hist_fill(Dao *d, uint32_t *prefix, uint32_t c, uint32_t left, uint32_t *row) {
  uint32_t v;
  if (c + 1 == d->k) {
    prefix[c] = left;
    memcpy(&d->hist[*row * d->k], prefix, d->k * sizeof *prefix);
    (*row)++;
    return;
  }
  for (v = left + 1; v-- > 0;) {
    prefix[c] = v;
    hist_fill(d, prefix, c + 1, left - v, row);
  }
}

/* Constructor I of a carrier of COUNT constructors. */
static void ctor_text(FILE *f, uint32_t i, uint32_t count) {
  if (count == 1) {
    fputs("unit", f);
    return;
  }
  if (i == 0) {
    fputs("inl unit", f);
    return;
  }
  fputs("inr (", f);
  ctor_text(f, i - 1, count - 1);
  fputc(')', f);
}

/* The canonical config of histogram ROW: the first a_0 members choose
   choice 0, the next a_1 choice 1, and so on. Member j is read by either on
   the variable xj of type contractMj. */
static void config_text(FILE *f, const Dao *d, uint32_t row) {
  const uint32_t *a = &d->hist[row * d->k];
  uint32_t seen = 0;
  uint32_t c = 0;
  uint32_t j;
  fprintf(f, "def %sCfg%u : Member -> Choice := fun (x0 : Member) =>", d->prefix, row);
  for (j = 0; j < d->n; j++) {
    while (seen + a[c] <= j)
      seen += a[c++];
    fputs(j + 1 == d->n ? " " : " either (fun (u : Unit) => ", f);
    ctor_text(f, c, d->k);
    if (j + 1 < d->n)
      fprintf(f, ") (fun (x%u : %sM%u) =>", j + 1, d->prefix, j + 1);
  }
  for (j = 0; j + 1 < d->n; j++)
    fprintf(f, ") x%u", d->n - 2 - j);
  fputc('\n', f);
}

/* The table definitions: contractTbl O At H is witness (menu o) at the
   config of histogram H. */
static void tables_text(FILE *f, const Dao *d) {
  uint32_t j;
  uint32_t o;
  uint32_t h;
  for (j = d->n; j-- > 1;) {
    if (j + 1 == d->n)
      fprintf(f, "def %sM%u : Type 0 := Unit\n", d->prefix, j);
    else
      fprintf(f, "def %sM%u : Type 0 := Sum Unit %sM%u\n", d->prefix, j, d->prefix, j + 1);
  }
  for (h = 0; h < d->rows; h++)
    config_text(f, d, h);
  for (o = 0; o < d->m; o++) {
    for (h = 0; h < d->rows; h++) {
      fprintf(f, "def %sTbl%uAt%u : Outcome := witness (menu (", d->prefix, o, h);
      ctor_text(f, o, d->m);
      fprintf(f, ")) %sCfg%u\n", d->prefix, h);
    }
  }
}

/* The text of TEXT, a newline and the bytes written to F, in the arena. */
static char *joined_text(Arena *arena, const char *text, size_t len, FILE *f, size_t *out_len) {
  long size = ftell(f);
  char *buf = size < 0 ? NULL : arena_alloc(arena, len + (size_t)size + 2);
  if (buf == NULL)
    return NULL;
  memcpy(buf, text, len);
  buf[len] = '\n';
  rewind(f);
  if (fread(buf + len + 1, 1, (size_t)size, f) != (size_t)size)
    return NULL;
  buf[len + 1 + (size_t)size] = '\0';
  *out_len = len + 1 + (size_t)size;
  return buf;
}

/* Checks TEXT with the table definitions again and reads each table entry
   as a constructor index of Outcome. A value that is not a constructor is
   a stuck table entry. */
static int read_tables(Arena *arena, Machine *m, const char *path, const char *text, size_t len, Dao *d) {
  FILE *f = tmpfile();
  DeclList decls;
  Machine second;
  const Value *outcome;
  char *all;
  size_t all_len = 0;
  char name[64];
  uint32_t o;
  uint32_t h;
  if (f == NULL)
    return diag_fail(m->diag, "IO_WRITE", NULL, "tmpfile: %s", strerror(errno)) + 2;
  tables_text(f, d);
  all = ferror(f) ? NULL : joined_text(arena, text, len, f, &all_len);
  fclose(f);
  if (all == NULL)
    return diag_fail(m->diag, "IO_WRITE", NULL, "cannot write the table definitions") + 2;
  if (!front_load(arena, path, all, all_len, &decls, m->diag) || !check_program(arena, &decls, &second, m->diag, CHECK_GUEST))
    return 1;
  outcome = slot(&second, "Outcome");
  for (o = 0; o < d->m; o++) {
    for (h = 0; h < d->rows; h++) {
      uint64_t index = 0;
      const Value *v;
      snprintf(name, sizeof name, "%sTbl%uAt%u", d->prefix, o, h);
      v = slot(&second, name);
      if (v == NULL)
        return 1;
      if (outcome == NULL || !zk_index(&second, outcome, v, &index) || index >= d->m)
        return diag_fail(m->diag, "REFUSE_CONTRACT", "menu", "the table entry of outcome %u at histogram %u is stuck: the rule gives no constructor of Outcome", o, h) + 1;
      d->table[o * d->rows + h] = (uint32_t)index;
    }
  }
  return 0;
}

/* One entry id per equal table (ruling Q-S9-4 (iii)). */
static void dedup(Dao *d) {
  uint32_t o;
  uint32_t e;
  d->entries = 0;
  for (o = 0; o < d->m; o++) {
    for (e = 0; e < d->entries && memcmp(&d->table[d->rep[e] * d->rows], &d->table[o * d->rows], d->rows * sizeof *d->table) != 0; e++)
      ;
    d->entry[o] = e;
    if (e == d->entries)
      d->rep[d->entries++] = o;
  }
}

static int digit_of(char c, uint32_t base) {
  int v = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : 99;
  return (uint32_t)v < base ? v : -1;
}

/* A build word: decimal digits, or 0x and hex digits, with an optional u. */
static int parse_number(const char *s, U256 *out) {
  size_t len = strlen(s);
  uint32_t base = 10;
  u256_from_u64(out, 0u);
  if (len > 0 && s[len - 1] == 'u')
    len--;
  if (len > 2 && s[0] == '0' && s[1] == 'x') {
    base = 16;
    s += 2;
    len -= 2;
  }
  if (len == 0)
    return 0;
  for (; len > 0; s++, len--) {
    int digit = digit_of(*s, base);
    if (digit < 0 || !u256_scale_add(out, base, (uint32_t)digit))
      return 0;
  }
  return 1;
}

/* The --ring and --start arguments (ruling Q-S9-3). */
static int build_args(Arena *arena, Machine *m, const ContractOptions *opt, Dao *d) {
  U256 limit;
  U256 start;
  int i;
  d->ring = NULL;
  d->start = 0;
  u256_from_u64(&limit, d->m);
  if (opt->start != NULL && (!parse_number(opt->start, &start) || u256_cmp(&start, &limit) >= 0))
    return diag_fail(m->diag, "USAGE", NULL, "--start takes an outcome index below %u, found '%s'", d->m, opt->start) + 2;
  d->start = opt->start != NULL ? (uint32_t)start.limb[0] : 0;
  if (opt->ring == NULL)
    return 0;
  if (opt->ring_words != 2 * (int)d->n)
    return diag_fail(m->diag, "USAGE", NULL, "--ring takes %u words (X Y of each member), found %d", 2 * d->n, opt->ring_words) + 2;
  d->ring = arena_alloc(arena, 2 * d->n * sizeof *d->ring);
  if (d->ring == NULL)
    return diag_fail(m->diag, "OOM", NULL, "no memory for the ring") + 1;
  for (i = 0; i < opt->ring_words; i++) {
    if (!parse_number(opt->ring[i], &d->ring[i]))
      return diag_fail(m->diag, "USAGE", NULL, "--ring word '%s' is not a number", opt->ring[i]) + 2;
  }
  for (i = 0; i < (int)d->n; i++) {
    G1 point;
    U256 zero;
    int j;
    u256_from_u64(&zero, 0u);
    if (!g1_from_words(&point, &d->ring[2 * i], &d->ring[2 * i + 1]) || !g1_on_curve(&point))
      return diag_fail(m->diag, "REFUSE_CONTRACT", NULL, "ring point %d is not a point of bn254", i + 1) + 1;
    if (u256_cmp(&d->ring[2 * i], &zero) == 0 && u256_cmp(&d->ring[2 * i + 1], &zero) == 0)
      return diag_fail(m->diag, "REFUSE_CONTRACT", NULL, "ring point %d is infinity, not a member key", i + 1) + 1;
    for (j = 0; j < i; j++) {
      if (u256_cmp(&d->ring[2 * i], &d->ring[2 * j]) == 0 && u256_cmp(&d->ring[2 * i + 1], &d->ring[2 * j + 1]) == 0)
        return diag_fail(m->diag, "REFUSE_CONTRACT", NULL, "ring point %d duplicates member %d", i + 1, j + 1) + 1;
    }
  }
  return 0;
}

/* The lookup of entry E from choice C on: a flagIf on the votes of C for
   each count that fits in LEFT; the last choice takes the rest. NEXT writes
   the entry id of the outcome, else the outcome. *ROW counts the
   histograms in hist_fill order. */
static void trie_text(FILE *f, const Dao *d, uint32_t e, uint32_t c, uint32_t left, uint32_t *row, int next, const char *none) {
  uint32_t v;
  uint32_t t;
  if (c + 1 == d->k) {
    t = d->table[d->rep[e] * d->rows + *row];
    fprintf(f, "%uu", next ? d->entry[t] : t);
    (*row)++;
    return;
  }
  for (v = left + 1; v-- > 0;) {
    fprintf(f, "(flagIf (u256Eq (votes s %uu) %uu) ", c, v);
    trie_text(f, d, e, c + 1, left - v, row, next, none);
    fputc(' ', f);
  }
  fputs(none, f);
  for (v = 0; v <= left; v++)
    fputc(')', f);
}

static void lookup_text(FILE *f, const Dao *d, int next, const char *none) {
  uint32_t e;
  for (e = 0; e < d->entries; e++) {
    uint32_t row = 0;
    fprintf(f, "(flagIf (u256Eq (active s) %uu) ", e);
    trie_text(f, d, e, 0, d->n, &row, next, none);
    fputc(' ', f);
  }
  fputs(none, f);
  for (e = 0; e < d->entries; e++)
    fputc(')', f);
}

/* A list of the names PREFIX0 to PREFIX(N-1) as nested cons. */
static void ring_text(FILE *f, uint32_t n) {
  uint32_t j;
  for (j = 0; j < n; j++)
    fprintf(f, "%scons p%u ", j > 0 ? "(" : "", j);
  fputs("nil", f);
  for (j = 1; j < n; j++)
    fputc(')', f);
}

static void program_text(FILE *f, const Dao *d, const char *path) {
  char x[96];
  char y[96];
  char none[32];
  uint32_t j;
  const unsigned char *p;
  fputs("-- The DAO contract of ", f);
  for (p = (const unsigned char *)path; *p != '\0'; p++)
    fputc(*p < 32 || *p == 127 ? '?' : *p, f);
  fprintf(f, " (langc contract, S9 B1): %u members, %u choices, %u outcomes, %u constitutions.\n", d->n, d->k, d->m, d->entries);
  fputs("-- The ring is closed, the nullifier and the tally keys hold the poll id, enact is the amend of the design.\n", f);
  if (d->ring == NULL)
    fputs("def g : Prod U256 U256 := pair 1u 2u\n", f);
  for (j = 0; j < d->n; j++) {
    if (d->ring == NULL) {
      fprintf(f, "def k%u : U256 := zkKey %u\n", j, j);
      fprintf(f, "def p%u : Prod U256 U256 := ecMul g k%u\n", j, j);
    } else {
      word_text(WORD_U256, &d->ring[2 * j], x, sizeof x);
      word_text(WORD_U256, &d->ring[2 * j + 1], y, sizeof y);
      fprintf(f, "def p%u : Prod U256 U256 := pair %s %s\n", j, x, y);
    }
  }
  fputs("def ring : List (Prod U256 U256) := ", f);
  ring_text(f, d->n);
  fputs("\n\nstate State := makeState (used : Map U256 Flag) (tally : Map U256 U256)\n", f);
  fputs("  (pollId : U256) (active : U256) (prior : U256) (casts : U256)\n", f);
  fprintf(f, "def init : State := makeState mapEmpty mapEmpty 0u %uu %uu 0u\n\n", d->entries == 0 ? 0 : d->entry[d->start], d->entries);
  fputs("def votes : State -> U256 -> U256 := fun (s : State) (c : U256) => mapGet (tally s) (hash (pollId s) c)\n", f);
  snprintf(none, sizeof none, "%uu", d->m);
  fprintf(f, "def outcomeOf : State -> U256 := fun (s : State) => flagIf (u256Le %uu (casts s))\n  ", d->n);
  lookup_text(f, d, 0, none);
  fprintf(f, "\n  %s\n", none);
  fputs("def nextOf : State -> U256 := fun (s : State) =>\n  ", f);
  lookup_text(f, d, 1, "(active s)");
  fputs("\n\n-- Entry: a choice out of range, a used key image or a failed verify reverts, else the cast is counted.\n", f);
  fputs("def cast : (env : Env) -> (s : State) -> (choice c1", f);
  for (j = 1; j <= d->n; j++)
    fprintf(f, " s%u", j);
  fputs(" ix iy : U256) -> Option (Prod State (List Out)) :=\n  fun (env : Env) (s : State) (choice c1", f);
  for (j = 1; j <= d->n; j++)
    fprintf(f, " s%u", j);
  fprintf(f, " ix iy : U256) =>\n    flagIf (u256Le %uu choice) none\n", d->k);
  fputs("      (flagIf (mapGet (used s) (hash (hash ix iy) (pollId s))) none\n", f);
  fputs("        (flagIf (zkVerify ring (pollId s) choice (cons c1 ", f);
  for (j = 1; j <= d->n; j++)
    fprintf(f, "(cons s%u ", j);
  fputs("(cons ix (cons iy nil", f);
  for (j = 0; j < d->n + 3; j++)
    fputc(')', f);
  fputs(")\n          (some (pair (makeState (mapSet (used s) (hash (hash ix iy) (pollId s)) flagYes)\n", f);
  fputs("            (mapSet (tally s) (hash (pollId s) choice) (u256Add (votes s choice) 1u))\n", f);
  fputs("            (pollId s) (active s) (prior s) (u256Add (casts s) 1u)) nil))\n          none))\n\n", f);
  fputs("-- Entry: before every member casts it reverts, else the next constitution is the menu of the outcome.\n", f);
  fputs("def enact : (env : Env) -> (s : State) -> Option (Prod State (List Out)) :=\n", f);
  fprintf(f, "  fun (env : Env) (s : State) => flagIf (u256Le %uu (casts s))\n", d->n);
  fputs("    (some (pair (makeState (used s) (tally s) (u256Add (pollId s) 1u) (nextOf s) (active s) 0u) nil))\n    none\n\n", f);
  fputs("-- Views.\n", f);
  fputs("def count : (env : Env) -> (s : State) -> (choice : U256) -> U256 := fun (env : Env) (s : State) (choice : U256) => votes s choice\n", f);
  fputs("def outcome : (env : Env) -> (s : State) -> U256 := fun (env : Env) (s : State) => outcomeOf s\n", f);
  fputs("def selfConstituting : (env : Env) -> (s : State) -> Flag := fun (env : Env) (s : State) => u256Eq (active s) (prior s)\n", f);
  fputs("def poll : (env : Env) -> (s : State) -> U256 := fun (env : Env) (s : State) => pollId s\n", f);
  fputs("def constitution : (env : Env) -> (s : State) -> U256 := fun (env : Env) (s : State) => active s\n", f);
}

/* Copies CODE to OUT. Returns 0, or 2 after an IO_WRITE diagnostic. */
static int copy_out(Machine *m, FILE *code, FILE *out) {
  unsigned char buffer[1024];
  size_t size;
  int failed = ferror(code);
  rewind(code);
  while (!failed && (size = fread(buffer, 1, sizeof buffer, code)) > 0)
    failed = fwrite(buffer, 1, size, out) != size;
  failed = failed || ferror(code) || fflush(out) != 0 || ferror(out);
  fclose(code);
  return failed ? diag_fail(m->diag, "IO_WRITE", NULL, "stdout: %s", strerror(errno)) + 2 : 0;
}

int contract_command(Arena *arena, Machine *m, const char *path, const char *text, size_t len, const ContractOptions *opt, FILE *out) {
  static const char *const slots[] = {"Member", "Choice", "Outcome", "outcomeSet", "menu"};
  uint32_t prefix[64];
  uint64_t rows = 1;
  uint32_t row = 0;
  uint32_t i;
  int status;
  Dao d;
  FILE *code;
  for (i = 0; i < sizeof slots / sizeof slots[0]; i++) {
    if (def_index(m, slots[i]) == m->def_count)
      return diag_fail(m->diag, "REFUSE_CONTRACT", slots[i], "the program has no DAO prelude: %s is not defined", slots[i]) + 1;
  }
  d.n = chain_count(slot(m, "Member"));
  d.k = chain_count(slot(m, "Choice"));
  d.m = chain_count(slot(m, "Outcome"));
  if (d.n == 0 || d.k == 0 || d.m == 0)
    return diag_fail(m->diag, "REFUSE_CONTRACT", d.n == 0 ? "Member" : d.k == 0 ? "Choice" : "Outcome", "the carrier is not a closed finite Sum Unit (Sum Unit ... Unit)") + 1;
  if (d.n > BLSAG_RING_MAX)
    return diag_fail(m->diag, "EVM_RING", "Member", "the ring has %u members, the EVM cast takes at most %u (O28)", d.n, BLSAG_RING_MAX) + 1;
  for (i = 1; i < d.k && rows <= TABLE_MAX; i++)
    rows = rows * (d.n + i) / i;
  if (d.k > sizeof prefix / sizeof prefix[0] || rows * d.m > TABLE_MAX)
    return diag_fail(m->diag, "REFUSE_CONTRACT", "Choice", "the table has more than %u rows", TABLE_MAX) + 1;
  status = build_args(arena, m, opt, &d);
  if (status != 0)
    return status;
  d.rows = (uint32_t)rows;
  d.hist = arena_alloc(arena, d.rows * d.k * sizeof *d.hist);
  d.table = arena_alloc(arena, d.rows * d.m * sizeof *d.table);
  d.entry = arena_alloc(arena, d.m * sizeof *d.entry);
  d.rep = arena_alloc(arena, d.m * sizeof *d.rep);
  if (d.hist == NULL || d.table == NULL || d.entry == NULL || d.rep == NULL)
    return diag_fail(m->diag, "OOM", NULL, "no memory for the tables") + 1;
  hist_fill(&d, prefix, 0, d.n, &row);
  table_prefix(m, &d);
  status = read_tables(arena, m, path, text, len, &d);
  if (status != 0)
    return status;
  dedup(&d);
  code = tmpfile();
  if (code == NULL)
    return diag_fail(m->diag, "IO_WRITE", NULL, "tmpfile: %s", strerror(errno)) + 2;
  program_text(code, &d, path);
  return copy_out(m, code, out);
}
