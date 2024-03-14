CC = gcc
CFLAGS = -Wall -Wextra -Werror -pthread -std=gnu11
DFLAGS = -g -fsanitize=address -fsanitize=undefined
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
BIN_DIR = ./bin
LDFLAGS = -leh_malloc
LDFLAGS += -L./

ifeq ($(BUILD_MODE),Debug)
    CFLAGS += $(DFLAGS)
endif

all: $(BUILD_DIR) $(TARGET_LIB)

$(TEST_DIR)/list_test.o: $(TEST_DIR)/list_test.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

list_test: $(TEST_DIR)/list_test.o $(TARGET_LIB)
	$(CC) -o $(BIN_DIR)/$@ $^ $(CFLAGS) $(LDFLAGS)

run_list_test: list_test
	$(BIN_DIR)/list_test

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(dir $(OBJ))

$(TARGET_LIB): $(OBJ)
	$(CC) $(CFLAGS) -shared -o $@ $^
	cp $@ ./

-include $(DEP)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEPFLAGS) -fPIC -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(LIBNAME) $(TEST_DIR)/*.o $(BIN_DIR)/list_test

fclean: clean

re: fclean all

.PHONY: all clean fclean re