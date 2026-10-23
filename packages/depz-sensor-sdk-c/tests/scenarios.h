/* scenarios.h — the contract 15 device-scenario runner (test support only). */
#ifndef DEPZ_TEST_SCENARIOS_H
#define DEPZ_TEST_SCENARIOS_H

#include <stddef.h>

/* Run contracts/scenarios/<id>.json over a strict causal replay of its
 * recording. 0 = passed (and reported to the coverage gate); otherwise the
 * reason went to stderr. The directory is $DEPZ_SCENARIOS_DIR, else the one
 * CMake compiled in. */
int scenario_run(const char *id);

/* The scenario ids CMake found at configure time (NULL-terminated). */
extern const char *const scenario_ids[];

#endif
