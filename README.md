# ECC
A super simple, minimal entity component system that is implemented within a single header file in C as a learning exercise.

`ecc` has a very similar architecture to my Go ECS implementation [`ecgo`](https://github.com/Evankj/ecgo) but using very basic memory arenas and linked lists and some questionable pointer arithmetic.

I intend to create a simple game using [`raylib`](https://www.raylib.com/) and `ecc` in the future.

## Running the (minimal, scuffed) tests
```sh
make test
```
