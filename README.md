# C++20 Result<T, E> Implementation

A lightweight, header-only implementation inspired by `std::expected` and Rust's `Result<T, E>` with a more familiar name and slightly different semantics, such as supporting an empty state after moving from a value. Developed for educational purposes and personal use, with a minimal design in mind.

## Building 🛠️

The library is header-only and requires no installation or build step. You can simply include the desired header in your project.

The `xmake.lua` script is provided for building the test executable.

## License ⚖️

The project is licensed under the **Apache License 2.0**. See [LICENSE.txt](LICENSE.txt) for further information.
