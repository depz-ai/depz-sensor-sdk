/* fake_vl53lx_trace.h — a scripted VL53L0X / VL53L1 bridge over a loopback link for
 * the vl53lx.json l0x_uld / l1_uld cases (test_vectors.c): answers READ_REG /
 * WRITE_REG from a register trace — every op the driver issues must be the
 * next one of the trace, a read answered with its recorded bytes — and keeps
 * the first mismatch. The C twin of the scripted platforms the other SDKs'
 * vector runners use. Needs the io layer (DEPZ_VECTORS_HAS_IO). */
#ifndef DEPZ_TEST_FAKE_VL53LX_TRACE_H
#define DEPZ_TEST_FAKE_VL53LX_TRACE_H

#include "depz_sensor_io.h"

#define FAKE_TRACE_DATA_MAX 32

typedef struct {
    bool     write;                 /* false = READ_REG answered with data  */
    uint16_t addr;
    uint8_t  data[FAKE_TRACE_DATA_MAX];
    size_t   len;
} fake_trace_op;

typedef struct fake_vl53lx_trace fake_vl53lx_trace;

/* The fake serves `ops` (borrowed for its lifetime) on `*host_link`. */
fake_vl53lx_trace *fake_vl53lx_trace_start(const fake_trace_op *ops, size_t n_ops, depz_link **host_link);
void               fake_vl53lx_trace_stop(fake_vl53lx_trace *f);

/* Ops consumed so far; `mismatch` (cap > 0) gets the first mismatch, "" if none. */
size_t fake_vl53lx_trace_issued(fake_vl53lx_trace *f, char *mismatch, size_t cap);

#endif
