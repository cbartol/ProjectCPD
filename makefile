SRC = src
BIN = bin

all: clean create serial

create:
	mkdir -p $(BIN)
	
serial:
	gcc -Wall -o $(BIN)/wolves-squirrels-serial $(SRC)/wolves-squirrels-serial.c

omp:
	gcc -ansi -pedantic -Wall -o $(BIN)/wolves-squirrels-omp $(SRC)/wolves-squirrels-omp.c -fopenmp

mpi:
	gcc -ansi -pedantic -Wall -o $(BIN)/wolves-squirrels-mpi $(SRC)/wolves-squirrels-mpi.c

clean:
	rm -r $(BIN) 2> /dev/null

tests:
	test-gen.sh
