/* The C SDK's tests/vector_seen.h behind one C entry point: the C++ test
 * sources call depz_test_vector_seen() and never see <windows.h>. */
#include "vector_seen.h"

void depz_test_vector_seen(const char *id);

void depz_test_vector_seen(const char *id) { vector_seen(id); }
