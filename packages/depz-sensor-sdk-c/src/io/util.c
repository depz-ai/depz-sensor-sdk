/* util.c — per-thread error detail, stream hubs, callback lists. */
#include "io_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── errors ────────────────────────────────────────────────────────────── */

#if defined(_MSC_VER)
#  define DEPZ_TLS __declspec(thread)
#else
#  define DEPZ_TLS _Thread_local
#endif

static DEPZ_TLS char t_msg[512];
static DEPZ_TLS int  t_status = -1;
static DEPZ_TLS int  t_status_cmd = -1;

const char *depz_err_str(int err)
{
    switch (err) {
    case DEPZ_OK:                return "DEPZ_OK";
    case DEPZ_E_ARG:             return "DEPZ_E_ARG";
    case DEPZ_E_NOMEM:           return "DEPZ_E_NOMEM";
    case DEPZ_E_IO:              return "DEPZ_E_IO";
    case DEPZ_E_CLOSED:          return "DEPZ_E_CLOSED";
    case DEPZ_E_TIMEOUT:         return "DEPZ_E_TIMEOUT";
    case DEPZ_E_STATUS:          return "DEPZ_E_STATUS";
    case DEPZ_E_BUSY:            return "DEPZ_E_BUSY";
    case DEPZ_E_PROTOCOL:        return "DEPZ_E_PROTOCOL";
    case DEPZ_E_NO_DEVICE:       return "DEPZ_E_NO_DEVICE";
    case DEPZ_E_WRONG_TYPE:      return "DEPZ_E_WRONG_TYPE";
    case DEPZ_E_REPLAY_MISMATCH: return "DEPZ_E_REPLAY_MISMATCH";
    case DEPZ_E_BOOTLOADER:      return "DEPZ_E_BOOTLOADER";
    default:                     return "DEPZ_E_?";
    }
}

const char *depz_last_error(void) { return t_msg; }
int depz_last_status(void) { return t_status; }
int depz_last_status_cmd(void) { return t_status_cmd; }

int depz_fail(int err, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(t_msg, sizeof t_msg, fmt, ap);
    va_end(ap);
    t_status = -1;
    t_status_cmd = -1;
    return err;
}

static const char *status_name(uint8_t s)
{
    static const char *names[] = {
        "OK", "ERROR", "ERR_INVALID_CMD", "ERR_PAYLOAD_FORMAT", "ERR_INVALID_PARAM",
        "ERR_PAYLOAD_CRC", "ERR_BUSY", "ERR_CMD_NOT_SUPPORTED", "ERR_NOT_INITIALIZED",
        "ERR_HARDWARE_FAULT"};
    return s < sizeof names / sizeof names[0] ? names[s] : "?";
}

int depz_fail_status(int err, uint8_t cmd, uint8_t status)
{
    snprintf(t_msg, sizeof t_msg, "cmd 0x%02X failed: %s (0x%02X)", cmd, status_name(status),
             status);
    t_status = status;
    t_status_cmd = cmd;
    return err;
}

