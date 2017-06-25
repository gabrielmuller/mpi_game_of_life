CC=mpicc

FLAGS=-O3 -Wno-unused-result
LDFLAGS=-lpthread
#DEBUG=-DDEBUG
#RESULT=-DRESULT
NTHREADS=4
INPUT=input-little.in

all:
	clear
	clear
	make -s clean
	make -s gol
	make run

gol: gol.c
	$(CC) $(DEBUG) $(RESULT) $(FLAGS) gol_mpi.c -o gol

clean:
	rm -rf gol

run:
	mpirun  -np $(NTHREADS) ./gol $(INPUT)
