include ../Make.rule

BIN_PATH = ../../publish/skynet/luaclib/
C_FILES +=$(wildcard *.c)
H_FILES += $(wildcard *.h)

INC := -I$(SKYNET_ROOT)skynet-src/
LIBS := --share
CFLAGS := -fPIC -g -O0 -Wall -fdiagnostics-color=auto

OBJ_DIR = ./obj/
OBJ := $(patsubst %.c, $(OBJ_DIR)%.o, $(C_FILES))


BIN = beecluster.so
TARGET = $(BIN_PATH)$(BIN)

$(TARGET):$(OBJ)
	@echo link...  $(TARGET)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(INC) $(LIBS)

$(OBJ_DIR)%.o:%.c
	$(CC) $(CFLAGS) -c -o $@ $< $(INC)

clean:
	rm -f $(OBJ_DIR)* $(TARGET)


