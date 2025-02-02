# List all of your source files here separated by spaces.
SRC := sim86.cpp

# Set the name of your binary.
PRODUCT := sim86

################################################################################
# These configuration options change how your code (listed above) is compiled
# every time you type "make".
################################################################################

# This option sets which compiler your code will be compiled with.  Likely
# choices are icc, icpc, gcc, g++, clang
CC := clang

# These flags will be applied to your code any time it is built.
CFLAGS := -Wall -std=c++20

# These flags are applied only if you build your code with "make DEBUG=1".  -g
# generates debugging symbols, -DDEBUG defines the preprocessor symbol "DEBUG"
# (so that you can use "#ifdef DEBUG" in your code), and -O0 disables compiler
# optimizations, so that the binary generated more directly corresponds to your
# source code.
CFLAGS_DEBUG := -g -DDEBUG -O0

# -O3 sets the optimization level to three. -DNDEBUG defines the NDEBUG macro,
# which disables assertion checks.
CFLAGS_RELEASE := -O3 -DNDEBUG

# These flags are used to invoke Clang's address sanitizer.
CFLAGS_ASAN := -O1 -g -fsanitize=address -fno-omit-frame-pointer

# These flags are applied when linking object files together into your binary.
# If you need to link against libraries, add the appropriate flags here.  By
# default, your code is linked against the "rt" library with the flag -lrt;
# this library is used by the timing code in the testbed.
# LDFLAGS := -lrt -flto -fuse-ld=gold
LDFLAGS :=

################################################################################
# You probably won't need to change anything below this line, but if you're
# curious about how makefiles work, or if you'd like to customize the behavior
# of your makefile, go ahead and take a peek.
################################################################################

# You shouldn't need to touch this.  This keeps track of whether you are
# building in a release or debug configuration, and sets CFLAGS appropriately.
# (This mechanism is based on one from the original materials for 6.197 by
# Ceryen Tan and Marek Olszewski.)
OLDMODE=$(shell cat .buildmode 2> /dev/null)
ifeq ($(DEBUG),1)
  CFLAGS := $(CFLAGS_DEBUG) $(CFLAGS)
  ifneq ($(OLDMODE),debug)
    $(shell echo debug > .buildmode)
  endif
else ifeq ($(ASAN),1)
  CFLAGS := $(CFLAGS_ASAN) $(CFLAGS)
  LDFLAGS += -fsanitize=address
  ifneq ($(OLDMODE),asan)
    $(shell echo asan > .buildmode)
  endif
else
  CFLAGS := $(CFLAGS_RELEASE) $(CFLAGS)
  ifneq ($(OLDMODE),nodebug)
    $(shell echo nodebug > .buildmode)
  endif
endif

# When you invoke make without an argument, make behaves as though you had
# typed "make all", and builds whatever you have listed here.  (It knows to
# pick "make all" because "all" is the first rule listed.)
all: $(PRODUCT)

# This special "target" will remove the binary and all intermediate files.
clean::
	rm -f $(OBJ) $(PRODUCT) .buildmode \
        $(addsuffix .gcda, $(basename $(SRC))) \
        $(addsuffix .gcno, $(basename $(SRC))) \
        $(addsuffix .gcov, $(SRC) fasttime.h)

# This rule generates a list of object names.  Each of your source files (but
# not your header files) produces a single object file when it's compiled.  In
# a later step, all of those object files are linked together to produce the
# binary that you run.
OBJ = $(addsuffix .o, $(basename $(SRC)))

# These rules tell make how to automatically generate rules that build the
# appropriate object-file from each of the source files listed in SRC (above).
%.o : %.c .buildmode
	$(CC) $(CFLAGS) -c $< -o $@
%.o : %.cc .buildmode
	$(CC) $(CFLAGS) -c $< -o $@
%.o : %.cpp .buildmode
	$(CC) $(CFLAGS) -c $< -o $@

# This rule tells make that it can produce your binary by linking together all
# of the object files produced from your source files and any necessary
# libraries.
$(PRODUCT): $(OBJ) .buildmode
	$(CC) -o $@ $(OBJ) $(LDFLAGS)
