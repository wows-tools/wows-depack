# WoWs depack

World of Warships resource files unpacker

# Usage

# Compilation

# Dependencies

# Fuzzing

This tool/lib parses binary data, consequently checking for overflow might be a good idea.

To do so, install [american fuzzy lop++ (afl++)](https://aflplus.plus/):

```shel
apt install afl-clang
```

Compile with afl support:
```shell
cmake -DCMAKE_CXX_COMPILER=afl-clang++ -DCMAKE_C_COMPILER=afl-clang .
```

Run the fuzzing
```shell
# Replace with your WoWs install directory
INDEX_DIR=" ~/Games/World\ of\ Warships/bin/6775398/idx/"

afl-fuzz -i "$INDEX_DIR" -o ./out -t 10000 -- ./wows-depack-cli -i '@@'
```

Then wait for crashes to occure (hopefully this will be in vain).
