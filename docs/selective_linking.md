# Selective Linking Guide

FrameKit supports optional linkage. You only link modules you explicitly enable.

## 1) FrameKit core only (no NetKit)
```bash
cmake -S . -B build \
  -DFRAMEKIT_BUILD_TESTS=ON \
  -DFRAMEKIT_BUILD_EXAMPLES=ON \
  -DFRAMEKIT_ENABLE_NETKIT=OFF
cmake --build build
```

## 2) FrameKit + NetKit integration
```bash
cmake -S . -B build \
  -DFRAMEKIT_BUILD_TESTS=ON \
  -DFRAMEKIT_BUILD_EXAMPLES=ON \
  -DFRAMEKIT_ENABLE_NETKIT=ON \
  -DFRAMEKIT_NETKIT_SOURCE_DIR=../netkit
cmake --build build
```

## 3) MediaKit stays optional
MediaKit is a separate repository and is not required by FrameKit builds.
Only add MediaKit when your application uses media session contracts/plugins.

## Verification
Check linked libraries of your app:
- macOS: `otool -L <binary>`
- Linux: `ldd <binary>`
- Windows: `dumpbin /dependents <binary>`

If NetKit is disabled, your FrameKit binaries should not link NetKit artifacts.
