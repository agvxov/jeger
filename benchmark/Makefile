#CPPFLAGS := -DDEBUG
#CXXFLAGS := -O0 -ggdb
CXXFLAGS := -O2
BUILD.cpp := ${CXX} ${CXXFLAGS} ${CPPFLAGS}

build: jeger gnu

jeger: ../source/vector.c ../source/jeger.c jeger_racer.cpp
	g++ -shared ../source/vector.c -o vector.so
	g++ -shared ../source/jeger.c -o jeger.so
	${BUILD.cpp} -I../source/ jeger_racer.cpp vector.so jeger.so -o jeger_racer.out

gnu: gnu_racer.cpp
	${BUILD.cpp} gnu_racer.cpp -o gnu_racer.out

test: build
	export LD_LIBRARY_PATH=$$(realpath .):$$LD_LIBRARY_PATH
	perf stat -r 10000 ./gnu_racer.out
	perf stat -r 10000 ./jeger_racer.out

clean:
	rm *.so *.out

.PHONY: test
