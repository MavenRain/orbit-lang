/* The writer must produce checked kit code or fail before emitting it. */
#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "front/front.h"
#include "front/writer.h"

static int writer_case(const char *label, const char *suffix, const char *path,
                       char *const *ring, int ring_words, int broken,
                       int want, const char *code) {
  Arena arena;
  Diag diag;
  DeclList decls;
  Machine machine;
  ContractOptions opt = {ring, ring_words, NULL};
  char *source = NULL;
  size_t len = 0;
  FILE *out = broken ? fopen("/dev/null", "r") : tmpfile();
  int ok;
  FILE *input;
  if (out == NULL) return 0;
  arena_init(&arena, (size_t)1 << 30);
  diag_init(&diag);
  source = arena_alloc(&arena, SOURCE_MAX_BYTES + 1);
  input = fopen("examples/dao.lang", "rb");
  ok = source != NULL && input != NULL;
  if (ok) {
    len = fread(source, 1, SOURCE_MAX_BYTES, input);
    source[len] = '\0';
    ok = !ferror(input) && feof(input);
  }
  if (input != NULL) fclose(input);
  if (ok && suffix != NULL) {
    size_t extra = strlen(suffix);
    char *all = arena_alloc(&arena, len + extra + 1);
    ok = all != NULL;
    if (ok) {
      memcpy(all, source, len);
      memcpy(all + len, suffix, extra + 1);
      source = all;
      len += extra;
    }
  }
  ok = ok && front_load(&arena, path, source, len, &decls, &diag)
    && check_program(&arena, &decls, &machine, &diag, CHECK_GUEST);
  if (ok) {
    int status = contract_command(&arena, &machine, path, source, len, &opt, out);
    ok = status == want && (code == NULL ? !diag.set
      : diag.set && strcmp(diag.code, code) == 0);
    if (!broken && status == 0 && want == 0) {
      long size = ftell(out);
      char *text = size < 0 ? NULL : arena_alloc(&arena, (size_t)size + 1);
      Machine kit;
      rewind(out);
      ok = ok && text != NULL && fread(text, 1, (size_t)size, out) == (size_t)size;
      if (ok) {
        text[size] = '\0';
        ok = front_load(&arena, "written.lang", text, (size_t)size, &decls, &diag)
          && check_program(&arena, &decls, &kit, &diag, CHECK_KIT);
      }
    } else if (!broken && want != 0) {
      ok = ok && ftell(out) == 0;
    }
  }
  if (!ok) {
    fprintf(stderr, "FAIL writer %s\n", label);
    diag_print(&diag, stderr);
  }
  fclose(out);
  arena_release(&arena);
  return ok;
}

static int closed_pipe(void) {
  int fds[2];
  int status = 0;
  char error[256] = {0};
  FILE *err = tmpfile();
  pid_t child;
  int ok;
  if (err == NULL) return 0;
  if (pipe(fds) != 0) {
    fclose(err);
    return 0;
  }
  close(fds[0]);
  child = fork();
  if (child == 0) {
    signal(SIGPIPE, SIG_DFL);
    if (dup2(fds[1], STDOUT_FILENO) < 0 || dup2(fileno(err), STDERR_FILENO) < 0)
      _exit(127);
    close(fds[1]);
    execl("build/langc", "langc", "contract", "examples/dao.lang", (char *)NULL);
    _exit(127);
  }
  close(fds[1]);
  ok = child > 0 && waitpid(child, &status, 0) == child
    && WIFEXITED(status) && WEXITSTATUS(status) == 2;
  rewind(err);
  ok = fread(error, 1, sizeof error - 1, err) > 0 && ok
    && strstr(error, "langc: IO_WRITE:") != NULL;
  fclose(err);
  if (!ok) fprintf(stderr, "FAIL writer closed pipe: status %d, %s\n", status, error);
  return ok;
}

int main(void) {
  char *infinity[] = {"0", "0", "1", "2"};
  char *duplicate[] = {"1", "2", "1", "2"};
  char *valid[] = {"1u", "2u", "0x1u",
    "0x30644e72e131a029b85045b68181585d97816a916871ca8d3c208c16d87cfd45u"};
  int ok = writer_case("baseline", NULL, "dao.lang", NULL, 0, 0, 0, NULL);
  ok &= writer_case("helper names",
    "\ndef contractCfg0 : Unit := unit\n"
    "def contractM1 : Unit := unit\n"
    "def contractTbl0At0 : Unit := unit\n"
    "def contract0Cfg0 : Unit := unit\n", "dao.lang", NULL, 0, 0, 0, NULL);
  ok &= writer_case("path comment", NULL, "line\nbreak\r.lang", NULL, 0, 0, 0, NULL);
  ok &= writer_case("infinity key", NULL, "dao.lang", infinity, 4, 0, 1, "REFUSE_CONTRACT");
  ok &= writer_case("duplicate key", NULL, "dao.lang", duplicate, 4, 0, 1, "REFUSE_CONTRACT");
  ok &= writer_case("distinct keys", NULL, "dao.lang", valid, 4, 0, 0, NULL);
  ok &= writer_case("output failure", NULL, "dao.lang", NULL, 0, 1, 2, "IO_WRITE");
  ok &= closed_pipe();
  if (ok) puts("writer regressions: passed");
  return ok ? 0 : 1;
}
