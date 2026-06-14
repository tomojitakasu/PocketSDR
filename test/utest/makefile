#
#  makefile for Pocket SDR unit tests
#
#  Intended environment: MSYS2 UCRT64
#

USE_FFTW ?= 0
USE_AVX2 ?= 1

SRC = ../../src
LIB = ../../lib
TEST_SRC = ../src

TESTS = \
    test_sdr_array \
    test_sdr_ch \
    test_sdr_cmn \
    test_sdr_code \
    test_sdr_code_gal \
    test_sdr_conf \
    test_sdr_dev \
    test_sdr_fec \
    test_sdr_func \
    test_sdr_ldpc \
    test_sdr_nav \
    test_sdr_nb_ldpc \
    test_sdr_pvt \
    test_sdr_rcv \
    test_sdr_sdev \
    test_sdr_usb

PERF_TESTS = \
    test_sdr_perf

ifeq ($(OS),Windows_NT)
    CC = g++
    EXE = .exe
    LIB_DIR = $(LIB)/win32
else
    UNAME_S := $(shell uname -s)
    UNAME_M := $(shell uname -m)
    EXE =
    ifeq ($(UNAME_S),Darwin)
        CC = clang++
        LIB_DIR = $(LIB)/macos
    else
        CC = g++
        LIB_DIR = $(LIB)/linux
    endif
endif

INCLUDE = -I$(TEST_SRC) -I$(SRC) -I$(LIB)/RTKLIB/src

WARNOPT = -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable \
          -Wno-unused-function -Wno-missing-field-initializers

OPTIONS =
LDFLAGS =
SYS_LIBS =
STATIC_OPT =

ifeq ($(OS),Windows_NT)
    INCLUDE += -I$(LIB)/cyusb -I$(LIB)/cyusb/inc
    OPTIONS += -DWIN32
    LDFLAGS += -Wl,--gc-sections
    SYS_LIBS += -lwinmm -lws2_32
    STATIC_OPT = -static
    ifeq ($(USE_AVX2),1)
        OPTIONS += -DAVX2 -mavx2 -mfma
    endif
else ifeq ($(UNAME_S),Darwin)
    OPTIONS += -DMACOS -Wno-deprecated
    LDFLAGS += -Wl,-dead_strip
    SYS_LIBS += -lpthread
    ifeq ($(UNAME_M),arm64)
        INCLUDE += -I/opt/homebrew/include
        OPTIONS += -DNEON
    else ifeq ($(USE_AVX2),1)
        OPTIONS += -DAVX2 -mavx2 -mfma
    endif
else
    LDFLAGS += -Wl,--gc-sections
    SYS_LIBS += -lm -lpthread
    ifeq ($(UNAME_M),aarch64)
        OPTIONS += -DNEON
    else ifeq ($(USE_AVX2),1)
        OPTIONS += -DAVX2 -mavx2 -mfma
    endif
endif

ifeq ($(USE_FFTW),1)
    OPTIONS += -DFFTW
    FFT_LIBS = -lfftw3f
else
    INCLUDE += -I$(LIB)/pocketfft
    FFT_LIBS = $(LIB_DIR)/libpocketfft.a
endif

CFLAGS = -O2 -g $(INCLUDE) $(WARNOPT) $(OPTIONS) \
         -ffunction-sections -fdata-sections

LDLIBS = $(STATIC_OPT) $(LIB_DIR)/librtk.a $(FFT_LIBS) $(SYS_LIBS)

TARGETS = $(addsuffix $(EXE),$(TESTS))
PERF_TARGETS = $(addsuffix $(EXE),$(PERF_TESTS))

STUB_USB  = $(TEST_SRC)/sdr_usb_stub.c
STUB_DEV  = $(TEST_SRC)/sdr_dev_stub.c
STUB_SDEV = $(TEST_SRC)/sdr_sdev_stub.c
STUB_RCV  = $(TEST_SRC)/sdr_rcv_stub.c

SRC_test_sdr_array    = $(SRC)/sdr_array.c $(SRC)/sdr_cmn.c
SRC_test_sdr_ch       = $(SRC)/sdr_ch.c $(SRC)/sdr_nav.c $(SRC)/sdr_code.c \
                        $(SRC)/sdr_code_gal.c $(SRC)/sdr_func.c \
                        $(SRC)/sdr_fec.c $(SRC)/sdr_ldpc.c \
                        $(SRC)/sdr_nb_ldpc.c $(SRC)/sdr_cmn.c
