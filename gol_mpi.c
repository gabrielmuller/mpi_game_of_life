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
#include <string.h>
#include <mpi.h>
#include <unistd.h>
//variáveis globais
typedef unsigned char cell_t;
int rank, num_threads, lines_per_thread, size;

inline cell_t * allocate_board (int sizeY) {
  cell_t * board = (cell_t *) malloc(sizeof(cell_t)*size*sizeY);
  return board;
}

/* return the number of on cells adjacent to the i,j cell */
inline int adjacent_to (cell_t * board, int sizeX, int sizeY, int i, int j) {
  /*int	k, l, count=0;

  int sk = (i>0) ? i-1 : i;
  int ek = (i+1 < sizeY) ? i+1 : i;
  int sl = (j>0) ? j-1 : j;
  int el = (j+1 < sizeX) ? j+1 : j;
  for (k=sk; k<=ek; k++) {
    for (l=sl; l<=el; l++) {
      count+=board[k*size + l];
    }
  }
  count-=board[i*size + j];*/
  int mul = i*size;
  int bitwise = ((j+1 < sizeX) << 3) | ((j > 0) << 2) | ((i+1 < sizeY) <<1)
| (i>0);
  switch (bitwise) {
    case 15: //soma todos
      return board[mul+size + j] + board[mul+size + j+1]
+ board[mul-size + j] + board[mul-size + j-1] +
board[mul-size + j+1] + board[mul+size + j-1]
+ board[mul + j-1] + board[mul + j+1];
    case 14: //i == 0
      return board[mul+size + j] + board[mul+size + j+1]
 + board[mul+size + j-1] + board[mul + j-1] + board[mul + j+1];
    case 11: // j == 0
      return board[mul+size + j] + board[mul+size + j+1]
+ board[mul-size + j]  + board[mul-size + j+1]+ board[mul + j+1];;
    case 13: //i +1 = sizeY
      return board[mul-size + j] + board[mul-size + j-1] +
board[mul-size + j+1] + board[mul + j-1] + board[mul + j+1];
    case 7: //j+1 = sizeX;
      return board[mul+size + j] + board[mul-size + j]
      + board[mul-size + j-1] + board[mul+size + j-1] + board[mul + j-1];
    case 10:
      return board[mul+size, j] + board[mul, j+1]
      + board[mul+size, j+i];
    case 5:
      return board[mul-size, j] + board[mul, j-1]
      + board[mul-size, j-i];
    case 9:
      return board[mul-size, j] + board[mul, j+1]
      + board[mul-size, j+i];
    case 6:
      return board[mul+size, j] + board[mul, j-1]
      + board[mul+size, j-1];
  }
}

/* print the life board */
void print (cell_t * board, int sizeY) {
  /* for each row */
  for (int j=0; j<sizeY; j++) {
    /* print each column position... */
    for (int i=0; i<size; i++) {
    printf("%c", board[j*size + i] ? 'x' : ' ');
    }
    /* followed by a carriage return */
    printf ("\n");
  }
}

inline void play (cell_t * board, int sizeX, int sizeY) {
  int	i, j, a, mult;
  int total_size = sizeX*sizeY;
  cell_t * prev = (cell_t*) malloc(total_size*sizeof(cell_t));
  memcpy(prev, board, total_size);
  /* for each cell, apply the rules of Life */
  for (i=0; i<sizeY; i++) {
    mult = i * size;
    for (j=0; j<sizeX; j++) {
      int a = adjacent_to(prev, sizeX, sizeY, i, j);
      /*if (a == 2) board[i*size + j] = prev[i*size + j];
      if (a == 3) board[i*size + j] = 1;
      if (a < 2) board[i*size + j] = 0;
      if (a > 3) board[i*size + j] = 0;*/
      board[mult+j] = (a == 2) ? prev[mult+j] : (a == 3);
    }
  }
  free(prev);
}

