#--------------------
# Project directories
#--------------------
DIR_SRC     	:= $(DIR_ROOT)/src
DIR_SRC_TESTS  	:= $(DIR_ROOT)/tests
DIR_BUILD   	:= $(DIR_ROOT)/build
DIR_BUILD_LIB  	:= $(DIR_BUILD)/lib
DIR_BUILD_TESTS	:= $(DIR_BUILD)/tests
DIR_SCRIPTS		:= $(DIR_ROOT)/scripts
DIR_INCLUDE		:= $(DIR_ROOT)/include

#-------------------
# Tool configuration
#-------------------

ifeq ($(OS),Windows_NT)
	DETECTED_OS := Windows
else
	DETECTED_OS := $(shell sh -c 'uname 2>/dev/null || echo Unknown')
endif

ifeq ($(DETECTED_OS),Linux)
	CC				:= gcc
	LIB_DYN_NAME	:= libmidoridb.so
else ifeq ($(DETECTED_OS),Darwin)
	CC				:= clang
	LIB_DYN_NAME	:= libmidoridb.dylib
else
	$(error MidoriDB doesn't support '$(DETECTED_OS)' OS yet )
endif


CCFLAGS			:= -std=gnu11 -I$(DIR_INCLUDE) \
					-lm \
					-g \
					-O2 \
					-fpic \
					-masm=intel \
					-Wall -Wextra -Wpedantic \
					-D_FORTIFY_SOURCE=3 \
			       	-fsanitize=bounds \
			       	-fsanitize-undefined-trap-on-error \
			       	-fstack-protector-strong \
			       	-Wvla \
			       	-Wimplicit-fallthrough 

MAKE_FLAGS  	:= --quiet --no-print-directory
