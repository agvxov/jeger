.PHONY: test

SOURCE.d := source/
OBJECT.d := object/

CFLAGS   += -Wall -Wpedantic -I${SOURCE.d}/
CPPFLAGS += ${CFLAGS}

OUTPUT := jeger

main: object/main.o object/generator.o object/jeger.yy.o
	${LINK.cxx}

object/%.yy.cpp: source/%.l
	flex -o $@ $<

object/%.o: source/%.c
	${COMPILE.c} $< -o $@

object/%.o: source/%.cpp
	${COMPILE.cxx} $< -o $@
