/* link.c — the byte-link seam, loopback pairs, .depzrec record/replay
 * (contract 08). The serial port lives in serial_posix.c / serial_win32.c. */
#include "io_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ── generic ──────────────────────────────────────────────────────────── */

int depz_link_new(const depz_link_vtable *vt, void *self, const char *name, depz_link **out)
{
    depz_link *l;
    if (!vt || !vt->read || !vt->write || !vt->close || !vt->destroy || !out)
        return depz_fail(DEPZ_E_ARG, "link: incomplete vtable");
    l = (depz_link *)calloc(1, sizeof *l);
    if (!l) return depz_fail(DEPZ_E_NOMEM, "link: out of memory");
    if (depz_mutex_init(&l->lock)) { free(l); return depz_fail(DEPZ_E_NOMEM, "link: mutex"); }
    l->vt = vt;
    l->self = self;
    depz_strlcpy(l->name, name ? name : "<link>", sizeof l->name);
    *out = l;
    return DEPZ_OK;
}

int depz_link_read(depz_link *l, uint8_t *buf, size_t cap, int timeout_ms, size_t *got)
{
    *got = 0;
    if (depz_link_closed(l)) return depz_fail(DEPZ_E_CLOSED, "%s: closed", l->name);
    return l->vt->read(l->self, buf, cap, timeout_ms, got);
}

int depz_link_write(depz_link *l, const uint8_t *data, size_t len)
{
    if (depz_link_closed(l)) return depz_fail(DEPZ_E_CLOSED, "%s: closed", l->name);
    return l->vt->write(l->self, data, len);
}

void depz_link_close(depz_link *l)
{
    bool was;
    if (!l) return;
    depz_mutex_lock(&l->lock);
    was = l->closed;
    l->closed = true;
    depz_mutex_unlock(&l->lock);
    if (!was) l->vt->close(l->self);
}

bool depz_link_closed(const depz_link *l)
{
    bool c;
    depz_mutex_lock((depz_mutex *)&l->lock);
    c = l->closed;
    depz_mutex_unlock((depz_mutex *)&l->lock);
    return c;
}

const char *depz_link_name(const depz_link *l) { return l->name; }

void depz_link_free(depz_link *l)
{
    if (!l) return;
    depz_link_close(l);
    l->vt->destroy(l->self);
    depz_mutex_destroy(&l->lock);
    free(l);
}

int depz_link_open_serial(const char *port, depz_link **out)
{
    if (!port || !out) return depz_fail(DEPZ_E_ARG, "open_serial: NULL argument");
    return depz_serial_link_open(port, out);
}

bool depz_link_replay_exhausted(const depz_link *l)
{
    return l && l->is_replay && depz_replay_exhausted_impl(l->self);
}

/* ── loopback ─────────────────────────────────────────────────────────── */

typedef struct lb_chunk { struct lb_chunk *next; size_t len, off; uint8_t data[1]; } lb_chunk;

typedef struct lb_shared {
    depz_mutex lock;
    depz_cond  cond;
    lb_chunk  *q[2][2];  /* per side: head, tail of what that side reads */
    bool       closed;
    int        refs;
} lb_shared;

typedef struct { lb_shared *sh; int side; } lb_end;

static int lb_read(void *self, uint8_t *buf, size_t cap, int timeout_ms, size_t *got)
{
    lb_end *e = (lb_end *)self;
    lb_shared *sh = e->sh;
    uint64_t deadline = depz_now_us() + (uint64_t)(timeout_ms < 0 ? 0 : timeout_ms) * 1000u;
    int rc = DEPZ_OK;
    *got = 0;
    depz_mutex_lock(&sh->lock);
    while (!sh->q[e->side][0]) {
        int left;
        if (sh->closed) { rc = depz_fail(DEPZ_E_CLOSED, "loopback: closed"); break; }
        left = depz_ms_until(deadline);
        if (!left) break;
        depz_cond_wait_ms(&sh->cond, &sh->lock, left);
    }
    if (rc == DEPZ_OK && sh->q[e->side][0]) {
        lb_chunk *c = sh->q[e->side][0];
        size_t n = c->len - c->off;
        if (n > cap) n = cap;
        memcpy(buf, c->data + c->off, n);
        c->off += n;
        *got = n;
        if (c->off == c->len) {
            sh->q[e->side][0] = c->next;
            if (!c->next) sh->q[e->side][1] = NULL;
            free(c);
        }
    }
    depz_mutex_unlock(&sh->lock);
    return rc;
}

