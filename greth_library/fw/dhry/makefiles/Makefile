include util.mak

TOP_DIR=../../
OBJ_DIR = $(TOP_DIR)dhry/makefiles/obj
ELF_DIR = $(TOP_DIR)dhry/makefiles/bin

CC=riscv64-unknown-elf-gcc
CPP=riscv64-unknown-elf-gcc
OBJDUMP=riscv64-unknown-elf-objdump

CFLAGS= -c -fPIC -g -O2
LDFLAGS=-static -T app.ld -nostartfiles
INCL_KEY=-I
DIR_KEY=-B

# include sub-folders list
INCL_PATH=\
	$(TOP_DIR)common \
	$(TOP_DIR)dhry/src

# source files directories list:
SRC_PATH = \
	$(TOP_DIR)dhry/src

LIB_NAMES = \
	gcc \
	stdc++ \
	c \
	m \

VPATH = $(SRC_PATH)

SOURCES = \
	main \
	uart \
	dhry

OBJ_FILES =  $(addprefix $(OBJ_DIR)/,$(addsuffix .o,$(SOURCES)))
EXECUTABLE =  $(addprefix $(ELF_DIR)/,dhry)
DUMPFILE = $(EXECUTABLE).dump

all: example
	@$(ECHO) "    All done.\n"

example: $(EXECUTABLE)

$(EXECUTABLE): $(OBJ_FILES)
	@echo $(CPP) $(LDFLAGS) $(OBJ_FILES) -o $@ $(addprefix -l,$(LIB_NAMES))
	@mkdir -p $(ELF_DIR)
	$(CPP) $(LDFLAGS) $(OBJ_FILES) -o $@ $(addprefix -l,$(LIB_NAMES))
	@$(ECHO) "\n  dhry has been built successfully.\n"

$(addprefix $(OBJ_DIR)/,%.o): %.cpp
	@echo $(CPP) $(CFLAGS) $(addprefix $(INCL_KEY),$(INCL_PATH)) $< -o $@
	@mkdir -p $(OBJ_DIR)
	$(CPP) $(CFLAGS) $(addprefix $(INCL_KEY),$(INCL_PATH)) $< -o $@

$(addprefix $(OBJ_DIR)/,%.o): %.c
	@echo $(CC) $(CFLAGS) $(addprefix $(INCL_KEY),$(INCL_PATH)) $< -o $@
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) $(addprefix $(INCL_KEY),$(INCL_PATH)) $< -o $@

$(addprefix $(OBJ_DIR)/,%.o): %.S
	@echo $(CC) $(CFLAGS) -D__ASSEMBLY__=1 $(addprefix $(INCL_KEY),$(INCL_PATH)) $< -o $@
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -D__ASSEMBLY__=1 $(addprefix $(INCL_KEY),$(INCL_PATH)) $< -o $@


