ifndef VERBOSE
.SILENT:
endif

CC = gcc
TARGET = ../libqmk.a

# CFLAGS = -O0 -g -Wall -c -fshort-wchar -ffreestanding -march=armv7-a -mfloat-abi=softfp -mfpu=neon
CFLAGS = -O0 -g -Wall -c -fshort-wchar -ffreestanding -mfloat-abi=softfp -mfpu=neon

INC = -Iinclude
BUILD_DIR = .build

C_FILES = $(shell find . -type f -name '*.c')

$(TARGET): $(patsubst ./%.c,$(BUILD_DIR)/%.o,$(C_FILES))
	@echo "  AR      $@"
	@rm -fdr $@ && ar rcs -o $@ $^

$(BUILD_DIR)/%.o: %.c $(BUILD_DIR)
	@echo "  CC [M]  $@"
	@mkdir -p $(BUILD_DIR)/$(*D)
	@$(CC) -c $(INC) $(CFLAGS) -o $@  $<

$(BUILD_DIR):
	@mkdir -p $@

clean:
	@echo "  CLEAN   $(BUILD_DIR)"
	@echo "  CLEAN   $(TARGET)"
	@rm -fdr $(BUILD_DIR) $(TARGET)