static int lb_write(void *self, const uint8_t *data, size_t len)
{
    lb_end *e = (lb_end *)self;
    lb_shared *sh = e->sh;
    int peer = 1 - e->side;
    lb_chunk *c;
    if (!len) return DEPZ_OK;
    c = (lb_chunk *)malloc(sizeof *c + len);
    if (!c) return depz_fail(DEPZ_E_NOMEM, "loopback: out of memory");
    c->next = NULL;
    c->len = len;
    c->off = 0;
    memcpy(c->data, data, len);
    depz_mutex_lock(&sh->lock);
    if (sh->closed) {
        depz_mutex_unlock(&sh->lock);
        free(c);
        return depz_fail(DEPZ_E_CLOSED, "loopback: closed");
    }
    if (sh->q[peer][1]) sh->q[peer][1]->next = c; else sh->q[peer][0] = c;
    sh->q[peer][1] = c;
    depz_cond_broadcast(&sh->cond);
    depz_mutex_unlock(&sh->lock);
    return DEPZ_OK;
}

/* Closing either end closes the pair, as a pulled cable would. */
static void lb_close(void *self)
{
    lb_shared *sh = ((lb_end *)self)->sh;
    depz_mutex_lock(&sh->lock);
    sh->closed = true;
    depz_cond_broadcast(&sh->cond);
    depz_mutex_unlock(&sh->lock);
}

static void lb_destroy(void *self)
{
    lb_end *e = (lb_end *)self;
    lb_shared *sh = e->sh;
    bool last;
    depz_mutex_lock(&sh->lock);
    last = --sh->refs == 0;
    depz_mutex_unlock(&sh->lock);
    free(e);
    if (last) {
        int s;
        for (s = 0; s < 2; s++) {
            lb_chunk *c = sh->q[s][0];
            while (c) { lb_chunk *n = c->next; free(c); c = n; }
        }
        depz_cond_destroy(&sh->cond);
        depz_mutex_destroy(&sh->lock);
        free(sh);
    }
}

static const depz_link_vtable lb_vt = {lb_read, lb_write, lb_close, lb_destroy};

int depz_link_loopback_pair(depz_link **a, depz_link **b)
{
    lb_shared *sh = (lb_shared *)calloc(1, sizeof *sh);
    lb_end *ea, *eb;
    int rc;
    if (!sh) return depz_fail(DEPZ_E_NOMEM, "loopback: out of memory");
    depz_mutex_init(&sh->lock);
    depz_cond_init(&sh->cond);
    sh->refs = 2;
    ea = (lb_end *)calloc(1, sizeof *ea);
    eb = (lb_end *)calloc(1, sizeof *eb);
    if (!ea || !eb) {
        free(ea);
        free(eb);
        depz_cond_destroy(&sh->cond);
        depz_mutex_destroy(&sh->lock);
        free(sh);
        return depz_fail(DEPZ_E_NOMEM, "loopback: out of memory");
    }
    ea->sh = eb->sh = sh;
    ea->side = 0;
    eb->side = 1;
    rc = depz_link_new(&lb_vt, ea, "loopback-a", a);
    if (rc) { lb_destroy(ea); lb_destroy(eb); return rc; }
    rc = depz_link_new(&lb_vt, eb, "loopback-b", b);
    if (rc) { depz_link_free(*a); lb_destroy(eb); return rc; }
    return DEPZ_OK;
}

/* ── .depzrec parsing helpers ─────────────────────────────────────────── */