/* read a file into the life board */
void read_file (FILE * f, cell_t * board, int size) {
  int	i, j;
  char	*s = (char *) malloc(size+10);

  /* read the first new line (it will be ignored) */
  fgets (s, size+10,f);

  /* read the life board */
  for (j=0; j<size; j++) {
    /* get a string */
    fgets (s, size+10,f);
    /* copy the string to the life board */
    for (i=0; i<size; i++) {
      if(s[i] == '\0' || s[i] == '\n') {
        break;
      }
      board[j*size + i] = s[i] == 'x';
    }
  }
  free(s);
}
//retorna as duas linhas que a fatia precisa
//um elemento pode ser nulo ()'N') se não for necessário,
//como na primeira e última
inline cell_t * create_border_packet (cell_t* borders, int border_size, int slice) {
    cell_t * result = allocate_board(2);
    for (int i = 0; i < 2; i++) {
      int index = (slice-1)*2 +i*3;
      if (index >= 0 && index < border_size) {
        memcpy(&(result[i*size]), &(borders[index*size]), size);
      }
    }
    return result;
}

inline void lpt () {
  lines_per_thread = size / num_threads;
  if (size % num_threads != 0) {
    lines_per_thread++;
  }
}

void slave () {
  int master = num_threads-1;

  //info[0]: size; info[1]: num_threads corrigido
  int info[2];
  MPI_Bcast(info, 2, MPI_INT, master, MPI_COMM_WORLD);
  MPI_Barrier(MPI_COMM_WORLD);
  size = info[0];
  num_threads = info[1];
  if (rank >= num_threads-1) {
    return;
  }
  lpt();

  int first = (rank == 0); //1 se primeira thread
  int slice_size = lines_per_thread + 2 - first;
  cell_t * my_slice = allocate_board(slice_size);
  //recebe tabuleiro inicial
  MPI_Recv(&(my_slice[(1-first)*size]), lines_per_thread*size,
MPI_UNSIGNED_CHAR, master, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  while (1) {
    //recebe border packet
    cell_t * packet = allocate_board(2);
    MPI_Recv(packet, 2*size, MPI_UNSIGNED_CHAR, master, 0, MPI_COMM_WORLD,
MPI_STATUS_IGNORE);

    if (packet[0] == 'F') {
      break;
    }

    if (!first) {
      memcpy(my_slice, packet, size);
    }
    memcpy(&(my_slice[((slice_size-1)*size)]), &(packet[1*size]), size);
    play(my_slice, size, slice_size);

    if (!first) {
      memcpy(packet, &(my_slice[1*size]), size);
    }
    memcpy(&(packet[1*size]), &(my_slice[(slice_size-2)*size]), size);

    //envia pacote de volta
    MPI_Send(packet, 2*size, MPI_UNSIGNED_CHAR, master, 0, MPI_COMM_WORLD);
  }
  //envia resultado final
  MPI_Send(&(my_slice[(1-first)*size]), lines_per_thread*size, MPI_UNSIGNED_CHAR, master, 0,
MPI_COMM_WORLD);
  free(my_slice);
}
//imprime board no final
void end(cell_t* board) {
  #ifdef RESULT
    printf("Final:\n");
    print (board,size);
  #endif
  free(board);
}
//quando mestre é única thread
void single (int steps, cell_t* board) {
  for (int step = 0; step < steps; step++) {
    play(board, size, size);
  }
  end(board);
}
void master (char* filename) {

  int steps;
  FILE    *f;
  f = fopen(filename, "r");;

  fscanf(f,"%d %d", &size, &steps);

  cell_t* board = allocate_board (size);
  read_file (f, board, size);
  fclose(f);

  if (num_threads > size) {
    num_threads = size;
    printf("Aviso: há mais threads que linhas. Apenas %d threads serão usadas.\n",
     num_threads);
  }

  if (num_threads == 1) {
    single(steps, board);
    return;
  }
  #ifdef DEBUG
  printf("Initial:\n");
  print(board,size);
  #endif

  //a maior parte das threads fica com o num de linhas igual a divisão
  //do tamanho pelo num de threads arredondado para cima.
  lpt();
  //a última thread ficará com o resto da divisão de linhas, e será mestre
  int lines_last_thread = size - (lines_per_thread * (num_threads-1));
  int slice_size = lines_last_thread + 1;

  //aloca fatia do mestre
  cell_t * my_slice = allocate_board(slice_size);
  memcpy(&(my_slice[size]),
&(board[(num_threads-1)*lines_per_thread*size]), lines_last_thread * size);

  int info[2] = {size, num_threads};
  //informa info a todos
  MPI_Bcast(info, 2, MPI_INT, rank, MPI_COMM_WORLD);
  MPI_Barrier(MPI_COMM_WORLD);

  MPI_Request* reqs = (MPI_Request*)
malloc(sizeof(MPI_Request) * (num_threads-1));

  //manda fatias do tabuleiro inicial para cada escravo
  for (int i = 0; i < num_threads-1; i++) {
    MPI_Isend(&(board[i*lines_per_thread*size]), lines_per_thread * size,
MPI_UNSIGNED_CHAR,
i, 0, MPI_COMM_WORLD, &(reqs[i]));
  }

  //cada "fronteira" entre as fatias tem duas linhas de transição
  //uma em cima (par) da linha, outra embaixo (ímpar)
  int border_size = (num_threads-1) * 2;
  cell_t * borders = allocate_board(border_size);

  //inicia valores de borders
  for (int i = 0; i < num_threads -1; i++) {
    int p = (i+1)*lines_per_thread;
    memcpy(&(borders[(i*2)*size]),
&(board[(p-1)*size]), sizeof(cell_t) * size);
    memcpy(&(borders[(i*2+1)*size]),
&(board[p*size]), sizeof(cell_t) * size);
  }
  MPI_Waitall(num_threads-2, reqs, MPI_STATUSES_IGNORE);
  cell_t * all_packets = allocate_board(2 * (num_threads-1));
  for (int step = 0; step < steps; step++) {
    //envia valores das fronteiras para escravos começarem a trabalhar
    for (int i = 0; i < num_threads -1; i++) {
      cell_t * packet = create_border_packet(borders, border_size, i);
      MPI_Isend(packet, 2*size, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD, &(reqs[i]));
      free(packet);
    }


    //mestre também trabalha
    memcpy(my_slice, &(borders[(border_size-2)*size]), size);

    play(my_slice, size, slice_size);

    MPI_Waitall(num_threads-1, reqs, MPI_STATUSES_IGNORE);

    //recebe resultado de todos
    for (int i = 0; i < num_threads-1; i++) {
      MPI_Irecv(&(all_packets[i*2*size]), 2*size, MPI_UNSIGNED_CHAR, i, 0,
MPI_COMM_WORLD, &(reqs[i]));
    }
    //espera receber de todos
    MPI_Waitall(num_threads-1, reqs, MPI_STATUSES_IGNORE);

    //atualiza fronteira
    memcpy(borders, &(all_packets[size]), border_size*size);
    memcpy(&(borders[(border_size-1)*size]), &(my_slice[size]), size);

  }

  //manda sinal que acabou
  cell_t * signal = allocate_board(2);
  signal[0] = 'F';

  for (int i = 0; i < num_threads-1; i++) {
    MPI_Send(signal, 2*size, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD);
  }

  //recebe resultado final
  for (int i = 0; i < num_threads-1; i++) {
    MPI_Irecv(&(board[i*lines_per_thread*size]), lines_per_thread * size,
MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD, &(reqs[i]));

  }
  memcpy(&(board[(num_threads-1)*lines_per_thread*size]),
&(my_slice[size]), lines_last_thread*size);

  MPI_Waitall(num_threads-1, reqs, MPI_STATUSES_IGNORE);

  free(borders);
  free(reqs);
  free(my_slice);
  free(all_packets);
  end(board);

}

int main (int argc, char ** argv) {

  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &num_threads);
  if (argc < 2) {
    if (rank == 0) {
      printf("-----------------------------------------------\n");
      printf("Formato: mpirun -np X ./gol nome-do-arquivo.in\n");
      printf("Execução cancelada. \n");
      printf("-----------------------------------------------\n");
    }
    MPI_Finalize();
    return 0;
  }
  //mestre é a última fatia
  if (rank == num_threads-1) {
    master(argv[1]);
  } else {
    slave();
  }
  MPI_Finalize();
  return 0;
}
