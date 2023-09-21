include ../Make.rule

BIN_PATH = ../../publish/service/
CPP_FILES +=$(wildcard *.cpp)
C_FILES +=$(wildcard *.c)
H_FILES += $(wildcard *.h)

INC += -I$(SKYNET_ROOT)skynet-src/
LOCAL_LIBS = $(LIBS) -lprotobuf --share 
CFLAGS +=
CXXFLAGS +=

OBJ_DIR = ./obj/
OBJ := $(patsubst %.cpp, $(OBJ_DIR)%.o, $(CPP_FILES)) $(patsubst %.c, $(OBJ_DIR)%.o, $(C_FILES))

BIN = mail.so
TARGET = $(BIN_PATH)$(BIN)

$(TARGET):$(OBJ) $(H_FILES)
	@echo link...  $(TARGET)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(INC) $(LOCAL_LIBS)
	
$(OBJ_DIR)%.o:%.c
	$(CC) $(CFLAGS) -c -o $@ $< $(INC)
	
$(OBJ_DIR)%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $< $(INC)

clean:
	rm -f $(OBJ_DIR)* $(TARGET)