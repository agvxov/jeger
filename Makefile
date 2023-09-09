CXXFLAGS := -fuse-ld=mold -ggdb -Wall -Wextra -Wpedantic
OUT := regtest

main:
	${CXX} ${CXXFLAGS} ${CPPFLAGS} source/main.cpp source/vector.c source/jeger.c -o ${OUT}

run:
	${OUT}

test: run
