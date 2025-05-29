.PHONY: test clean
.SUFFIXES:

# --- Paths / files
SOURCE.d := source/
OBJECT.d := object/
LIB.d	:= library/

SOURCE := main.c opts.c generator.c dictate.c
OBJECT := ${SOURCE}
OBJECT := $(subst .cpp,.o,${OBJECT})
OBJECT := $(subst .c,.o,${OBJECT})

GENSOURCE := jeger.yy.c

vpath %.o ${OBJECT.d}
vpath %.c ${SOURCE.d}
vpath %.c ${LIB.d}
vpath %.l ${SOURCE.d}
vpath %.y ${SOURCE.d}
vpath %.cpp ${SOURCE.d}
vpath %.yy.c ${OBJECT.d}

OUT := jeger

# --- Tools/Flags
ifeq (${DEBUG}, 1)
  LFLAGS   += --debug --trace
  YFLAGS   += --debug

  CPPFLAGS += -DDEBUG

  CFLAGS.D += -Wall -Wextra -Wpedantic
  CFLAGS.D += -O0 -ggdb -fno-inline
  CFLAGS.D += -fsanitize=address,undefined
  CFLAGS   += ${CFLAGS.D}
  CXXFLAGS += ${CFLAGS.D}
else
  CFLAGS += -O3 -flto=auto -fno-stack-protector
endif

CPPFLAGS += -I${SOURCE.d} -I${OBJECT.d} -I${LIB.d}
LDLIBS := -ldictate

# --- Rule Section ---
all: ${OUT}

${OUT}: ${GENSOURCE} ${OBJECT}
	${LINK.cpp} -o $@ $(addprefix ${OBJECT.d}/,${OBJECT} ${GENSOURCE}) ${LDLIBS}

%.o: %.c
	${COMPILE.c} -o ${OBJECT.d}/$@ $<

%.o: %.cpp
	${COMPILE.cpp} -o ${OBJECT.d}/$@ $<

%.yy.c: %.l
	flex -o ${OBJECT.d}/$@ --header=${OBJECT.d}/$(subst .c,.h,$@) $? 

%.yy.o: %.yy.c
	${COMPILE.c} -o ${OBJECT.d}/$@ ${OBJECT.d}/$<

test:
	./${OUT} -d -t -o jeger_bf.yy.c test/brainfuck.l 2>&1 | tool/hl_table
	cat jeger_bf.yy.c | tool/hl_table
	gcc -o bf.out jeger_bf.yy.c -ggdb
	./bf.out test/hw.bf

clean:
	-${RM} $(or ${OBJECT.d},#)/*
	-${RM} ${OUT}
