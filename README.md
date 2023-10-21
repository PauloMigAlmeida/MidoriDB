# MidoriDB
[![Unit Tests](https://github.com/PauloMigAlmeida/MidoriDB/actions/workflows/tests.yml/badge.svg)](https://github.com/PauloMigAlmeida/MidoriDB/actions/workflows/tests.yml)

<img src="https://github.com/PauloMigAlmeida/MidoriDB/assets/1011868/3356dfb3-e62c-4019-a43e-afe1a25b457a" alt="midoridblogo_readme" width="150">

In-memory database written in C from scratch

## Dependencies

On ubuntu:

```bash
apt install bison flex libfl-dev

# if you want to build tests
apt install libcunit1-dev
```


## Build

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

- [ ] Transaction [Model -> Flat Transation, Isolation -> Read uncommitted]
- [X] Locking [Granularity -> Table-level]
- [X] In-memory database
- [ ] Write Ahead Log

## Strech goals
- [ ] Transaction [Isolation -> Read committed (seems to be the default in most popular database engines)]
- [ ] Locking [Granularity -> Row-level]