static int hexval(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* Value of string field `key` in a one-line JSON object: `start` and
 * `len` point inside `line`. The format has no escapes in these fields (contract 08). */
static bool json_str_field(const char *line, const char *key, const char **start, size_t *len)
{
    char pat[32];
    const char *p, *q;
    snprintf(pat, sizeof pat, "\"%s\"", key);
    p = strstr(line, pat);
    if (!p) return false;
    p += strlen(pat);
    while (*p == ' ' || *p == ':') p++;
    if (*p != '"') return false;
    q = strchr(++p, '"');
    if (!q) return false;
    *start = p;
    *len = (size_t)(q - p);
    return true;
}

static bool json_uint_field(const char *line, const char *key, uint64_t *out)
{
    char pat[32];
    const char *p;
    snprintf(pat, sizeof pat, "\"%s\"", key);
    p = strstr(line, pat);
    if (!p) return false;
    p += strlen(pat);
    while (*p == ' ' || *p == ':') p++;
    if (*p < '0' || *p > '9') return false;
    *out = strtoull(p, NULL, 10);
    return true;
}

/* ── replay ───────────────────────────────────────────────────────────── */

typedef struct {
    uint64_t t_us;
    size_t   off, len;     /* into rx_bytes */
    size_t   tx_prefix;    /* tx bytes the host must have written first */
} rp_event;

typedef struct {
    depz_mutex lock;
    depz_cond  cond;
    uint8_t   *rx_bytes, *tx_bytes;
    size_t     tx_len;
    rp_event  *ev;
    size_t     n_ev, idx;
    size_t     tx_written;
    bool       strict, realtime, closed;
    uint64_t   t0;
} replay;

bool depz_replay_exhausted_impl(void *self)
{
    replay *r = (replay *)self;
    bool ex;
    depz_mutex_lock(&r->lock);
    ex = r->idx >= r->n_ev;
    depz_mutex_unlock(&r->lock);
    return ex;
}

static int rp_read(void *self, uint8_t *buf, size_t cap, int timeout_ms, size_t *got)
{
    replay *r = (replay *)self;
    uint64_t deadline = depz_now_us() + (uint64_t)(timeout_ms < 0 ? 0 : timeout_ms) * 1000u;
    rp_event e;
    *got = 0;
    depz_mutex_lock(&r->lock);
    for (;;) {
        int left;
        if (r->closed) {
            depz_mutex_unlock(&r->lock);
            return depz_fail(DEPZ_E_CLOSED, "replay: closed");
        }
        if (r->idx >= r->n_ev) { depz_mutex_unlock(&r->lock); return DEPZ_OK; }
        e = r->ev[r->idx];
        if (r->tx_written >= e.tx_prefix) break;
        left = depz_ms_until(deadline);
        if (!left) { depz_mutex_unlock(&r->lock); return DEPZ_OK; }
        depz_cond_wait_ms(&r->cond, &r->lock, left);
    }
    depz_mutex_unlock(&r->lock);
    if (r->realtime) {
        uint64_t due = r->t0 + e.t_us, now = depz_now_us();
        if (now < due) {
            if (due > deadline) {
                depz_sleep_ms(depz_ms_until(deadline));
                return DEPZ_OK;
            }
            depz_sleep_ms((int)((due - now + 999) / 1000));
        }
    }
    /* A recorded chunk larger than `cap` is served in pieces. */
    depz_mutex_lock(&r->lock);
    {
        rp_event *cur = &r->ev[r->idx];
        size_t n = cur->len < cap ? cur->len : cap;
        memcpy(buf, r->rx_bytes + cur->off, n);
        *got = n;
        cur->off += n;
        cur->len -= n;
        if (!cur->len) r->idx++;
    }
    depz_mutex_unlock(&r->lock);
    return DEPZ_OK;
}

static void hex_dump(char *out, size_t cap, const uint8_t *p, size_t n)
{
    size_t i, o = 0;
    for (i = 0; i < n && o + 3 < cap; i++) o += (size_t)snprintf(out + o, cap - o, "%02x", p[i]);
    if (cap) out[o < cap ? o : cap - 1] = '\0';
}

static int rp_write(void *self, const uint8_t *data, size_t len)
{
    replay *r = (replay *)self;
    int rc = DEPZ_OK;
    depz_mutex_lock(&r->lock);
    if (r->closed) {
        rc = depz_fail(DEPZ_E_CLOSED, "replay: closed");
    } else {
        if (r->strict) {
            size_t avail = r->tx_written < r->tx_len ? r->tx_len - r->tx_written : 0;
            size_t n = len < avail ? len : avail;
            if (n != len || memcmp(r->tx_bytes + r->tx_written, data, len) != 0) {
                char want[80], have[80];
                hex_dump(want, sizeof want, r->tx_bytes + r->tx_written, n);
                hex_dump(have, sizeof have, data, len);
                rc = depz_fail(DEPZ_E_REPLAY_MISMATCH,
                               "replay strict_tx mismatch at offset %zu: expected %s, got %s",
                               r->tx_written, want, have);
            }
        }
        if (rc == DEPZ_OK) {
            r->tx_written += len;
            depz_cond_broadcast(&r->cond);
        }
    }
    depz_mutex_unlock(&r->lock);
    return rc;
}

static void rp_close(void *self)
{
    replay *r = (replay *)self;
    depz_mutex_lock(&r->lock);
    r->closed = true;
    depz_cond_broadcast(&r->cond);
    depz_mutex_unlock(&r->lock);
}

static void rp_destroy(void *self)
{
    replay *r = (replay *)self;
    depz_cond_destroy(&r->cond);
    depz_mutex_destroy(&r->lock);
    free(r->rx_bytes);
    free(r->tx_bytes);
    free(r->ev);
    free(r);
}

static const depz_link_vtable rp_vt = {rp_read, rp_write, rp_close, rp_destroy};

static char *read_file(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    char *buf;
    long n;
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) || (n = ftell(f)) < 0 || fseek(f, 0, SEEK_SET)) {
        fclose(f);
        return NULL;
    }
    buf = (char *)malloc((size_t)n + 1);
    if (buf && fread(buf, 1, (size_t)n, f) != (size_t)n) { free(buf); buf = NULL; }
    fclose(f);
    if (buf) { buf[n] = '\0'; *len = (size_t)n; }
    return buf;
}

