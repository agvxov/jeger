.PHONY: test

#main:
#	bake main_generator.c
#	# im such a fucking genius
#	./main_generator.out 2>&1 | perl -pe "s/(\[.{1,4}\] = 128)/\x1b[90m\1\x1b[0m/g"
#
#
#test:
#	./main_generator.out > generated.h 2> /dev/null
#	bake main.c
#	./main.out

main:
	flex --debug -o scanner_scanner.yy.c source/scanner_scanner.l
	g++ -D SCANNER_MAIN -o scanner_scanner.out scanner_scanner.c
	./scanner_scanner.out source/scanner_scanner.l
