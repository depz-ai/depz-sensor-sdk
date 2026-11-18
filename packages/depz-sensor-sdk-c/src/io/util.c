/* util.c — streams and their subscriptions, callback lists (the error
 * detail is error.c). */
#include "io_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── strings ───────────────────────────────────────────────────────────── */

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

/* The subscriptions of one stream. Lock order: hub, then subscription. A
 * subscription never takes its hub's lock while holding its own. */
struct depz_hub {
    depz_mutex         lock;
    size_t             item_size;
    int  (*clone)(void *item);  /* depz_stream_desc: items with a heap part */
    void (*drop)(void *item);
    depz_subscription *head;
    int                refs;    /* the stream + one per live subscription */
    bool               closed;
    char               lost_reason[256]; /* set once before `closed`, "" = this side closed it */
};

struct depz_subscription {
    depz_hub          *hub;
    depz_subscription *next;
    depz_mutex         lock;
    depz_cond          cond;
    uint8_t           *buf;
    size_t             item_size, cap, head, count;
    uint64_t           dropped;
    bool               closed;
    const char        *lost_reason; /* into the hub, which outlives this; NULL = this side closed it */
};

static depz_hub *hub_new(const depz_stream_desc *desc)
{
    depz_hub *h = (depz_hub *)calloc(1, sizeof *h);
    if (!h) return NULL;
    if (depz_mutex_init(&h->lock)) { free(h); return NULL; }
    h->item_size = desc->item_size;
    h->clone = desc->clone;
    h->drop = desc->drop;
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

static depz_subscription *hub_subscribe(depz_hub *h, size_t maxsize)
{
    depz_subscription *s;
    if (!h || !maxsize) return NULL;
    s = (depz_subscription *)calloc(1, sizeof *s);
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
    s->lost_reason = h->lost_reason[0] ? h->lost_reason : NULL;
    s->next = h->head;
    h->head = s;
    h->refs++;
    depz_mutex_unlock(&h->lock);
    return s;
}

static void hub_push(depz_hub *h, const void *item)
{
    depz_subscription *s;
    depz_mutex_lock(&h->lock);
    for (s = h->head; s; s = s->next) {
        uint8_t *slot;
        depz_mutex_lock(&s->lock);
        if (s->count == s->cap) {           /* drop-oldest */
            if (h->drop) h->drop(s->buf + s->head * s->item_size);
            s->head = (s->head + 1) % s->cap;
            s->count--;
            s->dropped++;
        }
        slot = s->buf + ((s->head + s->count) % s->cap) * s->item_size;
        memcpy(slot, item, s->item_size);
        if (h->clone && h->clone(slot)) {
            s->dropped++;                   /* no memory for its copy: not queued */
        } else {
            s->count++;
            depz_cond_broadcast(&s->cond);
        }
        depz_mutex_unlock(&s->lock);
    }
    depz_mutex_unlock(&h->lock);
}

static void mark_closed_locked(depz_hub *h, const char *lost_reason)
{
    depz_subscription *s;
    if (!h->closed && lost_reason) depz_strlcpy(h->lost_reason, lost_reason, sizeof h->lost_reason);
    h->closed = true;
    for (s = h->head; s; s = s->next) {
        depz_mutex_lock(&s->lock);
        s->closed = true;
        s->lost_reason = h->lost_reason[0] ? h->lost_reason : NULL;
        depz_cond_broadcast(&s->cond);
        depz_mutex_unlock(&s->lock);
    }
}

static void hub_mark_closed(depz_hub *h, const char *lost_reason)
{
    depz_mutex_lock(&h->lock);
    mark_closed_locked(h, lost_reason);
    depz_mutex_unlock(&h->lock);
}

/* Mark closed and drop the stream's reference; the hub lives on while
 * subscriptions do. */
static void hub_close(depz_hub *h)
{
    depz_mutex_lock(&h->lock);
    mark_closed_locked(h, NULL);
    hub_unref_locked_and_unlock(h);
}

/* `desc` names the report in the timeout message; NULL = a subscription's
 * own wait, where a negative timeout waits until an item or the close. */
static int sub_next(depz_subscription *s, void *item, int timeout_ms, const depz_stream_desc *desc)
{
    uint64_t deadline;
    int rc = DEPZ_OK;
    deadline = depz_now_us() + (uint64_t)(timeout_ms < 0 ? 0 : timeout_ms) * 1000u;
    depz_mutex_lock(&s->lock);
    while (!s->count) {
        int left;
        if (s->closed) {
            rc = s->lost_reason
                     ? depz_fail(DEPZ_E_LOST, "%s", s->lost_reason) /* the device's cause, as it is */
                     : depz_fail(DEPZ_E_CLOSED, "%s: device closed", desc ? desc->name : "subscription");
            break;
        }
        left = timeout_ms < 0 ? -1 : depz_ms_until(deadline);
        if (!left) {
            rc = desc ? depz_fail(DEPZ_E_TIMEOUT, "%s: no report 0x%02x in %d ms", desc->name,
                                  (unsigned)desc->report, timeout_ms)
                      : depz_fail(DEPZ_E_TIMEOUT, "subscription: no item in %d ms", timeout_ms);
            break;
        }
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

int depz_subscription_next(depz_subscription *s, void *item, int timeout_ms)
{
    if (!s || !item) return depz_fail(DEPZ_E_ARG, "subscription: NULL argument");
    return sub_next(s, item, timeout_ms, NULL);
}

uint64_t depz_subscription_dropped_count(const depz_subscription *s)
{
    uint64_t n;
    if (!s) return 0;
    depz_mutex_lock((depz_mutex *)&s->lock);
    n = s->dropped;
    depz_mutex_unlock((depz_mutex *)&s->lock);
    return n;
}

void depz_subscription_close(depz_subscription *s)
{
    depz_hub *h;
    if (!s) return;
    h = s->hub;
    depz_mutex_lock(&h->lock);
    {
        depz_subscription **pp = &h->head;
        while (*pp && *pp != s) pp = &(*pp)->next;
        if (*pp) *pp = s->next;
    }
    if (h->drop) {                          /* what it still holds goes with it */
        size_t i;
        for (i = 0; i < s->count; i++) h->drop(s->buf + ((s->head + i) % s->cap) * s->item_size);
    }
    hub_unref_locked_and_unlock(h);
    depz_cond_destroy(&s->cond);
    depz_mutex_destroy(&s->lock);
    free(s->buf);
    free(s);
}

/* The stream object: callbacks + subscriptions + counters of one kind of
 * item, owned by the device (contract 07 §3). */

#define STREAM_NEXT_DEFAULT_MS 2000

int depz_stream_init(depz_stream *s, depz_device *dev, const depz_stream_desc *desc)
{
    memset(s, 0, sizeof *s);
    s->dev = dev;
    s->desc = *desc;
    if (depz_cb_list_init(&s->cbs)) return -1;
    if (depz_mutex_init(&s->lock)) { depz_cb_list_free(&s->cbs); return -1; }
    s->hub = hub_new(desc);
    if (!s->hub) {
        depz_mutex_destroy(&s->lock);
        depz_cb_list_free(&s->cbs);
        return -1;
    }
    s->live = true;
    return 0;
}

void depz_stream_free(depz_stream *s)
{
    if (!s->live) return;
    hub_close(s->hub);
    depz_cb_list_free(&s->cbs);
    depz_mutex_destroy(&s->lock);
    s->live = false;
}

void depz_stream_end(depz_stream *s, const char *lost_reason)
{
    if (s->live) hub_mark_closed(s->hub, lost_reason);
}

void depz_stream_emit_as(depz_stream *s, const void *item, const void *queued)
{
    depz_cb_entry *cbs;
    size_t i, n = depz_cb_list_snapshot(&s->cbs, &cbs);
    for (i = 0; i < n; i++)
        if (!cbs[i].where || cbs[i].where(item, cbs[i].user))
            ((depz_stream_cb)cbs[i].fn)(item, cbs[i].user);
    depz_cb_list_release(&s->cbs, cbs);
    hub_push(s->hub, queued ? queued : item);
}

void depz_stream_emit(depz_stream *s, const void *item) { depz_stream_emit_as(s, item, NULL); }

void depz_stream_count_parse_error(depz_stream *s)
{
    depz_mutex_lock(&s->lock);
    s->parse_errors++;
    depz_mutex_unlock(&s->lock);
}

void depz_stream_reset_parse_errors(depz_stream *s)
{
    depz_mutex_lock(&s->lock);
    s->parse_errors = 0;
    depz_mutex_unlock(&s->lock);
}

/* ── the public side ── */

/* The stream is device memory: nothing of it is read before the device is
 * entered (close() on another thread frees it otherwise). */
int depz_stream_on(depz_stream *s, depz_stream_cb cb, void *user, depz_stream_where where)
{
    int rc, token = 0;
    if (!s) return depz_fail(DEPZ_E_ARG, "stream: NULL stream");
    rc = depz_device_enter(s->dev);
    if (rc) return rc;
    if (!cb) rc = depz_fail(DEPZ_E_ARG, "%s: NULL callback", s->desc.name);
    else rc = depz_cb_list_add(&s->cbs, (void (*)(void))cb, user, where, &token);
    depz_device_leave(s->dev);
    return rc ? rc : token;
}

void depz_stream_off(depz_stream *s, int token)
{
    if (!s || token <= 0 || depz_device_enter(s->dev)) return;
    depz_cb_list_remove(&s->cbs, token);
    depz_device_leave(s->dev);
}

depz_subscription *depz_stream_subscribe(depz_stream *s, size_t maxsize)
{
    depz_subscription *sub;
    if (!s) {
        depz_fail(DEPZ_E_ARG, "stream: NULL stream");
        return NULL;
    }
    if (depz_device_enter(s->dev)) return NULL;
    sub = hub_subscribe(s->hub, maxsize ? maxsize : s->desc.default_maxsize);
    if (!sub) depz_fail(DEPZ_E_NOMEM, "%s: cannot subscribe", s->desc.name);
    depz_device_leave(s->dev);
    return sub;
}

int depz_stream_next(depz_stream *s, void *item, size_t item_size, int timeout_ms)
{
    depz_subscription *sub;
    int rc;
    if (!s) return depz_fail(DEPZ_E_ARG, "stream: NULL stream");
    rc = depz_device_enter(s->dev);
    if (rc) return rc;
    if (!item) rc = depz_fail(DEPZ_E_ARG, "%s: NULL item", s->desc.name);
    else if (s->desc.report < 0)
        rc = depz_fail(DEPZ_E_ARG,
                       "%s offers no next(): no single report to wait for (contract 07 §3) — "
                       "subscribe with depz_stream_on() or depz_stream_subscribe()",
                       s->desc.name);
    else if (item_size != s->desc.item_size)
        rc = depz_fail(DEPZ_E_ARG, "%s: an item of %zu bytes, the stream's is %zu", s->desc.name,
                       item_size, s->desc.item_size);
    if (rc) {
        depz_device_leave(s->dev);
        return rc;
    }
    sub = hub_subscribe(s->hub, 1);
    if (!sub) {
        rc = depz_fail(DEPZ_E_NOMEM, "%s: cannot subscribe", s->desc.name);
    } else {
        rc = sub_next(sub, item, timeout_ms < 0 ? STREAM_NEXT_DEFAULT_MS : timeout_ms, &s->desc);
        depz_subscription_close(sub);
    }
    depz_device_leave(s->dev);
    return rc;
}

size_t depz_stream_dropped_counts(const depz_stream *s, uint64_t *out, size_t cap)
{
    const depz_subscription *sub;
    size_t n = 0;
    if (!s || !depz_device_enter_const(s->dev)) return 0;
    depz_mutex_lock(&s->hub->lock);
    for (sub = s->hub->head; sub; sub = sub->next, n++)
        if (n < cap && out) out[n] = depz_subscription_dropped_count(sub);
    depz_mutex_unlock(&s->hub->lock);
    depz_device_leave(s->dev);
    return n;
}

uint64_t depz_stream_parse_errors(const depz_stream *s)
{
    uint64_t n;
    if (!s || !depz_device_enter_const(s->dev)) return 0;
    depz_mutex_lock((depz_mutex *)&s->lock);
    n = s->parse_errors;
    depz_mutex_unlock((depz_mutex *)&s->lock);
    depz_device_leave(s->dev);
    return n;
}

/* ── callback lists ────────────────────────────────────────────────────── */

int depz_cb_list_init(depz_cb_list *l)
{
    memset(l, 0, sizeof *l);
    l->next_token = 1;
    if (depz_mutex_init(&l->lock)) return -1;
    if (depz_cond_init(&l->released)) { depz_mutex_destroy(&l->lock); return -1; }
    return 0;
}

void depz_cb_list_free(depz_cb_list *l)
{
    free(l->items);
    depz_cond_destroy(&l->released);
    depz_mutex_destroy(&l->lock);
}

int depz_cb_list_add(depz_cb_list *l, void (*fn)(void), void *user, depz_stream_where where,
                     int *token)
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
        e->where = where;
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
    /* a snapshot taken before the removal may still call the entry: wait it
     * out — unless this is the dispatching thread itself (a callback
     * unsubscribing), which cannot wait for its own return */
    while (l->dispatching && !depz_thread_id_is_current(l->dispatcher))
        depz_cond_wait_ms(&l->released, &l->lock, -1);
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
        if (*out) {
            memcpy(*out, l->items, n * sizeof **out);
            l->dispatching++;
            l->dispatcher = depz_thread_self();
        } else {
            n = 0;
        }
    }
    depz_mutex_unlock(&l->lock);
    return n;
}

void depz_cb_list_release(depz_cb_list *l, depz_cb_entry *out)
{
    if (!out) return;
    depz_mutex_lock(&l->lock);
    l->dispatching--;
    depz_cond_broadcast(&l->released);
    depz_mutex_unlock(&l->lock);
    free(out);
}