int depz_link_open_replay(const char *path, bool strict_tx, bool realtime, depz_link **out)
{
    size_t flen, rx_cap = 0, tx_cap = 0, rx_len = 0, ev_cap = 0;
    char *text, *line, *save;
    replay *r;
    bool header = true;
    int rc;

    if (!path || !out) return depz_fail(DEPZ_E_ARG, "replay: NULL argument");
    text = read_file(path, &flen);
    if (!text) return depz_fail(DEPZ_E_IO, "replay: cannot read %s", path);
    r = (replay *)calloc(1, sizeof *r);
    if (!r) { free(text); return depz_fail(DEPZ_E_NOMEM, "replay: out of memory"); }

    for (line = text; line && *line; line = save) {
        const char *dir, *hex;
        size_t dlen, hlen, i, nbytes;
        uint64_t t = 0;
        uint8_t **dst;
        size_t *dst_len, *dst_cap;

        save = strchr(line, '\n');
        if (save) *save++ = '\0';
        if (header) { header = false; continue; }
        if (!json_str_field(line, "dir", &dir, &dlen) || !json_str_field(line, "data", &hex, &hlen))
            continue; /* blank line or an unknown event kind */
        json_uint_field(line, "t", &t);
        nbytes = hlen / 2;
        if (dlen == 2 && !memcmp(dir, "rx", 2)) {
            dst = &r->rx_bytes; dst_len = &rx_len; dst_cap = &rx_cap;
            if (r->n_ev == ev_cap) {
                size_t nc = ev_cap ? ev_cap * 2 : 256;
                rp_event *ne = (rp_event *)realloc(r->ev, nc * sizeof *ne);
                if (!ne) goto nomem;
                r->ev = ne;
                ev_cap = nc;
            }
            r->ev[r->n_ev].t_us = t;
            r->ev[r->n_ev].off = rx_len;
            r->ev[r->n_ev].len = nbytes;
            r->ev[r->n_ev].tx_prefix = r->tx_len;
            r->n_ev++;
        } else {
            dst = &r->tx_bytes; dst_len = &r->tx_len; dst_cap = &tx_cap;
        }
        if (*dst_len + nbytes > *dst_cap) {
            size_t nc = (*dst_cap ? *dst_cap : 4096);
            uint8_t *nb;
            while (nc < *dst_len + nbytes) nc *= 2;
            nb = (uint8_t *)realloc(*dst, nc);
            if (!nb) goto nomem;
            *dst = nb;
            *dst_cap = nc;
        }
        for (i = 0; i < nbytes; i++) {
            int hi = hexval(hex[2 * i]), lo = hexval(hex[2 * i + 1]);
            if (hi < 0 || lo < 0) {
                free(text);
                rp_destroy(r);
                return depz_fail(DEPZ_E_PROTOCOL, "replay: bad hex in %s", path);
            }
            (*dst)[*dst_len + i] = (uint8_t)(hi << 4 | lo);
        }
        *dst_len += nbytes;
    }
    free(text);
    depz_mutex_init(&r->lock);
    depz_cond_init(&r->cond);
    r->strict = strict_tx;
    r->realtime = realtime;
    r->t0 = depz_now_us();
    rc = depz_link_new(&rp_vt, r, path, out);
    if (rc) { rp_destroy(r); return rc; }
    (*out)->is_replay = true;
    return DEPZ_OK;

nomem:
    free(text);
    free(r->rx_bytes);
    free(r->tx_bytes);
    free(r->ev);
    free(r);
    return depz_fail(DEPZ_E_NOMEM, "replay: out of memory");
}

