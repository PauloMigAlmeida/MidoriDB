# MidoriDB
[![Unit Tests](https://github.com/PauloMigAlmeida/MidoriDB/actions/workflows/tests.yml/badge.svg)](https://github.com/PauloMigAlmeida/MidoriDB/actions/workflows/tests.yml)

<img src="https://github.com/PauloMigAlmeida/MidoriDB/assets/1011868/3356dfb3-e62c-4019-a43e-afe1a25b457a" alt="midoridblogo_readme" width="150">

In-memory database written in C completely from scratch.

## Features

* Dependency Minimalism: It relies solely on `libc` and `libm` in runtime, ensuring easy integration and reducing external dependencies
* Custom Implementation: Every component of MidoriDB has been crafted from the ground up - mostly because I wanted to learn about it really
* Small Footprint: approximately 200KB.


## Build

On ubuntu:

```bash
apt install bison flex libfl-dev

# if you want to build tests
apt install libcunit1-dev
```

To build it, just run: 

```bash
make
```

## Running tests

```bash
make all
./build/tests/run_unit_tests
```

## Wishlist
To make sure I won't lose focus on what I want this database to be able to do, I decided to write a list of features
that I want to implement in the short to medium term.

- [x] In-memory
- [x] Parser (CREATE, SELECT, INSERT, UPDATE, DELETE)
- [x] Recursive JOINs (INNER - more to come)
- [x] Recursive expressions (INSERT)
- [X] Locking [Granularity -> Table-level]


