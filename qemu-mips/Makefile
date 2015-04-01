include ../py/mkenv.mk
-include mpconfigport.mk

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

# include py core make definitions
include ../py/py.mk

CROSS_COMPILE = mips-sde-elf-

INC =  -I.
INC += -I..
INC += -I$(BUILD)
INC += -I../tools/tinytest/

CFLAGS_TARGET = -EL -mips32r2
CFLAGS = $(INC) -Wall -Wpointer-arith -Werror -ansi -std=gnu99 $(CFLAGS_TARGET) $(COPT) \
	 -flto -ffunction-sections -fdata-sections

#Debugging/Optimization
ifeq ($(DEBUG), 1)
CFLAGS += -g -DPENDSV_DEBUG
COPT = -O0
else
COPT += -Os -DNDEBUG
endif

LDFLAGS= -Wl,--gc-sections -T qemu-hosted.ld -Ttext 80100000 -Wl,-Map=$(@:.elf=.map)
LIBS= -lm

SRC_C = \
	main.c \

SRC_TEST_C = \
	test_main.c \

SRC_S = \

OBJ =
OBJ += $(PY_O)
OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_S:.s=.o))

OBJ_TEST =
OBJ_TEST += $(PY_O)
OBJ_TEST += $(addprefix $(BUILD)/, $(SRC_TEST_C:.c=.o))
OBJ_TEST += $(addprefix $(BUILD)/, $(SRC_S:.s=.o))
OBJ_TEST += $(BUILD)/tinytest.o

all: run

run: $(BUILD)/firmware.elf
	$(CROSS_COMPILE)qemu-system-el -machine mipssim -cpu 24Kf -monitor none -serial none -semihosting -kernel $(BUILD)/firmware.elf 

test: $(BUILD)/firmware-test.elf
	$(CROSS_COMPILE)qemu-system-el -machine mipssim -cpu 24Kf -monitor none -serial none -semihosting -kernel $(BUILD)/firmware-test.elf > $(BUILD)/console.out 
	$(Q)tail -n2 $(BUILD)/console.out
	$(Q)tail -n1 $(BUILD)/console.out | grep -q "status: 0"

.PHONY: $(BUILD)/genhdr/tests.h

$(BUILD)/test_main.o: $(BUILD)/genhdr/tests.h
$(BUILD)/genhdr/tests.h:
	$(Q)echo "Generating $@";(cd ../tests; ../tools/tinytest-codegen.py) > $@

$(BUILD)/tinytest.o:
	$(Q)$(CC) $(CFLAGS) -DNO_FORKING -o $@ -c ../tools/tinytest/tinytest.c

$(BUILD)/firmware.elf: $(OBJ)
	$(Q)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)
	$(Q)$(SIZE) $@

$(BUILD)/firmware-test.elf: $(OBJ_TEST)
	$(Q)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ_TEST) $(LIBS)
	$(Q)$(SIZE) $@

include ../py/mkrules.mk
