# L4 Linux SDK

This SDK keeps only the files needed to build these Linux targets:

- `l4_daemon`
- `l4_linux_mvi`
- `libar8030_client.so`

## Directory Layout

Build and install outputs are separated by target architecture:

```text
L4_Linux_SDK/
  build/
    x86_64/
    arm64/
  install/
    x86_64/
    arm64/
```

Generated `build/` and `install/` outputs should not be treated as source files.

## X86_64 Local Build

```bash
cd L4_Linux_SDK
./script/cmk-local.sh
```

The X86_64 build cache is written to `L4_Linux_SDK/build/x86_64`.

The X86_64 install output is written to `L4_Linux_SDK/install/x86_64`.

## ARM64 Cross Build

```bash
cd L4_Linux_SDK
./script/cmk-arm.sh
```

The ARM64 build cache is written to `L4_Linux_SDK/build/arm64`.

The ARM64 install output is written to `L4_Linux_SDK/install/arm64`.

The ARM64 build uses `L4_Linux_SDK/compiler.arm.cmake`, which expects the
`aarch64-none-linux-gnu` toolchain under:

```text
L4_Linux_SDK/toolchain/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu
```

The compiler prefix is:

```text
bin/aarch64-none-linux-gnu-
```

If the toolchain is installed in another directory, update `GCC_PATH` in
`compiler.arm.cmake` before running `./script/cmk-arm.sh`.

## Clean

Clean all SDK build and install outputs:

```bash
cd L4_Linux_SDK
./script/clean.sh
```

or:

```bash
./script/clean.sh all
```

Clean only X86_64 outputs:

```bash
./script/clean.sh x86_64
```

`x86` is also accepted as an alias for `x86_64`.

Clean only ARM64 outputs:

```bash
./script/clean.sh arm64
```

`arm` is also accepted as an alias for `arm64`.

## Manual Build

X86_64:

```bash
cmake -S L4_Linux_SDK -B L4_Linux_SDK/build/x86_64 \
  -DCMAKE_INSTALL_PREFIX=L4_Linux_SDK/install/x86_64 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DAPP_STATIC_LIB=OFF \
  -DUSING_8030USB=ON \
  -DUSING_8030UART=ON \
  -DUSING_8030SDIO=OFF \
  -DUSING_XDS_HDR=ON

cmake --build L4_Linux_SDK/build/x86_64 --target ar8030_client l4_linux_mvi l4_daemon -j
cmake --install L4_Linux_SDK/build/x86_64
```

ARM64:

```bash
cmake -S L4_Linux_SDK -B L4_Linux_SDK/build/arm64 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_INSTALL_PREFIX=L4_Linux_SDK/install/arm64 \
  -DCMAKE_TOOLCHAIN_FILE=L4_Linux_SDK/compiler.arm.cmake \
  -DAPP_STATIC_LIB=OFF \
  -DUSING_8030USB=ON \
  -DUSING_8030UART=ON \
  -DUSING_8030SDIO=OFF \
  -DUSING_XDS_HDR=ON

cmake --build L4_Linux_SDK/build/arm64 --target ar8030_client l4_linux_mvi l4_daemon -j
cmake --install L4_Linux_SDK/build/arm64
```

## Notes

- USB backend is enabled by default and uses `third_package/libusb/libusb-cmake.tar`.
- UART backend is enabled by default.
- SDIO is disabled by default.
- DRV, Python, Java, demo apps, helper tools, firmware files, and generated build/install outputs are intentionally excluded.
