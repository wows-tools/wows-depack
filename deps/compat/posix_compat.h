/*
 * POSIX compatibility shims for Windows (MinGW-w64 / UCRT64).
 * The endian section covers all platforms; all other sections are Windows-only.
 * Include this header instead of the scattered platform endian guards.
 */
#ifndef POSIX_COMPAT_H
#define POSIX_COMPAT_H

/* ---- Endian support (all platforms) ---- */
#if defined(__linux__)
#include <endian.h>
#elif defined(_WIN32)
#include <stdlib.h> /* _byteswap_* intrinsics */
/* Windows is always little-endian */
#define htole16(x) (x)
#define le16toh(x) (x)
#define htole32(x) (x)
#define le32toh(x) (x)
#define htole64(x) (x)
#define le64toh(x) (x)
/* Big-endian byte-swap via Windows intrinsics */
#define be16toh(x) _byteswap_ushort(x)
#define htobe16(x) _byteswap_ushort(x)
#define be32toh(x) _byteswap_ulong(x)
#define htobe32(x) _byteswap_ulong(x)
#define be64toh(x) _byteswap_uint64(x)
#define htobe64(x) _byteswap_uint64(x)
#else
#include <sys/endian.h>
#endif

/* ---- Windows-only POSIX shims ---- */
#ifdef _WIN32

#include <sys/stat.h>
#include <direct.h>
#include <stdio.h>

/* lstat: Windows has no symlinks; stat and lstat are equivalent here */
#define lstat(path, buf) stat(path, buf)

/* realpath */
#ifndef PATH_MAX
#define PATH_MAX _MAX_PATH
#endif
char *realpath(const char *path, char *resolved);

/* mkdir: POSIX takes (path, mode); Windows _mkdir takes only (path) */
#ifdef mkdir
#undef mkdir
#endif
#define mkdir(path, mode) _mkdir(path)

/* fmemopen: in the UCRT runtime but its declaration is suppressed by
 * __STRICT_ANSI__ when compiling with -std=c11.  Re-declare it explicitly. */
#include <stddef.h>
FILE *fmemopen(void *buf, size_t size, const char *mode);

#endif /* _WIN32 */
#endif /* POSIX_COMPAT_H */
