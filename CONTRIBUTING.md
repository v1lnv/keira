# Contributing to Keira Kernel

Thank you for showing interest in contributing to the Keira Kernel project! As a freestanding kernel, keeping the codebase clean, stable, and documented is crucial.

---

## Branching Model

Our Git repository follows a simplified branching model:
- **`master` / `main`**: Represents the stable production releases. Merges into this branch must compile cleanly and pass CI builds.
- **`develop`**: The primary integration branch for new features, bug fixes, and modifications. Pull Requests should target this branch first.

---

## Code Style and Formatting

To maintain code readability across our C, Assembly, and Rust subsystems:

### Rust Formatting
Format all Rust core files using `rustfmt` before committing changes:
```bash
cargo fmt --all
```

### C Formatting
Format C source and header files using `clang-format` (our configuration is stored in `.clang-format`):
```bash
clang-format -i drivers/**/*.c include/**/*.h
```

---

## Submission Process

1. **Check for Issues**: Look at our existing issues or open a new one using our templates to discuss your changes first.
2. **Fork and Branch**: Create a feature branch off of the `develop` branch (e.g. `feature/my-new-command`).
3. **Write and Test**:
   - Verify that your changes compile without any errors or warnings (`make clean && make`).
   - Test your changes locally in QEMU (`make run`).
4. **Submit a Pull Request**: Fill out the [Pull Request Template](.github/pull_request_template.md) completely, referencing any related issue numbers.

We look forward to your contributions!
