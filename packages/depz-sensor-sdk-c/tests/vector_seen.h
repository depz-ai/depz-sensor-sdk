/* vector_seen.h — report a golden-vector case ID as asserted, for the coverage
 * gate (scripts/check_vector_coverage.py). Test-runner support only.
 *
 * Call right AFTER the case's assertions passed. No-op unless
 * $DEPZ_VECTOR_SEEN names the output file. ctest runs one process per test,
 * so the line goes out as ONE append-mode write: O_APPEND on POSIX,
 * FILE_APPEND_DATA on Windows — both atomic, lines never interleave.
 */
#ifndef DEPZ_TEST_VECTOR_SEEN_H
#define DEPZ_TEST_VECTOR_SEEN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

static inline void vector_seen(const char *id)
{
    const char *path = getenv("DEPZ_VECTOR_SEEN");
    char line[512];
    int n;
    if (!path || !path[0] || !id) return;
    n = snprintf(line, sizeof line, "%s\n", id);
    if (n <= 0 || (size_t)n >= sizeof line) return;
#ifdef _WIN32
    {
        HANDLE h = CreateFileA(path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                               OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        DWORD wrote;
        if (h == INVALID_HANDLE_VALUE) return;
        WriteFile(h, line, (DWORD)n, &wrote, NULL);
        CloseHandle(h);
    }
#else
    {
        int fd = open(path, O_WRONLY | O_APPEND | O_CREAT, 0644);
        if (fd < 0) return;
        if (write(fd, line, (size_t)n) != n) { /* best effort: the gate reports the gap */ }
        close(fd);
    }
#endif
}

#endif
