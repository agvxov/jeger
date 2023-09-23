CXXFLAGS := -fuse-ld=mold -ggdb -Wall -Wextra -Wpedantic
ifeq (${DEBUG}, 1)
	CXXFLAGS += -DDEBUG
endif

OUT := regtest

main:
	${CXX} ${CXXFLAGS} ${CPPFLAGS} source/main.cpp source/vector.c source/jeger.c -o ${OUT}

run:
	${OUT}

test: run
