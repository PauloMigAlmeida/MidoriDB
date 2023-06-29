#----------------------------------------------------------------------------
# utils program makefile
#----------------------------------------------------------------------------

DIR_ROOT	:= $(CURDIR)/../../

include $(DIR_ROOT)/scripts/config.mk

DIR_TARGET	:= $(DIR_BUILD_TESTS)/utils

SRC_C_FILES	:= $(wildcard *.c)
BIN_C_FILES	:= $(SRC_C_FILES:%.c=$(DIR_TARGET)/%.o)

TAG 		:= [tests/utils]

all: mkdir compile
	@echo "$(TAG) Compiled successfully"

.PHONY: mkdir
mkdir:
	@mkdir -p $(DIR_TARGET)

.PHONY: clean
clean:
	@rm -f $(BIN_C_FILES)

.PHONY: compile
compile: $(BIN_C_FILES)

$(BIN_C_FILES): $(DIR_TARGET)/%.o: %.c
	@echo "$(TAG) Compiling $<"
	@$(CC) $(CCFLAGS) -I$(DIR_INCLUDE) -c $< -o $@

		

