#----------------------------------------------------------------------------
# MidoriDB root makefile
#----------------------------------------------------------------------------

DIR_ROOT := $(CURDIR)

include $(DIR_ROOT)/scripts/config.mk

DIR_SRC_SUBSYSTEMS := $(wildcard $(DIR_SRC)/*)

#----------------------------------------------------------------------------
# Build targets
#----------------------------------------------------------------------------

default: build

all: clean compile
	@# Help: clean and build the whole project

.PHONY: compile
compile: $(DIR_SRC_SUBSYSTEMS)

.PHONY: $(DIR_SRC_SUBSYSTEMS)
$(DIR_SRC_SUBSYSTEMS):
	@$(MAKE) $(MAKE_FLAGS) --directory=$@

.PHONY: clean
clean:
	@rm -rf $(DIR_BUILD)
	@echo "[clean] generated files deleted"
	@# Help: clean all generated files

.PHONY: build
build: all
	@echo "[build] compiling sources"
	@# Help: build the whole project

MAKEOVERRIDES =
help:
	@printf "%-20s %s\n" "Target" "Description"
	@printf "%-20s %s\n" "------" "-----------"
	@make -pqR : 2>/dev/null \
        	| awk -v RS= -F: '/^# File/,/^# Finished Make data base/ {if ($$1 !~ "^[#.]") {print $$1}}' \
        	| sort \
        	| egrep -v -e '^[^[:alnum:]]' -e '^$@$$' \
        	| xargs -I _ sh -c 'printf "%-20s " _; make _ -nB | (grep -i "^# Help:" || echo "") | tail -1 | sed "s/^# Help: //g"'
