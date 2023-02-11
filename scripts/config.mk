#--------------------
# Project directories
#--------------------
DIR_SRC     	:= $(DIR_ROOT)/src
DIR_BUILD   	:= $(DIR_ROOT)/build
DIR_SCRIPTS	:= $(DIR_ROOT)/scripts
DIR_INCLUDE	:= $(DIR_ROOT)/include

#-------------------
# Tool configuration
#-------------------
CC		:= gcc

CCFLAGS     	:= -std=gnu99 -I$(DIR_INCLUDE) -g \
		       -O2 \
		       -fPIE \
        	       -masm=intel \
		       -Wall -Wextra -Wpedantic \
		       -D_FORTIFY_SOURCE=3 \
		       -fsanitize=bounds \
		       -fsanitize-undefined-trap-on-error \
		       -fstack-protector-strong \
		       -Wvla \
		       -Wimplicit-fallthrough \
		       -ftrivial-auto-var-init=zero
			

LD          	:= ld

MAKE_FLAGS  	:= --quiet --no-print-directory

