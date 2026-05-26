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

# Copy shared library dependencies, skipping virtual/unresolved entries (e.g. linux-vdso)
copy_deps() {
    ldd "$1" | awk '/=> \// { print $3 }' | while read -r dep; do
        cp -n "$dep" "$DIST_DIR/lib/"
    done
}

for lib in "${LIBRARIES[@]}"; do
    copy_deps "$lib"
done

for bin in "${BINARIES[@]}"; do
    copy_deps "$bin"
done

# Patch rpath so binaries find bundled libs, and libraries find their own deps
for bin in "${BINARIES[@]}"; do
    patchelf --set-rpath '$ORIGIN/lib' "$DIST_DIR/$bin"
done

for lib in "${LIBRARIES[@]}"; do
    find "$DIST_DIR/lib" -name "${lib}*" ! -type l \
        -exec patchelf --set-rpath '$ORIGIN' {} \;
done

tar -czf "$TARBALL" -C dist .
