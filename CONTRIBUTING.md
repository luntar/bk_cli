# Contributing

This project is intentionally small and C++17-only.

## Local Checks

```powershell
cmake -S . -B build
cmake --build build
```

Keep changes focused:

- Prefer the C++17 standard library.
- Keep command handlers small and explicit.
- Avoid adding dependencies unless they remove real complexity.
- Update `README.md` when the public API changes.

## Style

- Use `bk` as the project namespace.
- Use clear command names that are short enough to type.
- Prefer typed arguments with `bk::argument_type`.
- Return user-facing command output through `bk::cmd_return_t`.
