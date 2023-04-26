#----------------------------------------------------------------------------
# parser program makefile
#----------------------------------------------------------------------------

DIR_ROOT	:= $(CURDIR)/../../

include $(DIR_ROOT)/scripts/config.mk

DIR_TARGET	:= $(DIR_BUILD_LIB)/parser

SRC_LEX_FILES	:= $(wildcard *.l)
INT_LEX_FILES	:= $(SRC_LEX_FILES:%.l=$(DIR_TARGET)/%.yy.c)
BIN_LEX_FILES	:= $(INT_LEX_FILES:%.c=%.o)

SRC_BISON_FILES	:= $(wildcard *.y)
INT_BISON_FILES	:= $(SRC_BISON_FILES:%.y=$(DIR_TARGET)/%.tab.c)
BIN_BISON_FILES	:= $(INT_BISON_FILES:%.c=%.o)

SRC_C_FILES	:= $(wildcard *.c)
BIN_C_FILES	:= $(SRC_C_FILES:%.c=$(DIR_TARGET)/%.o)

TAG 		:= [parser]

all: mkdir compile
	@echo "$(TAG) Compiled successfully"

.PHONY: mkdir
mkdir:
	@mkdir -p $(DIR_TARGET)

.PHONY: clean
clean:
	@rm -f $(BIN_C_FILES)

$(INT_LEX_FILES): $(SRC_LEX_FILES)
	@echo "$(TAG) Generating flex analyser from $<"
	@flex -o $@ $< 

$(INT_BISON_FILES): $(SRC_BISON_FILES)
	@echo "$(TAG) Generating bison parser from $<"
	@bison -d -o $@ $<

# Both Flex and Bison can generate code that will make
# the current CCFLAGS scream. Since I don't control
# much of how the code will be generated, I will silence
# the warnings that are not application to this application.
#
# Additionally, given that we generate code into a different 
# folder, then I have to adjust the include paths to make it
# compile correctly.
define compile_lexpar_common
	@echo "$(TAG) Compiling $(notdir $<)"
	@$(CC) $(CCFLAGS)		\
		-I$(DIR_INCLUDE)	\
		-I$(DIR_TARGET)		\
		-c $< -o $@
endef

$(BIN_LEX_FILES): $(INT_LEX_FILES)
	$(compile_lexpar_common)

$(BIN_BISON_FILES): $(INT_BISON_FILES)
	$(compile_lexpar_common)

.PHONY: compile
compile: $(BIN_BISON_FILES) $(BIN_LEX_FILES) $(BIN_C_FILES)

$(BIN_C_FILES): $(SRC_C_FILES)
	$(compile_lexpar_common)

		

