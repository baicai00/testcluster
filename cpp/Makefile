include Make.rule

LIB_DIRS = pb\
		common\

CODE_DIRS = log \
		shop \

LIB_PATH = $(foreach path, $(LIB_DIRS), ${SRC_PATH}/$(path))
CODE_PATH = $(foreach path, $(CODE_DIRS), ${SRC_PATH}/$(path))

.PHONY : all src lib

all: src

src : lib
	$(foreach src, $(CODE_PATH), $(MAKE) -C $(src) && ) true

lib :
	$(foreach src, $(LIB_PATH), $(MAKE) -C $(src) && ) true

clean:
	$(foreach src, $(LIB_PATH), $(MAKE) -C $(src) clean && ) true
	$(foreach src, $(CODE_PATH), $(MAKE) -C $(src) clean && ) true