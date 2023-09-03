include ../Make.rule
BIN_PATH = ./

CPP_FILES +=$(wildcard *.cpp)
C_FILES +=$(wildcard *.c)
H_FILES += $(wildcard *.h)

CFLAGS +=
CXXFLAGS +=
INC += -I$(SKYNET_ROOT)skynet-src/
LIBS +=

BIN = libcommon.a

OBJ_DIR = ./obj/
OBJ := $(patsubst %.cpp, $(OBJ_DIR)%.o, $(CPP_FILES)) $(patsubst %.c, $(OBJ_DIR)%.o, $(C_FILES))

TARGET = $(BIN_PATH)$(BIN)


$(TARGET):$(OBJ) 
	ar -rc -o $@ $(OBJ)

$(OBJ_DIR)%.o:%.c
	$(CC) $(CFLAGS) -c -o $@ $< $(INC)
	
$(OBJ_DIR)%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $< $(INC)

clean:
	rm -f $(OBJ_DIR)* $(TARGET)
