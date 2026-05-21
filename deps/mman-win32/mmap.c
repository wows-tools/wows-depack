#include <windows.h>
#include <io.h>
#include <errno.h>
#include "sys/mman.h"

void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset) {
    (void)addr;
    (void)flags;

    HANDLE hFile = (HANDLE)_get_osfhandle(fd);
    if (hFile == INVALID_HANDLE_VALUE) {
        errno = EBADF;
        return MAP_FAILED;
    }

    DWORD protect = PAGE_READONLY;
    DWORD access = FILE_MAP_READ;
    if (prot & PROT_WRITE) {
        protect = PAGE_READWRITE;
        access = FILE_MAP_WRITE;
    }
    if (prot & PROT_EXEC) {
        protect = (prot & PROT_WRITE) ? PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ;
        access = FILE_MAP_EXECUTE;
    }

    HANDLE hMap = CreateFileMappingA(hFile, NULL, protect, 0, 0, NULL);
    if (!hMap) {
        errno = ENOMEM;
        return MAP_FAILED;
    }

    void *ptr =
        MapViewOfFile(hMap, access, (DWORD)((offset >> 32) & 0xFFFFFFFF), (DWORD)(offset & 0xFFFFFFFF), (SIZE_T)len);
    CloseHandle(hMap);

    if (!ptr) {
        errno = ENOMEM;
        return MAP_FAILED;
    }
    return ptr;
}

int munmap(void *addr, size_t len) {
    (void)len;
    return UnmapViewOfFile(addr) ? 0 : -1;
}
