test: src/test_ecc.c
	cc -I./src -MMD -MP -c src/test_ecc.c -o build/test_ecc.o
	cc build/test_ecc.o -o build/test_ecc
	./build/test_ecc
