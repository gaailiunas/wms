# WMS
A minimal, peer-to-peer warehouse management system that works offline over a local network. Each device maintains its own local copy of the inventory database and synchronizes with other peers over LAN.


## Building
```bash
mkdir build && cd build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```
### Cross-compilation
```bash
mkdir build && cd build
cmake -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc -DCMAKE_EXE_LINKER_FLAGS="-static" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
```

