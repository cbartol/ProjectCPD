SRC = src
BIN = bin
GEN_TESTS = test/generated

all: clean create serial

debug: clean create
	gcc -Wall -o $(BIN)/wolves-squirrels-serial $(SRC)/wolves-squirrels-serial.c -DPROJ_DEBUG=1 -g

create:
	mkdir -p $(BIN)
	
serial:
	gcc -Wall -o $(BIN)/wolves-squirrels-serial $(SRC)/wolves-squirrels-serial.c

omp:
	gcc -ansi -pedantic -Wall -o $(BIN)/wolves-squirrels-omp $(SRC)/wolves-squirrels-omp.c -fopenmp

mpi:
	gcc -ansi -pedantic -Wall -o $(BIN)/wolves-squirrels-mpi $(SRC)/wolves-squirrels-mpi.c

clean:
	rm -rf $(BIN) 2> /dev/null
	rm -rf $(GEN_TESTS) 2> /dev/null

tests:
	./gen-test.sh
