CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=gnu11
DFLAGS = -g -fsanitize=address -fsanitize=undefined
RFLAGS = -O3
DEPFLAGS = -MMD -MP
INC_DIR = ./inc
BUILD_DIR = ./build
SRC_DIR = src
CFLAGS += -I$(INC_DIR)
SRC = eh_malloc.c
SRC := $(addprefix $(SRC_DIR)/,$(SRC))
OBJ = $(SRC:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
DEP = $(OBJ:%.o=%.d)
LIBNAME = eh_malloc.so
TARGET_LIB = $(BUILD_DIR)/$(LIBNAME)
BUILD_MODE ?= Release

ifeq ($(BUILD_MODE),Debug)
    CFLAGS += $(DFLAGS)
else ifeq ($(BUILD_MODE),Release)
    CFLAGS += $(RFLAGS)
endif

all: $(BUILD_DIR) $(TARGET_LIB)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(dir $(OBJ))

$(TARGET_LIB): $(OBJ)
	$(CC) $(CFLAGS) -shared -o $@ $^
	cp $@ ./

-include $(DEP)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@) # Создание директории для объектного файла, если она еще не существует
	$(CC) $(CFLAGS) $(DEPFLAGS) -fPIC -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(LIBNAME)

fclean: clean

re: fclean all

.PHONY: all clean fclean re