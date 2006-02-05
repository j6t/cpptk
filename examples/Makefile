COMPILER = g++
OPTIONS = -Wall -Wno-long-long -pedantic
INCLUDES = -I.. -I/usr/local/include/tcl8.4 -I/usr/local/include/tk8.4 -I/usr/X11R6/include -I/usr/local/include/boost_1_33_0
LIBDIRS = -L/usr/local/lib
LIBS = -ltcl84 -ltk84

ALL_OPTIONS = ${OPTIONS} ${INCLUDES} ${LIBDIRS} ${LIBS}


ALL_OUTS = example0 example1 example2 example3 example4 example5 example6

all : ${ALL_OUTS}

example0 : example0.cc cpptk.o cpptkbase.o
	${COMPILER} example0.cc cpptk.o cpptkbase.o -o $@ ${ALL_OPTIONS}

example1 : example1.cc cpptk.o cpptkbase.o
	${COMPILER} example1.cc cpptk.o cpptkbase.o -o $@ ${ALL_OPTIONS}

example2 : example2.cc cpptk.o cpptkbase.o
	${COMPILER} example2.cc cpptk.o cpptkbase.o -o $@ ${ALL_OPTIONS}

example3 : example3.cc cpptk.o cpptkbase.o
	${COMPILER} example3.cc cpptk.o cpptkbase.o -o $@ ${ALL_OPTIONS}

example4 : example4.cc cpptk.o cpptkbase.o
	${COMPILER} example4.cc cpptk.o cpptkbase.o -o $@ ${ALL_OPTIONS}

example5 : example5.cc cpptk.o cpptkbase.o
	${COMPILER} example5.cc cpptk.o cpptkbase.o -o $@ ${ALL_OPTIONS}

example6 : example6.cc cpptk.o cpptkbase.o
	${COMPILER} example6.cc cpptk.o cpptkbase.o -o $@ ${ALL_OPTIONS}

cpptk.o : ../cpptk.cc
	${COMPILER} $? -o $@ -c ${OPTIONS} ${INCLUDES}

cpptkbase.o : ../base/cpptkbase.cc
	${COMPILER} $? -o $@ -c ${OPTIONS} ${INCLUDES}


clean :
	rm -f ${ALL_OUTS} cpptk.o cpptkbase.o
