.PHONY: test

SOURCE.d := source/
OBJECT.d := object/

CFLAGS   += -Wall -Wpedantic -I${SOURCE.d}/
CPPFLAGS += ${CFLAGS}

OUTPUT := jeger

${OUTPUT}: object/main.o object/generator.o object/jeger.yy.o
	${LINK.cpp} -o ${OUTPUT} $?

test:
	./${OUTPUT} source/jeger.l

clean:
	-rm ${OBJECT.d}/*.o
	-rm ${OUTPUT}

object/%.yy.cpp: source/%.l
	flex -o $@ $<

object/%.o: source/%.c
	${COMPILE.c} $< -o $@

object/%.o: source/%.cpp
	${COMPILE.cpp} $< -o $@
