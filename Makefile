# Makefile for adept

# General setup
MAX_THREADS := 4

# Directory structure
SRC_DIR := src
INC_DIR := include 
BUILD_DIR := build
BIN_DIR := bin

RELEASE_BUILD_DIR := $(BUILD_DIR)/release
DEBUG_BUILD_DIR := $(BUILD_DIR)/debug

RELEASE_BIN_DIR := $(BIN_DIR)/release
DEBUG_BIN_DIR := $(BIN_DIR)/debug

COMP_DB_DIR := $(BUILD_DIR)/compdb

# Targets
TARGET_NAME := adept
RELEASE_TARGET := $(RELEASE_BIN_DIR)/$(TARGET_NAME)
DEBUG_TARGET := $(DEBUG_BIN_DIR)/$(TARGET_NAME)

# Compile inputs / outputs
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
INCS := $(wildcard $(INC_DIR)/*.h*)
RELEASE_OBJS := $(addsuffix .o,$(subst $(SRC_DIR),$(RELEASE_BUILD_DIR),$(SRCS)))
DEBUG_OBJS := $(addsuffix .o,$(subst $(SRC_DIR),$(DEBUG_BUILD_DIR),$(SRCS)))

# Compilation database for clangd
COMP_DB_FILES := $(addsuffix .json,$(subst $(SRC_DIR),$(COMP_DB_DIR),$(SRCS)))
COMP_DB_OUTPUT := compile_commands.json

# Libraries to link
SHARED_LIBS := curl
TINYXML2_LIB := libtinyxml2.a
TINYXML2_DIR := external/tinyxml2
RELEASE_TINYXML2 := $(TINYXML2_DIR)/lib/release/$(TINYXML2_LIB)
DEBUG_TINYXML2 := $(TINYXML2_DIR)/lib/debug/$(TINYXML2_LIB)
TINYXML2_INC_DIR := $(TINYXML2_DIR)/include

# GCC Flags / tools
CXX_STANDARD := c++17

CC := clang
CXX := clang++
CPP_FLAGS := -I$(INC_DIR) -I$(TINYXML2_INC_DIR)
CXX_FLAGS := -std=$(CXX_STANDARD) -fexceptions -frtti -Wall -pthread

RELEASE_FLAGS := -O3
DEBUG_FLAGS := -g -Og

LD_FLAGS := -static-libstdc++ $(addprefix -l,$(SHARED_LIBS))

.PHONY : release

release :
	$(MAKE) -j$(MAX_THREADS) $(RELEASE_TARGET)

$(RELEASE_TARGET) : $(RELEASE_OBJS) $(RELEASE_TINYXML2)
	@echo Building...
	@mkdir -p $(RELEASE_BIN_DIR)
	$(CXX) $(LD_FLAGS) $^ \
	-o $(RELEASE_TARGET)

$(RELEASE_BUILD_DIR)/%.o : $(SRC_DIR)/%
	@echo Compiling $<...
	@mkdir -p $(RELEASE_BUILD_DIR)
	$(CXX) $(CPP_FLAGS) $(CXX_FLAGS) $(RELEASE_FLAGS) \
	-c $< -o $@

$(RELEASE_TINYXML2) :
	$(MAKE) -C$(TINYXML2_DIR) -j$(MAX_THREADS) release

.PHONY : debug

debug :
	$(MAKE) -j$(MAX_THREADS) $(DEBUG_TARGET)

$(DEBUG_TARGET) : $(DEBUG_OBJS) $(DEBUG_TINYXML2)
	@echo Building...
	@mkdir -p $(DEBUG_BIN_DIR)
	$(CXX) $(LD_FLAGS) $^ \
	-o $(DEBUG_TARGET)

$(DEBUG_BUILD_DIR)/%.o : $(SRC_DIR)/%
	@echo Compiling $<...
	@mkdir -p $(DEBUG_BUILD_DIR)
	$(CXX) $(CPP_FLAGS) $(CXX_FLAGS) $(DEBUG_FLAGS) \
	-c $< -o $@

$(DEBUG_TINYXML2) :
	$(MAKE) -C$(TINYXML2_DIR) -j$(MAX_THREADS) debug

.PHONY : compdb

compdb :
	@echo $(COMP_DB_FILES)
	@$(MAKE) -j$(MAX_THREADS) $(COMP_DB_OUTPUT)

$(COMP_DB_OUTPUT) : $(COMP_DB_FILES)
	@echo Generating compile commands database...
	@sed -e '1s/^/[\'$$'\n''/' -e '$$s/,$$/\'$$'\n'']/' \
	$(COMP_DB_DIR)/*.json > $(COMP_DB_OUTPUT)

$(COMP_DB_DIR)/%.json : $(SRC_DIR)/%
	@echo Generating compile commands for $<...
	@mkdir -p $(COMP_DB_DIR)
	$(CXX) -E -MJ$@ $(CPP_FLAGS) $(CXX_FLAGS) -c $< -o $@.tmp
	@rm -f $@.tmp

.PHONY : clean

clean :
	@rm -rf build bin
	$(MAKE) -C $(TINYXML2_DIR) clean
