## Process this file with automake to produce Makefile.in

exampledir=${datadir}/@PACKAGE@/Examples/CBasic

if ENABLE_EXAMPLES
if ENABLE_CBASIC

example_PROGRAMS = CBasicTest

CBasicTest_SOURCES = CBasicTest.c
CBasicTest_LDADD = -lcunit -lcunitexamples
CBasicTest_LDFLAGS = -L.. -lm
INCLUDES = -I..

if ENABLE_CURSES
CBasicTest_LDADD += -l$(CURSES_LIB)
endif

example_DATA = README

endif
endif