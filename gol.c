/*
* The Game of Life
*
* a cell is born, if it has exactly three neighbours
* a cell dies of loneliness, if it has less than two neighbours
* a cell dies of overcrowding, if it has more than three neighbours
* a cell survives to the next generation, if it does not die of loneliness
* or overcrowding
*
* In this version, a 2D array of ints is used.  A 1 cell is on, a 0 cell is off.
* The game plays a number of  (given by the input), printing to the screen each time.  'x' printed
* means on, space means off.
*
*/
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
//variáveis globais
typedef unsigned char cell_t;
int rank;

cell_t ** allocate_board (int size) {
  cell_t ** board = (cell_t **) malloc(sizeof(cell_t*)*size);
  int	i;
  for (i=0; i<size; i++)
  board[i] = (cell_t *) malloc(sizeof(cell_t)*size);
  return board;
}

void free_board (cell_t ** board, int size) {
  int     i;
  for (i=0; i<size; i++)
  free(board[i]);
  free(board);
}


/* return the number of on cells adjacent to the i,j cell */
int adjacent_to (cell_t ** board, int size, int i, int j) {
  int	k, l, count=0;

  int sk = (i>0) ? i-1 : i;
  int ek = (i+1 < size) ? i+1 : i;
  int sl = (j>0) ? j-1 : j;
  int el = (j+1 < size) ? j+1 : j;

  for (k=sk; k<=ek; k++)
  for (l=sl; l<=el; l++)
  count+=board[k][l];
  count-=board[i][j];

  return count;
}

/* print the life board */
void print (cell_t ** board, int size) {
  int	i, j;
  /* for each row */
  for (j=0; j<size; j++) {
    /* print each column position... */
    for (i=0; i<size; i++)
    printf ("%c", board[i][j] ? 'x' : ' ');
    /* followed by a carriage return */
    printf ("\n");
  }
}

void play (cell_t ** board, int sizeX, int sizeY) {
  int	i, j, a;
  cell_t ** prev;
  memcpy(prev, board);
  /* for each cell, apply the rules of Life */
  for (i=0; i<sizeY; i++)
  for (j=0; j<sizeX; j++) {
    a = adjacent_to (prev, size, i, j);
    if (a == 2) board[i][j] = prev[i][j];
    if (a == 3) board[i][j] = 1;
    if (a < 2) board[i][j] = 0;
    if (a > 3) board[i][j] = 0;
  }
  memcpy(board, prev);
  free(prev);
}

void slave () {
  int size;

  //recebe largura do tabuleiro
  MPI_Recv(&size, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

  int my_lines;

  //recebe quantas linhas essa thread calcula
  MPI_Recv(&my_lines, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

  cell_t ** my_slice = malloc(sizeof(cell_t) * (my_lines+2) * size);

  MPI_Status stat;
  MPI_Recv(my_slice, (my_lines+2)*size, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD, &stat);

  //tamanho realmente recebido
  int recv_count;
  MPI_Get_count(&stat, MPI_UNSIGNED_CHAR, &recv_count);
  int recv_lines = recv_count / size;

  play(my_slice, size, recv_lines);

  MPI_Send(my_slice, )
}

void master () {
  FILE    *f;
  f = stdin;
  fscanf(f,"%d %d", &size, &steps);

  cell_t ** board, ** next;

  board = allocate_board (size);
  read_file (f, prev,size);
  fclose(f);

  #ifdef DEBUG
  printf("Initial:\n");
  print(prev,size);
  #endif
  int num_threads;
  MPI_Comm_size(&num_threads, MPI_COMM_WORLD);

  //limita número de threads.
  if (num_threads > size) {
    num_threads = size;
    printf("Aviso: foram pedidas mais threads que linhas.
     Usando %d threads.\n", num_threads);
  }
  //a maior parte das threads fica com o num de linhas igual a divisão
  //do tamanho pelo num de threads arredondado para o inteiro mais perto.
  lines_per_thread = round_division(size, num_threads);

  //a última thread ficará com o resto da divisão de linhas
  lines_last_thread = size - (lines_per_thread * (num_threads-1));

  //informa todas threads o tamanho do tabuleiro inteiro
  MPI_Bcast(&size, 1, MPI_INT, 0, MPI_COMM_WORLD);

  //manda tamanho da fatia a calcular para cada thread
  for (i = 1; i < num_threads-1; i++) {
    MPI_Send(&lines_per_thread, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
  }
  //manda tamanho corrigido para último escravo
  MPI_Send(&lines_last_thread, 1, MPI_INT, num_threads-1, 0, MPI_COMM_WORLD);

  for (int step = 0; step < steps; step++) {
    //envia mensagem com a fatia para cada escravo trabalhar
    //a fatia tem uma linha a mais no começo e fim, já que essa
    //informação é necessária para calcular a próxima geração.

    //envia mensagem para primeira thread
    MPI_Isend(&(board[0][0]), (lines_per_thread+1) * size,
       MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD, NULL);

    //envia para threads do meio
    for (i = 2; i < num_threads-1; i++) {

      MPI_Isend(&(board[(i-1) * lines_per_thread -1][0]),
       (lines_per_thread+2) * size,
         MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD, NULL);
    }

    //envia mensagem para última thread
    MPI_Isend(&(board[(num_threads-2) * lines_per_thread - 1][0]),
     (lines_last_thread+1) * size,
       MPI_UNSIGNED_CHAR, num_threads-1, 0, MPI_COMM_WORLD, NULL);

    MPI_Request* requests = malloc(sizeof(MPI_Request) * num_threads-1);

    //recebe do primeiro, linha a mais será descartada
    MPI_Irecv(&(board[0][0]), (lines_per_thread+1) * size,
      MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD, &(requests[0]));

    //recebe resultado de cada escravo
    for (i = 2; i < num_threads-1; i++) {
      MPI_Irecv(&(board[(i-1) * lines_per_thread][0]), (lines_per_thread+2) * size,
        MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD, requests+i-1);
    }

    //recebe da última thread
    MPI_Irecv(&(board[(num_threads-2) * lines_per_thread - 1][0]), (lines_last_thread+1) * size,
      MPI_UNSIGNED_CHAR, num_threads-1, 0, MPI_COMM_WORLD, requests+num_threads-1);

    //espera todos escravos terminarem e mandarem o resultado
    MPI_Waitall(requests, MPI_STATUSES_IGNORE);

    //nesse ponto board já está com o resultado combinado
    //e a próxima geração pode começar, sem swap
  }

  #ifdef RESULT
    printf("Final:\n");
    print (board,size);
  #endif

    free_board(board,size);
  }

/* read a file into the life board */
void read_file (FILE * f, cell_t ** board, int size) {
  int	i, j;
  char	*s = (char *) malloc(size+10);

  /* read the first new line (it will be ignored) */
  fgets (s, size+10,f);

  /* read the life board */
  for (j=0; j<size; j++) {
    /* get a string */
    fgets (s, size+10,f);
    /* copy the string to the life board */
    for (i=0; i<size; i++)
    board[i][j] = s[i] == 'x';

  }
}
int round_division (int a, int b) {
  int result = a / b;
  int remainder = a % b;
  if (remainder < (b/2)) {
    return result;
  } else {
    return (result + 1);
  }
}

int main (int argc, char ** argv) {
  MPI_Comm_rank(&rank);
  if (rank == 0) {
    master();
  } else {
    slave();
  }
  return 0;
}