SRC_test_sdr_cmn      = $(SRC)/sdr_cmn.c
SRC_test_sdr_code     = $(SRC)/sdr_code.c $(SRC)/sdr_code_gal.c \
                        $(SRC)/sdr_func.c $(SRC)/sdr_cmn.c
SRC_test_sdr_code_gal = $(SRC)/sdr_code_gal.c
SRC_test_sdr_conf     = $(SRC)/sdr_conf.c $(STUB_USB) $(SRC)/sdr_cmn.c
SRC_test_sdr_dev      = $(STUB_DEV) $(SRC)/sdr_cmn.c
SRC_test_sdr_fec      = $(SRC)/sdr_fec.c $(SRC)/sdr_cmn.c
SRC_test_sdr_func     = $(SRC)/sdr_func.c $(SRC)/sdr_cmn.c
SRC_test_sdr_ldpc     = $(SRC)/sdr_ldpc.c $(SRC)/sdr_nb_ldpc.c \
                        $(SRC)/sdr_cmn.c
SRC_test_sdr_nav      = $(SRC)/sdr_nav.c $(SRC)/sdr_fec.c \
                        $(SRC)/sdr_ldpc.c $(SRC)/sdr_nb_ldpc.c \
                        $(SRC)/sdr_code.c $(SRC)/sdr_code_gal.c \
                        $(SRC)/sdr_func.c $(SRC)/sdr_cmn.c
SRC_test_sdr_nb_ldpc  = $(SRC)/sdr_nb_ldpc.c $(SRC)/sdr_cmn.c
SRC_test_sdr_pvt      = $(SRC)/sdr_pvt.c $(SRC)/sdr_func.c \
                        $(STUB_RCV) $(SRC)/sdr_cmn.c
SRC_test_sdr_perf     = $(SRC)/sdr_func.c $(SRC)/sdr_code.c \
                        $(SRC)/sdr_code_gal.c $(SRC)/sdr_array.c \
                        $(SRC)/sdr_cmn.c
SRC_test_sdr_rcv      = $(SRC)/sdr_rcv.c $(SRC)/sdr_ch.c $(SRC)/sdr_nav.c \
                        $(SRC)/sdr_code.c $(SRC)/sdr_code_gal.c \
                        $(SRC)/sdr_func.c $(SRC)/sdr_array.c \
                        $(SRC)/sdr_pvt.c $(SRC)/sdr_conf.c \
                        $(SRC)/sdr_fec.c $(SRC)/sdr_ldpc.c \
                        $(SRC)/sdr_nb_ldpc.c $(STUB_DEV) $(STUB_SDEV) \
                        $(STUB_USB) $(SRC)/sdr_cmn.c
SRC_test_sdr_sdev     = $(STUB_SDEV) $(SRC)/sdr_cmn.c
SRC_test_sdr_usb      = $(STUB_USB) $(SRC)/sdr_cmn.c

CFLAGS_test_sdr_perf  = -O3 $(filter-out -O2,$(CFLAGS))
CFLAGS_test_sdr_ch    = $(CFLAGS) -DSDR_CH_TEST
CFLAGS_test_sdr_nav   = $(CFLAGS) -DSDR_NAV_TEST

all: $(TARGETS)

define BUILD_TEST
$(1)$(EXE): $(TEST_SRC)/$(1).c $$(SRC_$(1)) $(TEST_SRC)/test_sdr.h
	$(CC) $$(or $$(CFLAGS_$(1)),$$(CFLAGS)) $(LDFLAGS) -o $$@ $(TEST_SRC)/$(1).c $$(SRC_$(1)) $(LDLIBS)
endef

$(foreach T,$(TESTS) $(PERF_TESTS),$(eval $(call BUILD_TEST,$(T))))

ifneq ($(EXE),)
$(TESTS) $(PERF_TESTS): %: %$(EXE)
endif

test: all
	@set -e; for t in $(TARGETS); do echo "==> $$t"; ./$$t; done

perf: test_sdr_perf$(EXE)
	./test_sdr_perf$(EXE)

clean:
	rm -rf build
	rm -f $(TARGETS) $(PERF_TARGETS) sdr_func_c_test sdr_func_c_test.exe \
	    $(TESTS) $(PERF_TESTS) *.o *.stackdump
