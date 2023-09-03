include ../Make.rule

BIN_PATH = ../../publish/service/
CPP_FILES +=$(wildcard *.cc)
H_FILES += $(wildcard *.h)

INC := -I$(PB34_DIR)/include
LIBS := --share \
		-L${LIBPB_DIR} \
		-lprotobuf #-Wl,-soname,libpb.so.1
CXXFLAGS := -fPIC -Wall -O0 -fdiagnostics-color=auto -std=c++11

OBJ_DIR = ./obj/
OBJ := $(patsubst %.cc, $(OBJ_DIR)%.o, $(CPP_FILES))

BIN = libpb.so
STATIC_BIN = libpb_s.a
TARGET = $(BIN_PATH)$(BIN)

static :  $(STATIC_BIN)

$(TARGET):$(OBJ) 
	@echo link...  $(TARGET)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(INC) $(LIBS)
	


$(STATIC_BIN):$(OBJ)
	ar -rc -o $@ $(OBJ) 

$(OBJ_DIR)%.o:%.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $< $(INC)

clean:
	rm -f $(OBJ_DIR)* $(TARGET) $(STATIC_BIN)
