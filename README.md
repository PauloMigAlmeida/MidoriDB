# MidoriDB
[![Unit Tests](https://github.com/PauloMigAlmeida/MidoriDB/actions/workflows/tests.yml/badge.svg)](https://github.com/PauloMigAlmeida/MidoriDB/actions/workflows/tests.yml)

<img src="https://github.com/PauloMigAlmeida/MidoriDB/assets/1011868/3356dfb3-e62c-4019-a43e-afe1a25b457a" alt="midoridblogo_readme" width="150">

In-memory database written in C completely from scratch.

## Features

* **Dependency Minimalism**: It relies solely on `libc` and `libm` in runtime, ensuring easy integration and reducing external dependencies
* **Custom Implementation**: Every component of MidoriDB has been crafted from the ground up - mostly because I wanted to learn about it really
* **Small Footprint**: approximately 200KB.


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

## Linking shared-object with your executable

```bash
gcc  -L<root_repo>/build/               \
        -lmidoridb                      \
        -Wl,-rpath <root_repo>/build/	\
        -o your_code
```

## Example

```C
#include <engine/query.h>

int main(void) {
        struct database db = {0};
        struct query_output *output;
        
        if (database_open(&db) != MIDORIDB_OK)
                return -1;        

        output = query_execute(&db, "SELECT "
                                    "    id_a, COUNT(*) "
                                    "FROM "
                                    "    A INNER JOIN B "
                                    "    ON A.id_a = B.id_b "
                                    "GROUP BY "
                                    "    id_a;");        

        if (output->status != ST_OK_WITH_RESULTS)
                return -1;
        
        while (query_cur_step(&output->results) == MIDORIDB_ROW) {
                printf("id_a: %ld, count: %ld\n"
                        query_column_int64(&output->results, 0),
                        query_column_int64(&output->results, 1));
        }        
        
        query_free(output);
        database_close(&db);
        return 0;
}
```

## Wishlist
To make sure I won't lose focus on what I want this database to be able to do, I decided to write a list of features
that I want to implement in the short to medium term.

- [x] In-memory
- [x] Parser (CREATE, SELECT, INSERT, UPDATE, DELETE)
- [x] Recursive JOINs (INNER - more to come)
- [x] Recursive expressions (INSERT)
- [X] Locking [Granularity -> Table-level]

## References

These are all the references that helped me a lot during the development of MidoriDB

Books:

* https://www.amazon.com/flex-bison-Text-Processing-Tools/dp/0596155972
* https://www.amazon.com/Database-Internals-Deep-Distributed-Systems/dp/1492040347

Courses:

* https://www.youtube.com/watch?v=LWS8LEQAUVc&list=PLSE8ODhjZXjYzlLMbX3cR0sxWnRM7CLFn
