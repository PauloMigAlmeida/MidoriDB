#----------------------------------------------------------------------------
# MidoriDB root makefile
#----------------------------------------------------------------------------

DIR_ROOT := $(CURDIR)

include $(DIR_ROOT)/scripts/config.mk

DIR_SRC_SUBSYSTEMS 		:= $(wildcard $(DIR_SRC)/*)
DIR_SRC_TESTS_SUBSYSTEMS 	:= $(wildcard $(DIR_SRC_TESTS)/*)

#----------------------------------------------------------------------------
# Build targets
#----------------------------------------------------------------------------

default: build

all: clean compile linker test
	@# Help: clean, build and link the whole project

.PHONY: clean
clean:
	@rm -rf $(DIR_BUILD)
	@echo "[clean] generated files deleted"
	@# Help: clean all generated files

.PHONY: compile
compile: $(DIR_SRC_SUBSYSTEMS)

.PHONY: $(DIR_SRC_SUBSYSTEMS)
$(DIR_SRC_SUBSYSTEMS):
	@$(MAKE) $(MAKE_FLAGS) --directory=$@

.PHONY: build
build: clean compile linker
	@echo "[build] compiling sources"
	@# Help: build the whole project

.PHONY: linker
linker:
	@echo "[linker] linking all object files"
	@$(CC) $(CFLAGS) \
		-shared \
		-o $(DIR_BUILD_LIB)/libmidoridb.so \
		$(shell find $(DIR_BUILD_LIB)/ -type f -name "*.o")

.PHONY: test
test: $(DIR_SRC_TESTS_SUBSYSTEMS)

.PHONY: $(DIR_SRC_TESTS_SUBSYSTEMS)
$(DIR_SRC_TESTS_SUBSYSTEMS):
	@$(MAKE) $(MAKE_FLAGS) --directory=$@

.PHONY: cscope
cscope: 
	@rm -rf cscope*
	@find . -name '\.pc' -prune -o -name '*\.[ch]' -print > cscope.files
	@cscope -b -q -f cscope.out

MAKEOVERRIDES =
help:
	@printf "%-20s %s\n" "Target" "Description"
	@printf "%-20s %s\n" "------" "-----------"
	@make -pqR : 2>/dev/null \
        	| awk -v RS= -F: '/^# File/,/^# Finished Make data base/ {if ($$1 !~ "^[#.]") {print $$1}}' \
        	| sort \
        	| egrep -v -e '^[^[:alnum:]]' -e '^$@$$' \
        	| xargs -I _ sh -c 'printf "%-20s " _; make _ -nB | (grep -i "^# Help:" || echo "") | tail -1 | sed "s/^# Help: //g"'
