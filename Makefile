MODULE_APP =state_machine

#CFLAGS='-D_EVENT_DISABLE_DEBUG_MODE -g -O1' ./configure --disable-shared

COMPILE++ = g++
COMPILECC = gcc

LINK = ar rv

SRC = ./

INCLUDE = -I./

OUTPUTOBJ = ./obj/

#自动判断32位、64位系统
SYSBYTE := $(shell uname -r | grep 'x86_64')
ifeq ($(strip $(SYSBYTE)),) 
override SYSBYTE = -m32 
#-std=gnu++0x
else
override SYSBYTE = -m64 
#-std=gnu++0x
endif 

CFLAGS = -D__LINUX__  -c -g -O1 $(SYSBYTE)
CXXFLAGS = $(CFLAGS) -std=c++14

LDFLAGS=-lpthread

OBJ = $(OUTPUTOBJ)state_machine.o


MODULE_APP:chkobjdir $(OBJ)
	$(COMPILE++) -o $(MODULE_APP)  $(OBJ) $(LDFLAGS)  

$(OUTPUTOBJ)state_machine.o:$(SRC)main.cpp
	$(COMPILE++) $(CXXFLAGS) $(SRC)main.cpp $(INCLUDE) -o $(OUTPUTOBJ)state_machine.o

clean:
	rm -rdf $(MODULE_APP)
	rm -rf $(OUTPUTOBJ)*
	
chkobjdir:
	@if test ! -d $(OUTPUTOBJ);\
	then \
		mkdir $(OUTPUTOBJ); \
	fi
