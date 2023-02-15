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

all: clean compile link test
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
build: clean compile link
	@echo "[build] compiling sources"
	@# Help: build the whole project

.PHONY: link
link:
	@echo "[linker] linking all object files"
	@$(CC) $(CFLAGS) \
		-shared \
		-o $(DIR_BUILD_LIB)/libmidoridb.so \
		$(shell find $(DIR_BUILD_LIB)/ -type f -name "*.o")

.PHONY: $(DIR_SRC_TESTS_SUBSYSTEMS)
$(DIR_SRC_TESTS_SUBSYSTEMS):
	@$(MAKE) $(MAKE_FLAGS) --directory=$@

.PHONY: test_compile
test_compile: $(DIR_SRC_TESTS_SUBSYSTEMS)

.PHONY: test
test: test_compile
	@echo "[test/linker] linking all object files"

	@for f in "$(DIR_BUILD_TESTS)/main/*.o"; 			\
	do								\
		$(CC) $(CCFLAGS) 					\
	        	-lcunit 					\
	        	-L$(DIR_BUILD_LIB) 				\
			-lmidoridb 					\
			-Wl,-rpath=$(DIR_BUILD_LIB)			\
	                -o $(DIR_BUILD_TESTS)/$$(basename $$f '.o') 	\
			$$f						\
                	$(shell find $(DIR_BUILD_TESTS)/ -name '*.o'	\
			  -not -path '$(DIR_BUILD_TESTS)/main/*') ; 	\
	done

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
