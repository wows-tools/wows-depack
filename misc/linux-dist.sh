#!/bin/sh

mkdir -p dist/wows-depack/lib
cp wows-depack-cli dist/wows-depack/wows-depack-cli
cp -P libwows-depack.so* dist/wows-depack/lib 2>/dev/null || true
cp LICENSE README.md dist/wows-depack/
cp `ldd libwows-depack.so  | grep '=>' | sed 's/.*=> //' | sed 's/ .*//'` dist/wows-depack/lib/
cp `ldd wows-depack-cli  | grep '=>' | sed 's/.*=> //' | sed 's/ .*//'` dist/wows-depack/lib/
patchelf --set-rpath '$ORIGIN/lib:$ORIGIN/lib' dist/wows-depack/wows-depack-cli
tar -czf wows-depack-linux-x86_64.tar.gz -C dist .
