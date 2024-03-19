CC = gcc
CFLAGS = -Wall -Wextra -Werror -pthread -std=gnu11 -Wno-deprecated-declarations -Wno-unused-parameter -Wno-unused-variable -O2 -msse4.2
DFLAGS = -g -fsanitize=address -fsanitize=leak -fsanitize=undefined -fno-omit-frame-pointer
DEPFLAGS = -MMD -MP
INC_DIR = ./inc
BUILD_DIR = ./build
SRC_DIR = src
CFLAGS += -I$(INC_DIR)
SRC =	eh_malloc.c \
		slab_allocator.c \
		border_tasgs_allocator.c \

SRC := $(addprefix $(SRC_DIR)/,$(SRC))
OBJ = $(SRC:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
DEP = $(OBJ:%.o=%.d)
LIBNAME = eh_malloc.so
TARGET_LIB = $(BUILD_DIR)/$(LIBNAME)
BUILD_MODE ?= Release
TEST_DIR = ./test
TEST_OBJ = $(TEST_DIR)/test.o
BIN_DIR = ./bin
LDFLAGS = ./build/eh_malloc.so
LDFLAGS += -L./build
LD_PRELOAD =

ifeq ($(BUILD_MODE),Debug)
    CFLAGS += $(DFLAGS)
	LDFLAGS += -fsanitize=address -fsanitize=leak -fsanitize=undefined
	LD_PRELOAD += /usr/lib/x86_64-linux-gnu/libasan.so.6.0.0
endif

all: $(BUILD_DIR) $(TARGET_LIB)

# Test with list data structure
$(TEST_DIR)/list_test.o: $(TEST_DIR)/list_test.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

list_test: all $(BIN_DIR) $(TEST_DIR)/list_test.o $(TARGET_LIB)
	gcc -o $(BIN_DIR)/list_test $(TEST_DIR)/list_test.o $(TARGET_LIB) $(LDFLAGS)

run_list_test: list_test
	LD_PRELOAD=$(LD_PRELOAD) $(BIN_DIR)/list_test

# Common test
$(TEST_OBJ): $(TEST_DIR)/test.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

test: all $(TEST_OBJ) $(TARGET_LIB) $(BIN_DIR)
	gcc -o $(BIN_DIR)/test $(TEST_OBJ) $(TARGET_LIB) $(LDFLAGS)

run_test: test
	LD_PRELOAD=$(LD_PRELOAD) $(BIN_DIR)/test

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(TARGET_LIB): $(OBJ)
	$(CC) $(CFLAGS) -shared -o $@ $^
	cp $@ ./

-include $(DEP)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEPFLAGS) -fPIC -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(LIBNAME) $(TEST_DIR)/*.o $(BIN_DIR)

fclean: clean

re: fclean all

.PHONY: all clean fclean re