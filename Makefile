CXXFLAGS := -fuse-ld=mold -ggdb
OUT := regtest

main:
	g++ ${CXXFLAGS} source/main.cpp source/vector.c source/regex.c -o ${OUT}

run:
	${OUT}

test: run
