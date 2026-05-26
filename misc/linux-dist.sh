#!/bin/bash
set -e

PROJECT="wows-depack"
ARCH="$(uname -m)"
DIST_DIR="dist/${PROJECT}"
TARBALL="${PROJECT}-linux-${ARCH}.tar.gz"

BINARIES=(
    wows-depack-cli
)

LIBRARIES=(
    libwows-depack.so
)

EXTRA_FILES=(
    LICENSE
    README.md
)

mkdir -p "$DIST_DIR/lib"

for bin in "${BINARIES[@]}"; do
    cp "$bin" "$DIST_DIR/"
done

for lib in "${LIBRARIES[@]}"; do
    cp -P "${lib}"* "$DIST_DIR/lib/" 2>/dev/null || true
done

cp "${EXTRA_FILES[@]}" "$DIST_DIR/"

# Recursively copy shared library dependencies into lib/, skipping
# virtual/unresolved entries (e.g. linux-vdso) and already-present files.
copy_deps() {
    ldd "$1" | awk '/=> \// { print $3 }' | while read -r dep; do
        dest="$DIST_DIR/lib/$(basename "$dep")"
        if [[ ! -e "$dest" ]]; then
            cp "$dep" "$dest"
            copy_deps "$dep"
        fi
    done
}

for lib in "${LIBRARIES[@]}"; do
    copy_deps "$lib"
done

for bin in "${BINARIES[@]}"; do
    copy_deps "$bin"
done

# Patch rpath so binaries find bundled libs, and libraries find their own deps.
# Use --force-rpath to write DT_RPATH instead of DT_RUNPATH: DT_RPATH is
# inherited by the dynamic linker when resolving transitive dependencies,
# whereas DT_RUNPATH is not, causing ldd to fall back to the system cache.
for bin in "${BINARIES[@]}"; do
    patchelf --force-rpath --set-rpath '$ORIGIN/lib' "$DIST_DIR/$bin"
done

# Patch all non-symlink .so files in lib/, not just the ones matching LIBRARIES,
# so that any bundled system dep can also find its own transitive deps.
find "$DIST_DIR/lib" ! -type l -name "*.so*" \
    -exec patchelf --force-rpath --set-rpath '$ORIGIN' {} \;

tar -czf "$TARBALL" -C dist .
