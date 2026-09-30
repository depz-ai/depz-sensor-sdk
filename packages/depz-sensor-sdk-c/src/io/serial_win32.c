/* serial_win32.c — CDC-ACM serial port on Windows.
 *
 * Overlapped I/O: synchronous I/O on one handle is serialized by the OS, so a
 * pending ReadFile on the reader thread would hold every request's WriteFile
 * back until the read timed out. */
#ifdef _WIN32

#include "io_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    HANDLE h;
    HANDLE stop;       /* manual-reset: set by close() */
    HANDLE rd_event, wr_event;
    char port[256];
} win_port;

static int win_err(int err, const char *port, const char *what)
{
    char msg[256];
    DWORD e = GetLastError();
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, e, 0, msg,
                   sizeof msg, NULL);
    msg[strcspn(msg, "\r\n")] = '\0';
    return depz_fail(err, "%s: %s: %s (%lu)", port, what, msg, (unsigned long)e);
}

static int wp_read(void *self, uint8_t *buf, size_t cap, int timeout_ms, size_t *got)
{
    win_port *p = (win_port *)self;
    OVERLAPPED ov;
    DWORD n = 0, w;
    HANDLE waits[2];

    *got = 0;
    memset(&ov, 0, sizeof ov);
    ov.hEvent = p->rd_event;
    ResetEvent(p->rd_event);
    if (!ReadFile(p->h, buf, (DWORD)cap, &n, &ov)) {
        if (GetLastError() != ERROR_IO_PENDING) return win_err(DEPZ_E_CLOSED, p->port, "read");
        waits[0] = p->rd_event;
        waits[1] = p->stop;
        w = WaitForMultipleObjects(2, waits, FALSE, timeout_ms < 0 ? 0 : (DWORD)timeout_ms);
        if (w != WAIT_OBJECT_0) {
            CancelIoEx(p->h, &ov);
            GetOverlappedResult(p->h, &ov, &n, TRUE); /* wait for the cancel to land */
            if (w == WAIT_OBJECT_0 + 1) return depz_fail(DEPZ_E_CLOSED, "%s: closed", p->port);
            *got = n; /* bytes that arrived during the cancel still count */
            return DEPZ_OK;
        }
        if (!GetOverlappedResult(p->h, &ov, &n, FALSE))
            return win_err(DEPZ_E_CLOSED, p->port, "read");
    }
    *got = n;
    return DEPZ_OK;
}

static int wp_write(void *self, const uint8_t *data, size_t len)
{
    win_port *p = (win_port *)self;
    while (len) {
        OVERLAPPED ov;
        DWORD n = 0;
        memset(&ov, 0, sizeof ov);
        ov.hEvent = p->wr_event;
        ResetEvent(p->wr_event);
        if (!WriteFile(p->h, data, (DWORD)len, &n, &ov)) {
            if (GetLastError() != ERROR_IO_PENDING) return win_err(DEPZ_E_CLOSED, p->port, "write");
            if (WaitForSingleObject(p->wr_event, 2000) != WAIT_OBJECT_0) {
                CancelIoEx(p->h, &ov);
                GetOverlappedResult(p->h, &ov, &n, TRUE);
                return depz_fail(DEPZ_E_TIMEOUT, "%s: write timed out", p->port);
            }
            if (!GetOverlappedResult(p->h, &ov, &n, FALSE))
                return win_err(DEPZ_E_CLOSED, p->port, "write");
        }
        data += n;
        len -= n;
    }
    return DEPZ_OK;
}

static void wp_close(void *self)
{
    SetEvent(((win_port *)self)->stop);
}

static void wp_destroy(void *self)
{
    win_port *p = (win_port *)self;
    if (p->h != INVALID_HANDLE_VALUE) CloseHandle(p->h);
    if (p->stop) CloseHandle(p->stop);
    if (p->rd_event) CloseHandle(p->rd_event);
    if (p->wr_event) CloseHandle(p->wr_event);
    free(p);
}

static const depz_link_vtable wp_vt = {wp_read, wp_write, wp_close, wp_destroy};

int depz_serial_link_open(const char *port, depz_link **out)
{
    win_port *p;
    char path[300];
    DCB dcb;
    COMMTIMEOUTS to;
    int rc;

    p = (win_port *)calloc(1, sizeof *p);
    if (!p) return depz_fail(DEPZ_E_NOMEM, "serial: out of memory");
    p->h = INVALID_HANDLE_VALUE;
    depz_strlcpy(p->port, port, sizeof p->port);
    p->stop = CreateEventA(NULL, TRUE, FALSE, NULL);
    p->rd_event = CreateEventA(NULL, TRUE, FALSE, NULL);
    p->wr_event = CreateEventA(NULL, TRUE, FALSE, NULL);
    if (!p->stop || !p->rd_event || !p->wr_event) {
        wp_destroy(p);
        return depz_fail(DEPZ_E_NOMEM, "serial: CreateEvent failed");
    }
    /* "\\.\COM10": the plain name only works up to COM9. */
    if (strncmp(port, "\\\\.\\", 4) == 0) depz_strlcpy(path, port, sizeof path);
    else snprintf(path, sizeof path, "\\\\.\\%s", port);
    /* Share mode 0: Windows opens COM ports exclusively (contract 07). */
    p->h = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                       FILE_FLAG_OVERLAPPED, NULL);
    if (p->h == INVALID_HANDLE_VALUE) {
        rc = win_err(DEPZ_E_IO, port, "cannot open");
        wp_destroy(p);
        return rc;
    }
    memset(&dcb, 0, sizeof dcb);
    dcb.DCBlength = sizeof dcb;
    if (GetCommState(p->h, &dcb)) {
        dcb.BaudRate = CBR_115200; /* nominal: USB CDC ignores the rate */
        dcb.ByteSize = 8;
        dcb.Parity = NOPARITY;
        dcb.StopBits = ONESTOPBIT;
        dcb.fBinary = TRUE;
        dcb.fOutxCtsFlow = FALSE;
        dcb.fOutxDsrFlow = FALSE;
        dcb.fDtrControl = DTR_CONTROL_ENABLE;
        dcb.fRtsControl = RTS_CONTROL_ENABLE;
        dcb.fOutX = FALSE;
        dcb.fInX = FALSE;
        dcb.fNull = FALSE;
        dcb.fAbortOnError = FALSE;
        SetCommState(p->h, &dcb);
    }
    /* A read returns as soon as any byte is there; the wait itself is ours. */
    memset(&to, 0, sizeof to);
    to.ReadIntervalTimeout = MAXDWORD;
    to.ReadTotalTimeoutMultiplier = MAXDWORD;
    to.ReadTotalTimeoutConstant = 0x7FFFFFFE;
    SetCommTimeouts(p->h, &to);
    PurgeComm(p->h, PURGE_RXCLEAR | PURGE_TXCLEAR);

    rc = depz_link_new(&wp_vt, p, port, out);
    if (rc) wp_destroy(p);
    return rc;
}

#endif /* _WIN32 */
