#ifndef ORBIT_WRITER_H
#define ORBIT_WRITER_H

#include <stdio.h>

#include "front/check.h"

/* The build arguments of `langc contract` (S9 B1, ruling Q-S9-3): the ring
   words X1 Y1 to Xn Yn (RING NULL gives the ring of the member test keys)
   and the start outcome (START NULL gives 0). */
typedef struct {
  char *const *ring;
  int ring_words;
  const char *start;
} ContractOptions;

/* Writes the kit program text of the DAO in the checked guest program M
   (TEXT of LEN bytes from PATH) to OUT: the cast and amend entries and the
   count, outcome, selfConstituting, poll and constitution views. The table
   of each entry comes from a second check of TEXT with the generated table
   definitions. Returns 0, 1 after a REFUSE_CONTRACT or EVM_RING diagnostic,
   or 2 after a USAGE or IO_WRITE diagnostic. */
int contract_command(Arena *arena, Machine *m, const char *path, const char *text, size_t len, const ContractOptions *opt, FILE *out);

#endif