/* ── recording ────────────────────────────────────────────────────────── */

typedef struct {
    depz_link *inner;
    FILE      *f;
    depz_mutex lock;
    uint64_t   t0;
} recorder;

static void rec_emit(recorder *r, const char *dir, const uint8_t *data, size_t len)
{
    size_t i;
    if (!len) return;
    depz_mutex_lock(&r->lock);
    if (r->f) {
        fprintf(r->f, "{\"t\": %llu, \"dir\": \"%s\", \"data\": \"",
                (unsigned long long)(depz_now_us() - r->t0), dir);
        for (i = 0; i < len; i++) fprintf(r->f, "%02x", data[i]);
        fputs("\"}\n", r->f);
    }
    depz_mutex_unlock(&r->lock);
}

static int rec_read(void *self, uint8_t *buf, size_t cap, int timeout_ms, size_t *got)
{
    recorder *r = (recorder *)self;
    int rc = depz_link_read(r->inner, buf, cap, timeout_ms, got);
    if (rc == DEPZ_OK) rec_emit(r, "rx", buf, *got);
    return rc;
}

static int rec_write(void *self, const uint8_t *data, size_t len)
{
    recorder *r = (recorder *)self;
    /* Journal first: a fast reply must never be recorded ahead of its request,
     * or causal replay would serve it too early. */
    rec_emit(r, "tx", data, len);
    return depz_link_write(r->inner, data, len);
}

static void rec_close(void *self)
{
    recorder *r = (recorder *)self;
    depz_link_close(r->inner);
    depz_mutex_lock(&r->lock);
    if (r->f) { fclose(r->f); r->f = NULL; }
    depz_mutex_unlock(&r->lock);
}

static void rec_destroy(void *self)
{
    recorder *r = (recorder *)self;
    rec_close(r);
    depz_link_free(r->inner);
    depz_mutex_destroy(&r->lock);
    free(r);
}

static const depz_link_vtable rec_vt = {rec_read, rec_write, rec_close, rec_destroy};

int depz_link_open_recording(depz_link *inner, const char *path, const char *header_extra_json,
                             depz_link **out)
{
    recorder *r;
    char stamp[32];
    time_t now = time(NULL);
    struct tm tmv;
    int rc;

    if (!inner || !path || !out) {
        depz_link_free(inner);
        return depz_fail(DEPZ_E_ARG, "recording: NULL argument");
    }
    r = (recorder *)calloc(1, sizeof *r);
    if (!r) { depz_link_free(inner); return depz_fail(DEPZ_E_NOMEM, "recording: out of memory"); }
    r->f = fopen(path, "wb"); /* "\n" line ends on every OS */
    if (!r->f) {
        free(r);
        depz_link_free(inner);
        return depz_fail(DEPZ_E_IO, "recording: cannot create %s", path);
    }
#ifdef _WIN32
    gmtime_s(&tmv, &now);
#else
    gmtime_r(&now, &tmv);
#endif
    strftime(stamp, sizeof stamp, "%Y-%m-%dT%H:%M:%SZ", &tmv);
    fprintf(r->f, "{\"schema\": \"depz.rec/1\", \"created_utc\": \"%s\"%s%s}\n", stamp,
            header_extra_json && *header_extra_json ? ", " : "",
            header_extra_json ? header_extra_json : "");
    r->inner = inner;
    depz_mutex_init(&r->lock);
    r->t0 = depz_now_us();
    rc = depz_link_new(&rec_vt, r, depz_link_name(inner), out);
    if (rc) rec_destroy(r);
    return rc;
}
