COMPILER = g++
CXXFLAGS = -Wall -Wno-long-long -pedantic
TESTFLAGS = -DCPPTK_DUMP_COMMANDS -DCPPTK_DONT_EVALUATE
INCLUDES = -I/usr/local/include/tcl8.4 -I/usr/local/include/tk8.4 -I/usr/X11R6/include -I/usr/local/include/boost_1_33_0
LIBDIRS = -L/usr/local/lib
LIBS = -ltcl84 -ltk84

OTHERFILES = ../cpptk.h ../cpptkoptions.x ../cpptkconstants.x \
          ../base/cpptkbase.h ../base/cpptkbase.cc

all : build dotest

build : test test2

test : test.cc ../cpptk.cc ../base/cpptkbase.cc ${OTHERFILES}
	${COMPILER} test.cc ../cpptk.cc ../base/cpptkbase.cc -o $@ ${CXXFLAGS} ${TESTFLAGS} ${INCLUDES} ${LIBDIRS} ${LIBS}

test2 : test2.cc ../cpptk.cc ../base/cpptkbase.cc ${OTHERFILES}
	${COMPILER} test2.cc ../cpptk.cc ../base/cpptkbase.cc -o $@ ${CXXFLAGS} ${INCLUDES} ${LIBDIRS} ${LIBS}


dotest :
	./test
	./test2


clean :
	rm -rf test test2