void depz_strlcpy(char *dst, const char *src, size_t cap)
{
    size_t n;
    if (!cap) return;
    n = strlen(src);
    if (n >= cap) n = cap - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

/* ── streams ───────────────────────────────────────────────────────────── */

/* Lock order: hub, then stream. A stream never takes its hub's lock while
 * holding its own. */
struct depz_hub {
    depz_mutex   lock;
    size_t       item_size;
    depz_stream *head;
    int          refs;    /* the producer + one per live stream */
    bool         closed;
};

struct depz_stream {
    depz_hub    *hub;     /* NULL once unlinked */
    depz_stream *next;
    depz_mutex   lock;
    depz_cond    cond;
    uint8_t     *buf;
    size_t       item_size, cap, head, count;
    uint64_t     dropped;
    bool         closed;
};

depz_hub *depz_hub_new(size_t item_size)
{
    depz_hub *h = (depz_hub *)calloc(1, sizeof *h);
    if (!h) return NULL;
    if (depz_mutex_init(&h->lock)) { free(h); return NULL; }
    h->item_size = item_size;
    h->refs = 1;
    return h;
}

static void hub_unref_locked_and_unlock(depz_hub *h)
{
    bool last = --h->refs == 0;
    depz_mutex_unlock(&h->lock);
    if (last) {
        depz_mutex_destroy(&h->lock);
        free(h);
    }
}

depz_stream *depz_hub_subscribe(depz_hub *h, size_t maxsize)
{
    depz_stream *s;
    if (!h || !maxsize) return NULL;
    s = (depz_stream *)calloc(1, sizeof *s);
    if (!s) return NULL;
    s->buf = (uint8_t *)malloc(maxsize * h->item_size);
    if (!s->buf || depz_mutex_init(&s->lock)) { free(s->buf); free(s); return NULL; }
    if (depz_cond_init(&s->cond)) {
        depz_mutex_destroy(&s->lock);
        free(s->buf);
        free(s);
        return NULL;
    }
    s->item_size = h->item_size;
    s->cap = maxsize;
    depz_mutex_lock(&h->lock);
    s->hub = h;
    s->closed = h->closed;
    s->next = h->head;
    h->head = s;
    h->refs++;
    depz_mutex_unlock(&h->lock);
    return s;
}

void depz_hub_push(depz_hub *h, const void *item)
{
    depz_stream *s;
    if (!h) return;
    depz_mutex_lock(&h->lock);
    for (s = h->head; s; s = s->next) {
        size_t slot;
        depz_mutex_lock(&s->lock);
        if (s->count == s->cap) {           /* drop-oldest */
            s->head = (s->head + 1) % s->cap;
            s->count--;
            s->dropped++;
        }
        slot = (s->head + s->count) % s->cap;
        memcpy(s->buf + slot * s->item_size, item, s->item_size);
        s->count++;
        depz_cond_broadcast(&s->cond);
        depz_mutex_unlock(&s->lock);
    }
    depz_mutex_unlock(&h->lock);
}

static void mark_closed_locked(depz_hub *h)
{
    depz_stream *s;
    h->closed = true;
    for (s = h->head; s; s = s->next) {
        depz_mutex_lock(&s->lock);
        s->closed = true;
        depz_cond_broadcast(&s->cond);
        depz_mutex_unlock(&s->lock);
    }
}

void depz_hub_mark_closed(depz_hub *h)
{
    if (!h) return;
    depz_mutex_lock(&h->lock);
    mark_closed_locked(h);
    depz_mutex_unlock(&h->lock);
}

void depz_hub_close(depz_hub *h)
{
    if (!h) return;
    depz_mutex_lock(&h->lock);
    mark_closed_locked(h);
    hub_unref_locked_and_unlock(h);
}

int depz_stream_next(depz_stream *s, void *item, int timeout_ms)
{
    uint64_t deadline;
    int rc = DEPZ_OK;
    if (!s || !item) return depz_fail(DEPZ_E_ARG, "stream: NULL argument");
    deadline = depz_now_us() + (uint64_t)(timeout_ms < 0 ? 0 : timeout_ms) * 1000u;
    depz_mutex_lock(&s->lock);
    while (!s->count) {
        int left;
        if (s->closed) { rc = depz_fail(DEPZ_E_CLOSED, "stream: device closed"); break; }
        left = timeout_ms < 0 ? -1 : depz_ms_until(deadline);
        if (!left) { rc = depz_fail(DEPZ_E_TIMEOUT, "stream: no item in %d ms", timeout_ms); break; }
        depz_cond_wait_ms(&s->cond, &s->lock, left);
    }
    if (rc == DEPZ_OK) {
        memcpy(item, s->buf + s->head * s->item_size, s->item_size);
        s->head = (s->head + 1) % s->cap;
        s->count--;
    }
    depz_mutex_unlock(&s->lock);
    return rc;
}

uint64_t depz_stream_dropped_count(const depz_stream *s)
{
    uint64_t n;
    if (!s) return 0;
    depz_mutex_lock((depz_mutex *)&s->lock);
    n = s->dropped;
    depz_mutex_unlock((depz_mutex *)&s->lock);
    return n;
}

void depz_stream_close(depz_stream *s)
{
    depz_hub *h;
    if (!s) return;
    h = s->hub;
    depz_mutex_lock(&h->lock);
    {
        depz_stream **pp = &h->head;
        while (*pp && *pp != s) pp = &(*pp)->next;
        if (*pp) *pp = s->next;
    }
    hub_unref_locked_and_unlock(h);
    depz_cond_destroy(&s->cond);
    depz_mutex_destroy(&s->lock);
    free(s->buf);
    free(s);
}

/* ── callback lists ────────────────────────────────────────────────────── */

int depz_cb_list_init(depz_cb_list *l)
{
    memset(l, 0, sizeof *l);
    l->next_token = 1;
    return depz_mutex_init(&l->lock);
}

void depz_cb_list_free(depz_cb_list *l)
{
    free(l->items);
    depz_mutex_destroy(&l->lock);
}

int depz_cb_list_add(depz_cb_list *l, void (*fn)(void), void *user, int *token)
{
    int rc = DEPZ_OK;
    depz_mutex_lock(&l->lock);
    if (l->count == l->cap) {
        size_t cap = l->cap ? l->cap * 2 : 4;
        depz_cb_entry *n = (depz_cb_entry *)realloc(l->items, cap * sizeof *n);
        if (!n) rc = depz_fail(DEPZ_E_NOMEM, "callback list: out of memory");
        else { l->items = n; l->cap = cap; }
    }
    if (rc == DEPZ_OK) {
        depz_cb_entry *e = &l->items[l->count++];
        e->token = l->next_token++;
        e->fn = fn;
        e->user = user;
        if (token) *token = e->token;
    }
    depz_mutex_unlock(&l->lock);
    return rc;
}

void depz_cb_list_remove(depz_cb_list *l, int token)
{
    size_t i;
    depz_mutex_lock(&l->lock);
    for (i = 0; i < l->count; i++) {
        if (l->items[i].token == token) {
            memmove(&l->items[i], &l->items[i + 1], (l->count - i - 1) * sizeof l->items[0]);
            l->count--;
            break;
        }
    }
    depz_mutex_unlock(&l->lock);
}

size_t depz_cb_list_snapshot(depz_cb_list *l, depz_cb_entry **out)
{
    size_t n;
    *out = NULL;
    depz_mutex_lock(&l->lock);
    n = l->count;
    if (n) {
        *out = (depz_cb_entry *)malloc(n * sizeof **out);
        if (*out) memcpy(*out, l->items, n * sizeof **out);
        else n = 0;
    }
    depz_mutex_unlock(&l->lock);
    return n;
}
