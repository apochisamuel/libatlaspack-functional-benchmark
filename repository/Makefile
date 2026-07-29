CC ?= clang
CXX ?= clang++
CFLAGS ?= -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Isrc
CXXFLAGS ?= -std=c++17 -O2 -g -Wall -Wextra -Wpedantic -Isrc
AR ?= ar
ARFLAGS ?= rcs
SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)
LIB := libatlaspack.a

.PHONY: all clean test tools sanitize robustness coverage preflight
all: $(LIB)

$(LIB): $(OBJ)
	$(AR) $(ARFLAGS) $@ $^

src/%.o: src/%.c src/atlas.h src/internal.h
	$(CC) $(CFLAGS) -c $< -o $@

TEST_BINS := tests/test_atlas tests/test_malformed tests/test_mutation_sweep

tests/test_atlas: tests/test_atlas.c $(LIB)
	$(CC) $(CFLAGS) $< $(LIB) -o $@

tests/test_malformed: tests/test_malformed.c $(LIB)
	$(CC) $(CFLAGS) $< $(LIB) -o $@

tests/test_mutation_sweep: tests/test_mutation_sweep.c $(LIB)
	$(CC) $(CFLAGS) $< $(LIB) -o $@

test: $(TEST_BINS)
	./tests/test_atlas
	./tests/test_malformed
	./tests/test_mutation_sweep

tools: tools/atlas_dump

tools/atlas_dump: tools/atlas_dump.c $(LIB)
	$(CC) $(CFLAGS) $< $(LIB) -o $@

sanitize:
	$(MAKE) clean
	$(MAKE) CFLAGS="-std=c11 -O1 -g -Wall -Wextra -Wpedantic -Isrc -fsanitize=address,undefined -fno-omit-frame-pointer" test

robustness:
	./scripts/robustness_gate.sh

coverage:
	./scripts/coverage.sh

preflight:
	./scripts/preflight.sh

clean:
	rm -rf .build-fuzz
	rm -f src/*.o $(LIB) $(TEST_BINS) tools/atlas_dump
	rm -rf .coverage-build
