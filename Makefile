# ---------------------------------------------------------------------------
#  spfxd-engine - 2D game engine (C11 + OpenGL + SDL3)
#
#  make              build (debug by default)
#  make BUILD=release  optimized build
#  make run          build and run
#  make clean        remove build artifacts
#  make rebuild      clean + build
#  make help         show this help
# ---------------------------------------------------------------------------

# ---- Project layout -------------------------------------------------------
TARGET    := spfxd
BIN_DIR   := bin
BUILD_DIR := build
SRC_DIR   := src
INC_DIR   := inc
DEPS_DIR  := deps
GLAD_DIR  := $(DEPS_DIR)/glad

# ---- Configuration --------------------------------------------------------
CC    ?= cc
BUILD ?= debug          # debug | release
SAN   ?= 0              # SAN=1 enables ASan + UBSan (debug only)

# ---- Sources / objects ----------------------------------------------------
SRCS      := $(shell find $(SRC_DIR) -name '*.c') $(GLAD_DIR)/src/glad.c
OBJS      := $(SRCS:%.c=$(BUILD_DIR)/%.o)
DEP_FILES := $(OBJS:.o=.d)
EXE       := $(BIN_DIR)/$(TARGET)

# ---- SDL3 (via pkg-config) ------------------------------------------------
SDL_CFLAGS := $(shell pkg-config --cflags sdl3 2>/dev/null)
SDL_LIBS   := $(shell pkg-config --libs sdl3 2>/dev/null)

# ---- Flags ----------------------------------------------------------------
CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -MMD -MP \
           -I$(INC_DIR)/engine -I$(GLAD_DIR)/include -I$(DEPS_DIR) $(SDL_CFLAGS)
LDFLAGS :=
LDLIBS  := $(SDL_LIBS) -lm

ifeq ($(BUILD),release)
  CFLAGS += -O2 -DNDEBUG
else
  CFLAGS += -g -O0 -DDEBUG
  ifeq ($(SAN),1)
    CFLAGS  += -fsanitize=address,undefined -fno-omit-frame-pointer
    LDFLAGS += -fsanitize=address,undefined
  endif
endif

# glad needs libdl on Linux
UNAME := $(shell uname -s)
ifeq ($(UNAME),Linux)
  LDLIBS += -ldl
endif

# ---- Pretty output --------------------------------------------------------
ifneq ($(V),1)
  Q := @
endif

# ---- Rules ----------------------------------------------------------------
.PHONY: all run clean rebuild help check-sdl

all: check-sdl $(EXE)

$(EXE): $(OBJS)
	@mkdir -p $(@D)
	@echo "  LD    $@"
	$(Q)$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	@echo "  CC    $<"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

# Third-party code: silence warnings we don't control
$(BUILD_DIR)/$(GLAD_DIR)/src/glad.o: CFLAGS += -w

run: all
	./$(EXE)

clean:
	@echo "  CLEAN"
	$(Q)rm -rf $(BUILD_DIR) $(EXE)

rebuild: clean all

check-sdl:
	@pkg-config --exists sdl3 || { echo "error: SDL3 not found via pkg-config"; exit 1; }

help:
	@sed -n '2,10p' $(MAKEFILE_LIST) | sed 's/^# \{0,1\}//'

# Auto-generated header dependencies
-include $(DEP_FILES)
