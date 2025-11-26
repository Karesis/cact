# ===========================================================================
# CACT Compiler (cactc) Build System
# ===========================================================================

# === Configuration ===

TARGET_NAME := cactc
CC := clang

# --- Flags ---

# Include paths: local headers + fluf headers
INCLUDES := -Iinclude -Ivendor/fluf/include

# Compiler flags: C23 standard, strict warnings, debug info
CFLAGS := -g -Wall -Wextra -std=c23 \
          -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE \
          $(INCLUDES)
		  # -fsanitize=address -fno-omit-frame-pointer \

# Preprocessor flags: Generate dependency files (.d)
CPPFLAGS := -MMD -MP

# Linker flags
LDFLAGS := # -fsanitize=address
LDLIBS :=

# --- Dependencies (Fluf) ---

FLUF_DIR := vendor/fluf
FLUF_LIB := $(FLUF_DIR)/build/lib/libfluf.a

# Link against fluf static library
LDFLAGS += -L$(FLUF_DIR)/build/lib
LDLIBS += -lfluf

# === Directories ===

SRC_DIR := src
TEST_DIR := tests
BUILD_DIR := build

BIN_DIR := $(BUILD_DIR)/bin
OBJ_DIR := $(BUILD_DIR)/obj

# === Files Discovery ===

# 1. Source Files (recursive find)
SRCS := $(shell find $(SRC_DIR) -name '*.c')

# 2. Object Files
# src/foo.c -> build/obj/src/foo.o
ALL_OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/src/%.o,$(SRCS))

# 3. Main vs Lib Objects
# We assume src/main.c is the entry point for the CLI app.
# Tests should link against everything EXCEPT main.o to avoid "multiple definition of main".
MAIN_OBJ := $(OBJ_DIR)/src/main.o
LIB_OBJS := $(filter-out $(MAIN_OBJ), $(ALL_OBJS))

# 4. Test Files
# tests/test_lexer.c
TEST_SRCS := $(wildcard $(TEST_DIR)/test_*.c)
# tests/test_lexer.c -> build/bin/test_lexer
TEST_BINS := $(patsubst $(TEST_DIR)/%.c,$(BIN_DIR)/%,$(TEST_SRCS))

# 5. Final Targets
TARGET_BIN := $(BIN_DIR)/$(TARGET_NAME)

# 6. Dependency Files (.d)
DEPS := $(ALL_OBJS:.o=.d) $(TEST_BINS:=.d)

# === Installation Config ===

PREFIX ?= /usr/local
INSTALL_BIN := $(PREFIX)/bin

# ===========================================================================
# Recipes
# ===========================================================================

.PHONY: all clean install uninstall update run test check test_samples

# Default target: Build the compiler binary
all: $(TARGET_BIN)

# --- Link Main Compiler ---
$(TARGET_BIN): $(ALL_OBJS) $(FLUF_LIB)
	@echo "[LINK]    $@"
	@mkdir -p $(dir $@)
	$(CC) $(ALL_OBJS) $(LDFLAGS) $(LDLIBS) -o $@

# --- Compile Source Objects ---
$(OBJ_DIR)/src/%.o: $(SRC_DIR)/%.c
	@echo "[CC]      $<"
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# --- Compile & Link Tests ---
# Each test binary depends on the test source + library objects + fluf
$(BIN_DIR)/test_%: $(TEST_DIR)/test_%.c $(LIB_OBJS) $(FLUF_LIB)
	@echo "[TEST-LD] $@"
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) $< $(LIB_OBJS) $(LDFLAGS) $(LDLIBS) -o $@

# --- Build Dependency: Fluf ---
$(FLUF_LIB):
	@echo "[DEP]     Building fluf..."
	@$(MAKE) -C $(FLUF_DIR)

# ===========================================================================
# Utility Commands
# ===========================================================================

# --- Testing ---
# Build and Run all tests
test: $(TEST_BINS)
	@echo
	@echo "=== Running CACT Tests ==="
	@echo
	@$(foreach test,$(TEST_BINS), \
		echo "[RUN]     $(test)"; \
		./$(test) || exit 1; \
	)
	@echo
	@echo "=== All Tests Passed ==="
	@echo

test_samples: $(TARGET_BIN)
	@echo "[TEST]    Running Sample Integration Tests..."
	@python3 scripts/test_samples.py

# Alias for 'test'
check: test

# --- Running ---
run: all
	@echo "[RUN]     $(TARGET_BIN)"
	@$(TARGET_BIN)

# --- Cleaning ---
clean:
	@echo "[CLEAN]   $(TARGET_NAME)"
	@rm -rf $(BUILD_DIR)
	@echo "[CLEAN]   fluf"
	@$(MAKE) -C $(FLUF_DIR) clean

# ===========================================================================
# Project Management
# ===========================================================================

# --- Update ---
update:
	@echo "[UPDATE]  Updating project..."
	@if [ "$$(id -u)" -eq 0 ]; then \
		echo "Error: Do not run 'make update' as root."; \
		exit 1; \
	fi
	@echo "[GIT]     Pulling changes..."
	@git pull
	@echo "[GIT]     Updating submodules..."
	@git submodule update --init --recursive
	@echo "[MAKE]    Rebuilding..."
	@$(MAKE) all
	@echo "==> Update complete."

# --- Installation ---
install: all
	@echo "[CHECK]   Root privileges..."
	@if [ "$$(id -u)" -ne 0 ]; then \
		echo "Error: Run as root (sudo make install)."; \
		exit 1; \
	fi
	@echo "[INSTALL] $(TARGET_NAME) -> $(INSTALL_BIN)"
	@mkdir -p $(INSTALL_BIN)
	@cp $(TARGET_BIN) $(INSTALL_BIN)/$(TARGET_NAME)
	@chmod +x $(INSTALL_BIN)/$(TARGET_NAME)

uninstall:
	@echo "[CHECK]   Root privileges..."
	@if [ "$$(id -u)" -ne 0 ]; then \
		echo "Error: Run as root (sudo make uninstall)."; \
		exit 1; \
	fi
	@echo "[REMOVE]  $(INSTALL_BIN)/$(TARGET_NAME)"
	@rm -f $(INSTALL_BIN)/$(TARGET_NAME)

# ===========================================================================
# Versioning (Git Tags)
# ===========================================================================

# 1. Get current tag
CURRENT_TAG := $(shell git describe --tags --abbrev=0 2>/dev/null || echo "v0.0.0")

# 2. Parse version (v0.1.2 -> MAJOR=0 MINOR=1 PATCH=2)
VERSION_BITS := $(subst v,,$(CURRENT_TAG))
MAJOR := $(word 1,$(subst ., ,$(VERSION_BITS)))
MINOR := $(word 2,$(subst ., ,$(VERSION_BITS)))
PATCH := $(word 3,$(subst ., ,$(VERSION_BITS)))

# 3. Calculate next patch version
NEXT_PATCH := $(shell echo $$(($(PATCH)+1)))
NEW_TAG := v$(MAJOR).$(MINOR).$(NEXT_PATCH)

# 4. Default message
NOTE ?= Maintenance update

.PHONY: btag

btag:
	@if [ -n "$$(git status --porcelain)" ]; then \
		echo "Error: Working directory dirty. Commit changes first."; \
		exit 1; \
	fi
	@echo "[RELEASE] $(CURRENT_TAG) -> $(NEW_TAG)"
	@echo "[MESSAGE] $(NOTE)"
	@git tag -a $(NEW_TAG) -m "Release $(NEW_TAG): $(NOTE)"
	@git push origin $(NEW_TAG)

# Include dependency graph
ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif
