/* serial_posix.c — CDC-ACM serial port on Linux and macOS. */
#ifndef _WIN32

#if defined(__linux__) && !defined(_DEFAULT_SOURCE)
#  define _DEFAULT_SOURCE /* cfmakeraw, flock */
#endif
#if defined(__APPLE__) && !defined(_DARWIN_C_SOURCE)
#  define _DARWIN_C_SOURCE
#endif

#include "io_internal.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <termios.h>
#include <unistd.h>

typedef struct {
    int fd;
    int wake[2];  /* self-pipe: close() wakes a blocked poll() */
    char port[256];
} posix_port;

static int sp_read(void *self, uint8_t *buf, size_t cap, int timeout_ms, size_t *got)
{
    posix_port *p = (posix_port *)self;
    struct pollfd fds[2];
    ssize_t n;
    int rc;

    *got = 0;
    fds[0].fd = p->fd;
    fds[0].events = POLLIN;
    fds[1].fd = p->wake[0];
    fds[1].events = POLLIN;
    do {
        rc = poll(fds, 2, timeout_ms < 0 ? 0 : timeout_ms);
    } while (rc < 0 && errno == EINTR);
    if (rc < 0) return depz_fail(DEPZ_E_CLOSED, "%s: poll: %s", p->port, strerror(errno));
    if (fds[1].revents) return depz_fail(DEPZ_E_CLOSED, "%s: closed", p->port);
    if (!rc) return DEPZ_OK;
    if (fds[0].revents & (POLLERR | POLLNVAL))
        return depz_fail(DEPZ_E_CLOSED, "%s: device lost", p->port);
    n = read(p->fd, buf, cap);
    if (n > 0) { *got = (size_t)n; return DEPZ_OK; }
    if (n < 0 && (errno == EAGAIN || errno == EINTR)) return DEPZ_OK;
    /* 0 = hang-up (unplugged), <0 = EIO and friends. */
    return depz_fail(DEPZ_E_CLOSED, "%s: device lost%s%s", p->port, n < 0 ? ": " : "",
                     n < 0 ? strerror(errno) : "");
}

static int sp_write(void *self, const uint8_t *data, size_t len)
{
    posix_port *p = (posix_port *)self;
    uint64_t deadline = depz_now_us() + 2000000u; /* 2 s, like the Python SDK */
    while (len) {
        ssize_t n = write(p->fd, data, len);
        if (n > 0) { data += n; len -= (size_t)n; continue; }
        if (n < 0 && errno == EINTR) continue;
        if (n < 0 && errno == EAGAIN) {
            struct pollfd fd;
            int left = depz_ms_until(deadline);
            if (!left) return depz_fail(DEPZ_E_TIMEOUT, "%s: write timed out", p->port);
            fd.fd = p->fd;
            fd.events = POLLOUT;
            poll(&fd, 1, left);
            continue;
        }
        return depz_fail(DEPZ_E_CLOSED, "%s: write: %s", p->port, strerror(errno));
    }
    return DEPZ_OK;
}

static void sp_close(void *self)
{
    posix_port *p = (posix_port *)self;
    char b = 1;
    ssize_t ignored = write(p->wake[1], &b, 1);
    (void)ignored;
}

/* The kernel keeps termios per tty across close(); the raw mode we set would
 * otherwise poison the next opener (Chromium's Web Serial starts from the
 * existing state and then cannot talk to the device). Leave it as `stty sane`
 * would, with IXON off so a 0x13 byte never stalls a binary frame. */
static void restore_sane(int fd)
{
    struct termios t;
    if (tcgetattr(fd, &t)) return;
    t.c_iflag = (t.c_iflag | BRKINT | ICRNL | IMAXBEL | PARMRK)
              & ~(tcflag_t)(IGNBRK | INLCR | IGNCR | IXOFF | IXON);
    t.c_oflag |= OPOST | ONLCR;
    t.c_lflag |= ISIG | ICANON | IEXTEN | ECHO | ECHOE | ECHOK | ECHOCTL | ECHOKE;
    t.c_cc[VMIN] = 1;
    t.c_cc[VTIME] = 0;
    tcsetattr(fd, TCSANOW, &t);
}

static void sp_destroy(void *self)
{
    posix_port *p = (posix_port *)self;
    if (p->fd >= 0) {
        restore_sane(p->fd);
        flock(p->fd, LOCK_UN);
        close(p->fd);
    }
    close(p->wake[0]);
    close(p->wake[1]);
    free(p);
}

static const depz_link_vtable sp_vt = {sp_read, sp_write, sp_close, sp_destroy};

int depz_serial_link_open(const char *port, depz_link **out)
{
    posix_port *p;
    struct termios t;
    int rc;

    p = (posix_port *)calloc(1, sizeof *p);
    if (!p) return depz_fail(DEPZ_E_NOMEM, "serial: out of memory");
    depz_strlcpy(p->port, port, sizeof p->port);
    if (pipe(p->wake)) {
        free(p);
        return depz_fail(DEPZ_E_IO, "serial: pipe: %s", strerror(errno));
    }
    fcntl(p->wake[0], F_SETFD, FD_CLOEXEC);
    fcntl(p->wake[1], F_SETFD, FD_CLOEXEC);
    fcntl(p->wake[1], F_SETFL, O_NONBLOCK);

    p->fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (p->fd < 0) {
        int e = errno;
        p->fd = -1;
        sp_destroy(p);
        return depz_fail(DEPZ_E_IO, "cannot open %s: %s", port, strerror(e));
    }
    /* A CDC-ACM tty is not single-opener: two readers would each get a random
     * half of the bytes. Refuse loudly instead (contract 07: one owner). */
    if (flock(p->fd, LOCK_EX | LOCK_NB)) {
        close(p->fd);
        p->fd = -1;
        sp_destroy(p);
        return depz_fail(DEPZ_E_IO, "cannot open %s: in use by another program", port);
    }
    if (tcgetattr(p->fd, &t) == 0) {
        cfmakeraw(&t);
        t.c_cflag |= CLOCAL | CREAD;
        t.c_cc[VMIN] = 0;
        t.c_cc[VTIME] = 0;
        cfsetispeed(&t, B115200); /* nominal: USB CDC ignores the rate */
        cfsetospeed(&t, B115200);
        tcsetattr(p->fd, TCSANOW, &t);
    }
    tcflush(p->fd, TCIFLUSH);

    rc = depz_link_new(&sp_vt, p, port, out);
    if (rc) sp_destroy(p);
    return rc;
}

#endif /* !_WIN32 */
