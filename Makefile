.PHONY: test

SOURCE.d := source/
OBJECT.d := object/

CFLAGS   += -Wall -Wpedantic -I${SOURCE.d}/
CPPFLAGS += ${CFLAGS}

OUTPUT := jeger

${OUTPUT}: object/main.o object/generator.o object/jeger.yy.o
	${LINK.cpp} -o ${OUTPUT} $^ 

test:
	./${OUTPUT} test/brainfuck.l 2>&1 | perl -pe "s/(\[.{1,4}\] = 128)/\x1b[90m\1\x1b[0m/g"
	cat jeger.yy.c

clean:
	-rm ${OBJECT.d}/*.o
	-rm ${OUTPUT}

object/%.yy.cpp: source/%.l
	flex -o $@ $<

object/%.o: source/%.c
	${COMPILE.c} $< -o $@

object/%.o: source/%.cpp
	${COMPILE.cpp} $< -o $@
