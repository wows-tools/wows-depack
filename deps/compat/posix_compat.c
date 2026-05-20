#ifdef _WIN32

#include <stdlib.h>
#include "posix_compat.h"

char *realpath(const char *path, char *resolved) {
    return _fullpath(resolved, path, _MAX_PATH);
}

#endif /* _WIN32 */
