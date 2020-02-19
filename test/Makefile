COMPILER = g++-7 -std=c++17 -Wno-parentheses
CXXFLAGS = -Wall -Wno-long-long -pedantic
TESTFLAGS = -DCPPTK_DUMP_COMMANDS -DCPPTK_DONT_EVALUATE
INCLUDES = -I/usr/include/tcl8.6 -I/usr/include/tk8.6
LIBDIRS =
LIBS = -ltcl8.6 -ltk8.6

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
