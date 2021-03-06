# Compile **xmr-stak** for macOS

## Dependencies

Assuming you already have [Homebrew](https://brew.sh) installed, the installation of dependencies is pretty straightforward and will generate the `xmr-stak` binary in the `bin/` directory.

### For AMD GPUs

OpenCL is bundled with Xcode, so no other depedency then the basic ones needed. Just enable OpenCL via the `-DOpenCL_ENABLE=ON` CMake option.

```shell
brew install gcc openssl cmake
cmake . -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DOpenCL_ENABLE=ON
make install
```

### For CPU-only mining

```shell
brew install gcc openssl cmake
cmake . -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DOpenCL_ENABLE=OFF
make install
```

[All available CMake options](compile.md#cpu-build-options)
