SRC_DIR     = .
QPID_DIR    = /path/to/qpid/repo/trunk/qpid
SCHEMA_FILE = $(SRC_DIR)/schema.xml
GEN_DIR     = $(SRC_DIR)/gen
OUT_FILE    = $(SRC_DIR)/nodereporter

CC           = gcc
LIB_DIR      = $(QPID_DIR)/cpp/src/.libs
DBUS         = -I/usr/include/dbus-1.0 -I/usr/lib64/dbus-1.0/include -L/lib64 -ldbus-1
CC_INCLUDES  = -I$(SRC_DIR) -I$(QPID_DIR)/cpp/src -I$(QPID_DIR)/cpp/src/gen -I$(GEN_DIR) $(DBUS)
CC_FLAGS     = -g -O3 
LD_FLAGS     = -lqmfagent -lqmfcommon -lboost_regex -L$(LIB_DIR) -ldbus-1 -lhal
SPEC_DIR     = $(QPID_DIR)/specs
MGEN_DIR     = $(QPID_DIR)/cpp/managementgen
MGEN         = $(MGEN_DIR)/qmf-gen

vpath %.cpp $(SRC_DIR):$(GEN_DIR)
vpath %.d   $(OBJ_DIR)
vpath %.o   $(OBJ_DIR)

cpps    = $(wildcard $(SRC_DIR)/*.cpp)
cpp    += $(wildcard $(SRC_DIR)/*.c)
cpps   += $(wildcard $(GEN_DIR)/qmf/com/redhat/nodereporter/*.cpp)
deps    = $(addsuffix .d, $(basename $(cpps)))
objects = $(addsuffix .o, $(basename $(cpps)))

.PHONY: all clean gen

#==========================================================
# Pass 0: generate source files from schema
ifeq ($(MAKELEVEL), 0)

all: gen
	@$(MAKE)

gen:
	$(MGEN) -o $(GEN_DIR)/qmf $(SCHEMA_FILE)

clean:
	rm -rf $(GEN_DIR) $(OUT_FILE) *.d *.o


#==========================================================
# Pass 1: generate dependencies
else ifeq ($(MAKELEVEL), 1)

all: $(deps)
	@$(MAKE)

%.d : %.cpp
	$(CC) -M $(CC_FLAGS) $(CC_INCLUDES) $< > $@


#==========================================================
# Pass 2: build project
else ifeq ($(MAKELEVEL), 2)

$(OUT_FILE) : $(objects)
	$(CC) -o $(OUT_FILE) $(CC_FLAGS) $(LD_FLAGS) $(objects)

include $(deps)

%.o : %.cpp
	$(CC) -c $(CC_FLAGS) $(CC_INCLUDES) -o $@ $<

endif
