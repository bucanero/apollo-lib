# Apollo test suite — characterization tests for the endian refactor.
#
# Builds the suite TWICE from the same sources, mirroring tools/Makefile:
#   test_apollo_le  — default little-endian behavior
#   test_apollo_be  — big-endian behavior (-D__PS3_PC__, the BIGENDIAN path)
#
# Targets:
#   make check            run the hand-authored opcode vectors (LE + BE)
#   make golden           (re)generate golden corpus manifests from $(PATCHES)
#   make check-corpus     re-run the corpus and diff against committed goldens
#   make bsd-invariance   assert BSD (type 2) output is identical LE vs BE
#   make sweep            convenience: check-corpus over the full patches repo
#   make clean

SHELL := /bin/bash

APOLLO_ROOT := ..
SRC  := $(APOLLO_ROOT)/source
INC  := -I$(APOLLO_ROOT)/include -I$(APOLLO_ROOT)/mbedtls-2.16.12/include -I$(APOLLO_ROOT)/tools -I.
MBED := $(APOLLO_ROOT)/mbedtls-2.16.12/build/library

CC ?= gcc
CFLAGS  := -g -O2 -Wall -D_GNU_SOURCE -D_USE_MBEDTLS -DAPOLLO_CLI $(INC)
LDFLAGS := -L$(MBED) -lmbedcrypto -lz -lm

LIB_SRCS := crc_util list offzip patches decrypt loader packzip wildcard upy
TST_SRCS := test_common test_savewizard test_bsd test_search test_parse test_corpus test_main

vpath %.c $(SRC) .

LE_DIR := build/le
BE_DIR := build/be
LE_OBJS := $(addprefix $(LE_DIR)/,$(addsuffix .o,$(LIB_SRCS) $(TST_SRCS)))
BE_OBJS := $(addprefix $(BE_DIR)/,$(addsuffix .o,$(LIB_SRCS) $(TST_SRCS)))

# Default corpus = vendored fixtures (reproducible from this repo alone).
# Point at the full patches repo for a broad sweep:  make check-corpus PATCHES=/path/to/apollo-patches
PATCHES ?= fixtures

.PHONY: all check golden check-corpus bsd-invariance sweep clean

all: test_apollo_le test_apollo_be

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
# builds — this is a hard pass/fail gate.
bsd-invariance: golden
	@mkdir -p build
	@awk -F'\t' '$$2=="2"' golden/corpus_le.txt > build/bsd_le.txt
	@awk -F'\t' '$$2=="2"' golden/corpus_be.txt > build/bsd_be.txt
	@diff -u build/bsd_le.txt build/bsd_be.txt \
	  && echo "BSD invariance: OK ($$(wc -l < build/bsd_le.txt | tr -d ' ') codes, LE == BE)"

sweep:
	@$(MAKE) check-corpus PATCHES=$(APOLLO_ROOT)/../apollo-patches

clean:
	rm -rf build test_apollo_le test_apollo_be
