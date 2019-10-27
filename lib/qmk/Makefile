ifndef VERBOSE
.SILENT:
endif

CC = gcc
TARGET = ../libqmk.a

CFLAGS = -fPIC -O0 -g -Wall -c
INC = -I../
BUILD_DIR = .build

C_FILES = $(shell find . -type f -name '*.c')

$(TARGET): $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_FILES))
	echo "* Creating library"
	@ar -c -r -o $@ $^

$(BUILD_DIR)/%.o: %.c $(BUILD_DIR)
	echo "* Making $<"
	@mkdir -p $(BUILD_DIR)/$(*D)
	$(CC) -c $(INC) $(CFLAGS) -o $@  $<

$(BUILD_DIR):
	@mkdir -p $@

clean:
	rm -fdr $(BUILD_DIR) $(TARGET)
