test: test/test_ecc.c
	cc -I./ecc -MMD -MP -c test/test_ecc.c -o build/test_ecc.o
	cc build/test_ecc.o -o build/test_ecc
	./build/test_ecc

benchmark: test/benchmark_ecc.c
	cc -I./ecc -MMD -MP -c test/benchmark_ecc.c -o build/benchmark_ecc.o
	cc build/benchmark_ecc.o -o build/benchmark_ecc
	./build/benchmark_ecc
