# Apollo test suite — characterization tests for the endian refactor.
#
# ONE binary, test_apollo, covering both endian modes. The library selects
# save-data endianness at runtime, so `make check` runs the vector suite twice
# in a single process (LE pass, then BE pass) and prints the same two summaries
# the old test_apollo_le / test_apollo_be pair did. Corpus mode takes --be to
# pick one, which is what bsd-invariance compares.
#
# Targets:
#   make ci               self-contained gate for CI (portable across machines)
#   make check            run the hand-authored opcode vectors (LE + BE)
#   make bsd-invariance   assert BSD (type 2) output is identical LE vs BE
#   make corpus-determinism  assert the corpus is reproducible run-to-run
#   make golden           (re)generate golden corpus manifests from $(PATCHES)
#   make check-corpus     re-run the corpus and diff against committed goldens
#   make sweep            convenience: check-corpus over the full patches repo
#   make clean
#
# The suite also cross-compiles: `make -f ../Makefile.wasm test` builds it for
# wasm32 and runs it under Node, which is how the WASM platform is gated. The
# `--corpus` targets are native-only — test_corpus.c isolates each code in a
# fork(), which Emscripten does not implement.
#
# CI note: `check`, `bsd-invariance` and `corpus-determinism` only compare
# outputs produced on the SAME machine, so they are portable across runners.
# `check-corpus` diffs against committed goldens, which embed a platform-
# specific crash signal number (SIGBUS vs SIGSEGV) — it is a LOCAL check for the
# developer's machine, run before/after the endian refactor, not a CI gate.

SHELL := /bin/bash

APOLLO_ROOT := ..
SRC  := $(APOLLO_ROOT)/source
INC  := -I$(APOLLO_ROOT)/include -I$(APOLLO_ROOT)/mbedtls-2.16.12/include -I$(APOLLO_ROOT)/tools -I.
MBED := $(APOLLO_ROOT)/mbedtls-2.16.12/build/library

CC ?= gcc

# Cross-compilation knobs. Defaults are a native host build; Makefile.wasm
# overrides them to build the same suite for wasm32 and run it under Node
# (EXE=.js RUN=node LIBZ= EXTRA_CFLAGS=--use-port=zlib ...).
EXE           ?=
RUN           ?=
# Object/scratch tree. Must differ per toolchain: the objects are not
# interchangeable, and make only compares timestamps, so a shared tree lets a
# wasm object satisfy a native link (and vice versa).
BUILD         ?= build
# zlib comes from -lz natively; under Emscripten it comes from
# --use-port=zlib in EXTRA_CFLAGS instead, so the wasm build clears this.
LIBZ          ?= -lz
EXTRA_CFLAGS  ?=
EXTRA_LDFLAGS ?=

CFLAGS  := -g -O2 -Wall -D_GNU_SOURCE -D_USE_MBEDTLS -DAPOLLO_CLI $(INC) $(EXTRA_CFLAGS)
LDFLAGS := -L$(MBED) -lmbedcrypto $(LIBZ) -lm $(EXTRA_LDFLAGS)

LIB_SRCS := crc_util list offzip patches decrypt loader packzip wildcard upy
TST_SRCS := test_common test_offzip test_crypt_bsd test_mgspw test_samples test_savewizard test_sw_endian_gaps test_bsd test_search test_parse test_bounds test_null test_corpus test_main

vpath %.c $(SRC) .

OBJ_DIR := $(BUILD)/obj
OBJS    := $(addprefix $(OBJ_DIR)/,$(addsuffix .o,$(LIB_SRCS) $(TST_SRCS)))

# Default corpus = vendored fixtures (reproducible from this repo alone).
# Point at the full patches repo for a broad sweep:  make check-corpus PATCHES=/path/to/apollo-patches
PATCHES ?= fixtures

.PHONY: all ci check check-mgspw check-samples golden check-corpus bsd-invariance corpus-determinism sweep clean

all: test_apollo$(EXE)

# Portable CI gate: everything here compares same-machine outputs only.
ci: check bsd-invariance corpus-determinism
	@echo "== CI checks passed =="

$(OBJ_DIR)/%.o: %.c test_common.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR):
	mkdir -p $@

test_apollo$(EXE): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Runs both passes; the binary prints a header and totals for each.
check: all
	@$(RUN) ./test_apollo$(EXE)

