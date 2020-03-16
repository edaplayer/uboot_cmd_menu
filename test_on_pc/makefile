# CXX:=g++ -std=c++11
CC:=gcc
SRCS:= 		$(wildcard *.c)
OBJ:= 		$(patsubst %.c, %.o, ${SRCS})
PREFILE:= 	$(patsubst %.o, %.i, ${OBJ})
ASMFILE:= 	$(patsubst %.o, %.s, ${OBJ})
SRCS:= 		$(wildcard *.c)

TARGET:=main
all: ${TARGET}
.PHONY: clean distclean

$(PREFILE):%.i:%.c
	$(info Preprocess: build main.i)
	@$(CC) -E -o $@ $<

$(ASMFILE):%.s:%.i
	$(info Compile: build main.s)
	@$(CC) -S -o $@ $<

$(OBJ):%.o:%.s
	$(info Assemble: build main.o)
	@$(CC) -c -o $@ $<
	@objdump -DrwC -Mintel $@ > $(patsubst %.o,%.o.asm,$@)

$(TARGET):$(OBJ)
	$(info Link: build main)
	@$(CC) -o $@ $^ -Wl,-Map=gcc.map
	@objdump -D $@ > $(patsubst %,%.asm,$@)

clean:
	rm -f *.o *.out *.s *.i *.asm *.map ${OBJ} ${TARGET}

distclean : clean
	rm -f *.d
