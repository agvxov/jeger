.PHONY: test

SOURCE.d := source/
OBJECT.d := object/

CFLAGS   += -Wall -Wpedantic -I${SOURCE.d}/

ifeq (${DEBUG}, 1)
  LFLAGS   += --debug --trace
  CFLAGS   += -O0 -ggdb -fno-inline
  CPPFLAGS += -DDEBUG=1
endif

CXXFLAGS += ${CFLAGS} -std=gnu++20

OUTPUT := jeger

${OUTPUT}: object/main.o object/generator.o object/jeger.yy.o
	${LINK.cpp} -o ${OUTPUT} $^ 

test:
	./${OUTPUT} test/brainfuck.l 2>&1 | perl -pe "s/(\[.{1,4}\] = 128)/\x1b[90m\1\x1b[0m/g"
	cat jeger.yy.c
	gcc jeger.yy.c

clean:
	-rm ${OBJECT.d}/*.yy.*
	-rm ${OBJECT.d}/*.o
	-rm ${OUTPUT}

object/%.yy.cpp: source/%.l
	flex ${LFLAGS} -o $@ $<

object/%.o: source/%.c
	${COMPILE.c} $< -o $@

object/%.o: source/%.cpp
	${COMPILE.cpp} $< -o $@
