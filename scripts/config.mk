#--------------------
# Project directories
#--------------------
DIR_SRC     	:= $(DIR_ROOT)/src
DIR_SRC_TESTS  	:= $(DIR_ROOT)/tests
DIR_BUILD   	:= $(DIR_ROOT)/build
DIR_BUILD_LIB  	:= $(DIR_BUILD)/lib
DIR_BUILD_TESTS	:= $(DIR_BUILD)/tests
DIR_SCRIPTS	:= $(DIR_ROOT)/scripts
DIR_INCLUDE	:= $(DIR_ROOT)/include

#-------------------
# Tool configuration
#-------------------
CC		:= gcc

CCFLAGS     	:= -std=gnu99 -I$(DIR_INCLUDE) -g \
		       -O2 \
		       -fpic \
        	       -masm=intel \
		       -Wall -Wextra -Wpedantic \
		       -D_FORTIFY_SOURCE=3 \
		       -fsanitize=bounds \
		       -fsanitize-undefined-trap-on-error \
		       -fstack-protector-strong \
		       -Wvla \
		       -Wimplicit-fallthrough \
		       -ftrivial-auto-var-init=zero

MAKE_FLAGS  	:= --quiet --no-print-directory

