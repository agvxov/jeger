DEBUG := 0

BUILD.cpp := ${CXX} ${CXXFLAGS} ${CPPFLAGS}

.ONESHELL:

test: build
	for $$i i *.out;
		do time $$i
	done

build: jeger gnu

jeger: ../source/vector.c ../source/jeger.c jeger_racer.cpp
	gcc -shared ../source/vector.c -o vector.so
	gcc -shared ../source/jeger.c -o jeger.so
	${BUILD.cpp} -I../source/ vector.so jeger.so jeger_racer.cpp -o jeger_racer.out

gnu: gnu_racer.cpp
	${BUILD.cpp} gnu_racer.cpp -o gnu_racer.out

clean:
	rm *.so *.out