# Opt-in correctness check against a real MGS Peace Walker save (not vendored).
# Expects the encrypted save, with its decrypted twin alongside as <file>.dec:
#   make check-mgspw MGSPW_SAVE=/path/to/00000000.000
check-mgspw: all
	@[ -n "$(MGSPW_SAVE)" ] || { echo "set MGSPW_SAVE=/path/to/00000000.000"; exit 2; }
	@MGSPW_SAVE="$(MGSPW_SAVE)" $(RUN) ./test_apollo$(EXE) | grep -E "mgspw|checks,"

# Opt-in known-answer checks against real game saves from the save-decrypters
# repo (not vendored):
#
#   make check-samples SAMPLES=/path/to/save-decrypters
#
# The Python vectors additionally need the helper modules that their patches
# import, which live in apollo-patches. PATCHES already names that checkout for
# the corpus targets, so point it there to include them:
#
#   make check-samples SAMPLES=/path/to/save-decrypters \
#                      PATCHES=/path/to/apollo-patches
#
# Without it PATCHES stays at its `fixtures` default, which has no python/,
# and the Python vectors skip while the rest still run.
check-samples: all
	@[ -n "$(SAMPLES)" ] || { echo "set SAMPLES=/path/to/save-decrypters"; exit 2; }
	@SAMPLES="$(SAMPLES)" PATCHES="$(PATCHES)" $(RUN) ./test_apollo$(EXE) | grep -E "sample_|skipped|checks,"

golden: all
	@mkdir -p golden
	$(RUN) ./test_apollo$(EXE) --corpus $(PATCHES) > golden/corpus_le.txt
	$(RUN) ./test_apollo$(EXE) --corpus $(PATCHES) --be > golden/corpus_be.txt
	@echo "golden manifests regenerated from $(PATCHES)"

check-corpus: all
	@mkdir -p $(BUILD)
	@$(RUN) ./test_apollo$(EXE) --corpus $(PATCHES) > $(BUILD)/corpus_le.now
	@$(RUN) ./test_apollo$(EXE) --corpus $(PATCHES) --be > $(BUILD)/corpus_be.now
	@diff -u golden/corpus_le.txt $(BUILD)/corpus_le.now && echo "LE corpus: OK"
	@diff -u golden/corpus_be.txt $(BUILD)/corpus_be.now && echo "BE corpus: OK"

# BSD codes (type 2) are fully build-flag-invariant: BE* is unconditional and
# integer truncation goes through _set_var_slice(), which is keyed on the host
# rather than on the save-data mode (identical in both LE and BE modes). Their manifest lines must match between the two passes —
# this is a hard pass/fail gate. Self-contained: compares fresh LE vs BE output
# on this machine, so it is portable across CI runners.
bsd-invariance: all
	@mkdir -p $(BUILD)
	@$(RUN) ./test_apollo$(EXE) --corpus $(PATCHES) > $(BUILD)/corpus_le.now
	@$(RUN) ./test_apollo$(EXE) --corpus $(PATCHES) --be > $(BUILD)/corpus_be.now
	@awk -F'\t' '$$2=="2"' $(BUILD)/corpus_le.now > $(BUILD)/bsd_le.txt
	@awk -F'\t' '$$2=="2"' $(BUILD)/corpus_be.now > $(BUILD)/bsd_be.txt
	@diff -u $(BUILD)/bsd_le.txt $(BUILD)/bsd_be.txt \
	  && echo "BSD invariance: OK ($$(wc -l < $(BUILD)/bsd_le.txt | tr -d ' ') codes, LE == BE)"

# The corpus must be reproducible: applying the same codes to the same fixed
# buffer twice yields byte-identical manifests. Same-machine, so portable.
corpus-determinism: all
	@mkdir -p $(BUILD)
	@$(RUN) ./test_apollo$(EXE) --corpus $(PATCHES) > $(BUILD)/det_le_1.txt
	@$(RUN) ./test_apollo$(EXE) --corpus $(PATCHES) > $(BUILD)/det_le_2.txt
	@diff $(BUILD)/det_le_1.txt $(BUILD)/det_le_2.txt && echo "corpus deterministic: LE OK"
	@$(RUN) ./test_apollo$(EXE) --corpus $(PATCHES) --be > $(BUILD)/det_be_1.txt
	@$(RUN) ./test_apollo$(EXE) --corpus $(PATCHES) --be > $(BUILD)/det_be_2.txt
	@diff $(BUILD)/det_be_1.txt $(BUILD)/det_be_2.txt && echo "corpus deterministic: BE OK"

sweep:
	@$(MAKE) check-corpus PATCHES=$(APOLLO_ROOT)/../apollo-patches

clean:
	rm -rf $(BUILD) test_apollo$(EXE) test_apollo.wasm
