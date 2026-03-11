# Bindings Status

The core product is the C API. Bindings are integration conveniences.

## Current Binding Presence
- Python: present in `bindings/python/`
- C#: present in `bindings/csharp/`
- Rust: present in `bindings/rust/`
- JavaScript/WASM: present in `bindings/javascript/`
- Lua: present in `bindings/lua/`

## Product Positioning
- Buyer-critical path: C API + deterministic core verification.
- Bindings are secondary and can be validated after core acceptance.

## Recommended Evaluation Order
1. Validate C API build/test/proof flow.
2. Validate one binding relevant to your integration stack.
3. Treat remaining bindings as optional follow-up.
