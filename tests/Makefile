# Apollo test suite — characterization tests for the endian refactor.
#
# Builds the suite TWICE from the same sources, mirroring tools/Makefile:
#   test_apollo_le  — default little-endian behavior
#   test_apollo_be  — big-endian behavior (-D__PS3_PC__, the BIGENDIAN path)
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
CFLAGS  := -g -O2 -Wall -D_GNU_SOURCE -D_USE_MBEDTLS -DAPOLLO_CLI $(INC)
LDFLAGS := -L$(MBED) -lmbedcrypto -lz -lm

LIB_SRCS := crc_util list offzip patches decrypt loader packzip wildcard upy
TST_SRCS := test_common test_savewizard test_sw_endian_gaps test_bsd test_search test_parse test_corpus test_main

vpath %.c $(SRC) .

LE_DIR := build/le
BE_DIR := build/be
LE_OBJS := $(addprefix $(LE_DIR)/,$(addsuffix .o,$(LIB_SRCS) $(TST_SRCS)))
BE_OBJS := $(addprefix $(BE_DIR)/,$(addsuffix .o,$(LIB_SRCS) $(TST_SRCS)))

# Default corpus = vendored fixtures (reproducible from this repo alone).
# Point at the full patches repo for a broad sweep:  make check-corpus PATCHES=/path/to/apollo-patches
PATCHES ?= fixtures

.PHONY: all ci check golden check-corpus bsd-invariance corpus-determinism sweep clean

all: test_apollo_le test_apollo_be

# Portable CI gate: everything here compares same-machine outputs only.
ci: check bsd-invariance corpus-determinism
	@echo "== CI checks passed =="

$(LE_DIR)/%.o: %.c test_common.h | $(LE_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BE_DIR)/%.o: %.c test_common.h | $(BE_DIR)
	$(CC) $(CFLAGS) -D__PS3_PC__ -c -o $@ $<

$(LE_DIR) $(BE_DIR):
	mkdir -p $@

test_apollo_le: $(LE_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
test_apollo_be: $(BE_OBJS)
	$(CC) $(CFLAGS) -D__PS3_PC__ -o $@ $^ $(LDFLAGS)

check: all
	@echo "== little-endian ==" && ./test_apollo_le
	@echo "== big-endian ==" && ./test_apollo_be

golden: all
	@mkdir -p golden
	./test_apollo_le --corpus $(PATCHES) > golden/corpus_le.txt
	./test_apollo_be --corpus $(PATCHES) > golden/corpus_be.txt
	@echo "golden manifests regenerated from $(PATCHES)"

check-corpus: all
	@mkdir -p build
	@./test_apollo_le --corpus $(PATCHES) > build/corpus_le.now
	@./test_apollo_be --corpus $(PATCHES) > build/corpus_be.now
	@diff -u golden/corpus_le.txt build/corpus_le.now && echo "LE corpus: OK"
	@diff -u golden/corpus_be.txt build/corpus_be.now && echo "BE corpus: OK"

# BSD codes (type 2) are fully build-flag-invariant: BE* is unconditional and
# host-integer truncation uses HOST_LSB (host byte order, identical in both
# LE and __PS3_PC__ builds). Their manifest lines must match between the two
# builds — this is a hard pass/fail gate. Self-contained: compares fresh LE vs
# BE output on this machine, so it is portable across CI runners.
bsd-invariance: all
	@mkdir -p build
	@./test_apollo_le --corpus $(PATCHES) > build/corpus_le.now
	@./test_apollo_be --corpus $(PATCHES) > build/corpus_be.now
	@awk -F'\t' '$$2=="2"' build/corpus_le.now > build/bsd_le.txt
	@awk -F'\t' '$$2=="2"' build/corpus_be.now > build/bsd_be.txt
	@diff -u build/bsd_le.txt build/bsd_be.txt \
	  && echo "BSD invariance: OK ($$(wc -l < build/bsd_le.txt | tr -d ' ') codes, LE == BE)"

# The corpus must be reproducible: applying the same codes to the same fixed
# buffer twice yields byte-identical manifests. Same-machine, so portable.
corpus-determinism: all
	@mkdir -p build
	@./test_apollo_le --corpus $(PATCHES) > build/det_le_1.txt
	@./test_apollo_le --corpus $(PATCHES) > build/det_le_2.txt
	@diff build/det_le_1.txt build/det_le_2.txt && echo "corpus deterministic: LE OK"
	@./test_apollo_be --corpus $(PATCHES) > build/det_be_1.txt
	@./test_apollo_be --corpus $(PATCHES) > build/det_be_2.txt
	@diff build/det_be_1.txt build/det_be_2.txt && echo "corpus deterministic: BE OK"

sweep:
	@$(MAKE) check-corpus PATCHES=$(APOLLO_ROOT)/../apollo-patches

clean:
	rm -rf build test_apollo_le test_apollo_be
