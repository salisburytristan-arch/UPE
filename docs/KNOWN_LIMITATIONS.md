# Known Limitations

## Scope Limitations

- Prebuilt libraries may be absent; source build is the primary recommended path.
- Custom allocators outside the provided engine allocators are not certified for determinism.

## Determinism Scope

- Determinism is guaranteed across platforms when using identical compiler flags
- Required flags: `/fp:precise` (MSVC), `-ffp-contract=off` (GCC/Clang)
- Multithreading requires deterministic parallel mode enabled
- System allocator usage breaks determinism guarantees; use UpeAllocator

## Features

- No first-class GPU compute integration (CPU-only)
- Multibody constraint hierarchies (e.g., gears, pulleys) use iterative solve, not analytical

## Support Model

- This is a standalone SDK purchase; no support contracts or maintenance included
- Buyer is responsible for integration and operational support
