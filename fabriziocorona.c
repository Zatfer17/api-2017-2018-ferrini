#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define true 1
#define false 0

//#define block_size 8192

//////////////////
//STRUTTURE DATI//
//////////////////

//struttura dati transizioni
struct transition_t {
  char written;
  int move; //NOTE: se L -1, se S 0, se R +1
  int final_state;
  struct transition_t *next;
};

//struttura dati coda tape
struct tape_id_t{
  char *right_tape;
  char *left_tape;
  char direction;
  unsigned int position; //se position >0 sono nel nastro destro, se <0 sono nel sinistro, stai bene attemto a come gestisci posizione 0
  int current_state;
  struct tape_id_t *next;
};

//struttura dati generale della TM
struct my_tm_t {
  struct transition_t ***matrix; //ha dimensione pari a biggest_dep_state + 1
  struct tape_id_t *queue;  //ha dimensione pari a biggest_dep_state + 1
  char *accept_state; //ha dimensione pari a biggest_accept_state + 1
  unsigned long maximum_transitions;
  unsigned long current_transitions;
  int biggest_state;
  int biggest_dep_state;
  int biggest_accept_state;
};

//dichiaro struttura TM
struct my_tm_t my_tm;


//////////////////////
//PROTOTIPI FUNZIONI//
//////////////////////

void consume_stuff(){
  char c;
  c=getchar();
  while(c!='\n'){
    //printf("%c\n", c);
    c=getchar();
  }
}

int ascii_encoder(int ascii_code){
  //a=97, z=122, _=95, A=65, Z=90, 0=48, 9=57
  if (ascii_code>=97 && ascii_code<=122){
    return (ascii_code-97);
  }
  else if (ascii_code==95){
    return 26;
  }
  else if (ascii_code>=65 && ascii_code<=90){
    return (ascii_code-38);
  }
  else if (ascii_code>=48 && ascii_code<=57){
    return (ascii_code+5);
  }
  else {
    exit(1);
  }
}

struct tape_id_t *nondeterministic_move(struct tape_id_t *queue_cursor, struct transition_t *transition_cursor){

  //printf("MOSSA NONDETERMINISTICA\n");

  struct tape_id_t *new_tape=malloc(sizeof(struct tape_id_t));
  new_tape->right_tape=malloc((strlen(queue_cursor->right_tape)+1)*sizeof(char));
  strcpy(new_tape->right_tape, queue_cursor->right_tape);
  new_tape->left_tape=malloc((strlen(queue_cursor->left_tape)+1)*sizeof(char));
  strcpy(new_tape->left_tape, queue_cursor->left_tape);

  switch (queue_cursor->direction){
    case 'r':
      if(transition_cursor->move==-1 && queue_cursor->position==0){
        new_tape->position=0;
        new_tape->direction='l';
      }
      else {
        new_tape->position=queue_cursor->position+transition_cursor->move;
        new_tape->direction=queue_cursor->direction;
      }
      new_tape->right_tape[queue_cursor->position]=transition_cursor->written;
      new_tape->current_state=transition_cursor->final_state;
      new_tape->next=NULL;
      break;
    case 'l':
      if (transition_cursor->move==1 && queue_cursor->position==0){
        new_tape->position=0;
        new_tape->direction='r';
      }
      else {
        new_tape->position=queue_cursor->position-transition_cursor->move;
        new_tape->direction=queue_cursor->direction;
      }
      new_tape->left_tape[queue_cursor->position]=transition_cursor->written;
      new_tape->current_state=transition_cursor->final_state;
      new_tape->next=NULL;
      break;
  }
  return new_tape;
}

struct tape_id_t *deterministic_move(struct tape_id_t *queue_cursor, struct transition_t *transition_cursor){

  //printf("MOSSA DETERMINISTICA\n");

  switch (queue_cursor->direction){
    case 'r':
      if (transition_cursor->move==-1 && queue_cursor->position==0){
        queue_cursor->right_tape[queue_cursor->position]=transition_cursor->written;
        queue_cursor->position=0;
        queue_cursor->direction='l';
      }
      else {
        queue_cursor->right_tape[queue_cursor->position]=transition_cursor->written;
        queue_cursor->position=queue_cursor->position+transition_cursor->move;
      }
      queue_cursor->current_state=transition_cursor->final_state;
      queue_cursor->next=NULL;
      break;
    case 'l':
      if (transition_cursor->move==1 && queue_cursor->position==0){
        queue_cursor->left_tape[queue_cursor->position]=transition_cursor->written;
        queue_cursor->position=0;
        queue_cursor->direction='r';
      }
      else {
        queue_cursor->left_tape[queue_cursor->position]=transition_cursor->written;
        queue_cursor->position=queue_cursor->position-transition_cursor->move;
      }
      queue_cursor->current_state=transition_cursor->final_state;
      queue_cursor->next=NULL;
      break;
  }
  return queue_cursor;

}

struct tape_id_t *enqueue(struct tape_id_t *tape, struct tape_id_t *alt_queue){
  if (alt_queue==NULL){
    alt_queue=tape;
  }
  else {
    struct tape_id_t *temp=alt_queue;
    alt_queue=tape;
    tape->next=temp;
  }
  return alt_queue;
}

void tm_setup(){

  //elimino caratteri inutili
  consume_stuff();

  int row; //stato partenza letto
  char column; //codifica ascii carattere letto
  char written; //da scrivere
  char move; //mossa, +1 se R, 0 se S, -1 se L
  int final; //stato finale

  int number_of_states=0;

  int return_value;

  my_tm.matrix = malloc(sizeof(struct transition_t *));

  //creo un array di array
  //la prima indicizzazione avviene per stato, la seconda per carattere letto
  while(return_value=scanf("%d %c %c %c %d ", &row, &column, &written, &move, &final)>0){
    if (row>=number_of_states){
      my_tm.matrix = realloc(my_tm.matrix, (row+1)*sizeof(struct transition_t *));
      int i,j;
      for (i=number_of_states;i<row+1;i++){
        my_tm.matrix[i]=malloc(63*sizeof(struct transition_t *));
        for(j=0;j<63;j++){
          my_tm.matrix[i][j]=NULL;
        }
      }
      number_of_states=row+1;
    }

    if (row>my_tm.biggest_state){
      my_tm.biggest_state=row;
    }

    if (final>my_tm.biggest_state){
      my_tm.biggest_state=final;
    }

    struct transition_t *new_state = malloc(sizeof(struct transition_t));

    int encoded_value=ascii_encoder(column);
    if (my_tm.matrix[row][encoded_value]!=NULL){
      struct transition_t *cursor;
      cursor=my_tm.matrix[row][encoded_value];
      while(cursor->next!=NULL){
        cursor=cursor->next;
      }
      cursor->next=new_state;
    }
    else {
      my_tm.matrix[row][encoded_value]=new_state;
    }

    new_state->written=written;
    switch(move){
      case 'R':
        new_state->move=1;
        break;
      case 'L':
        new_state->move=-1;
        break;
      case 'S':
        new_state->move=0;
        break;
    }
    new_state->final_state=final;
    new_state->next=NULL;
  }

  my_tm.biggest_dep_state=number_of_states-1;


  int i,j;

  my_tm.matrix = realloc(my_tm.matrix, (my_tm.biggest_state+1)*sizeof(struct transition_t *));
  for (i=my_tm.biggest_dep_state+1;i<my_tm.biggest_state+1;i++){
    my_tm.matrix[i]=malloc(63*sizeof(struct transition_t *));
    for(j=0;j<63;j++){
      my_tm.matrix[i][j]=NULL;
    }
  }

  //elimino caratteri inutili
  consume_stuff();

  int acc;
  number_of_states=0;
  my_tm.accept_state = malloc(sizeof(char));

  //creo array degli stati di accettazione in modo analogo a sopra
  while(return_value=scanf("%d%*c", &acc)>0){
    if (acc>=number_of_states){
      my_tm.accept_state=realloc(my_tm.accept_state, (acc+1)*sizeof(char));
      int i;
      for (i=number_of_states;i<acc;i++){
        my_tm.accept_state[i]='n';
      }
      my_tm.biggest_accept_state=acc;
      number_of_states=acc+1;
    }
    my_tm.accept_state[acc]='y';
  }

  my_tm.biggest_accept_state=number_of_states-1;

  my_tm.accept_state=realloc(my_tm.accept_state, (my_tm.biggest_state+1)*sizeof(char));
  for (i=my_tm.biggest_accept_state+1; i<my_tm.biggest_state+1; i++){
    my_tm.accept_state[i]='n';
  }

  //elimino caratteri inutili
  consume_stuff();

  scanf("%lu%*c\n", &my_tm.maximum_transitions);

  //elimino caratteri inutili
  consume_stuff();

}

void tm_execution(){
  char *corona=NULL;

  int right_block_size=64;
  int left_block_size=64;

  //int i=0;

  while(scanf("%ms", &corona)!=EOF){

    int right_cats=0;
    int left_cats=0;

    char *right_blank_block;
    right_blank_block=malloc((right_block_size+1)*sizeof(char));
    memset(right_blank_block, '_', right_block_size*sizeof(char));
    right_blank_block[right_block_size]='\0';

    char *left_blank_block;
    left_blank_block=malloc((left_block_size+1)*sizeof(char));
    memset(left_blank_block, '_', left_block_size*sizeof(char));
    left_blank_block[left_block_size]='\0';

    my_tm.queue=malloc(sizeof(struct tape_id_t));

    my_tm.queue->right_tape=malloc((strlen(corona)+right_block_size+1)*sizeof(char));
    strcpy(my_tm.queue->right_tape, corona);
    strcat(my_tm.queue->right_tape, right_blank_block);

    my_tm.queue->left_tape=malloc((left_block_size+1)*sizeof(char));
    strcpy(my_tm.queue->left_tape, left_blank_block);

    //CORONA NON PERDONA
    free(corona);
    free(right_blank_block);
    free(left_blank_block);

    my_tm.queue->position=0;
    my_tm.queue->current_state=0;
    my_tm.queue->direction='r';
    my_tm.queue->next=NULL;

    int exit_value=-1;

    my_tm.current_transitions=0;

    while (true){

      //printf("CICLO #%d\n", i);

      struct tape_id_t *queue_cursor=my_tm.queue;
      struct tape_id_t *alt_queue=NULL;
      struct transition_t *transition_cursor;

      //per ogni elemento della lista
      while (queue_cursor!=NULL){
        //preparo transition_cursor
        switch (queue_cursor->direction){
          case 'r':
          //eventualmente estendo nastro se arrivo a carattere terminatore
            if (queue_cursor->right_tape[queue_cursor->position]=='\0'){
              //right_cats++;
              queue_cursor->right_tape=realloc(queue_cursor->right_tape, (strlen(queue_cursor->right_tape)+right_block_size+1)*sizeof(char));
              right_blank_block=malloc((right_block_size+1)*sizeof(char));
              memset(right_blank_block, '_', right_block_size*sizeof(char));
              right_blank_block[right_block_size]='\0';
              strcat(queue_cursor->right_tape, right_blank_block);
              free(right_blank_block);
              right_block_size=right_block_size+64;
            }
            transition_cursor=my_tm.matrix[queue_cursor->current_state][ascii_encoder(queue_cursor->right_tape[queue_cursor->position])];
            break;
          case 'l':
            if (queue_cursor->left_tape[queue_cursor->position]=='\0'){
              //left_cats++;
              queue_cursor->left_tape=realloc(queue_cursor->left_tape, (strlen(queue_cursor->left_tape)+left_block_size+1)*sizeof(char));
              left_blank_block=malloc((left_block_size+1)*sizeof(char));
              memset(left_blank_block, '_', left_block_size*sizeof(char));
              left_blank_block[left_block_size]='\0';
              strcat(queue_cursor->left_tape, left_blank_block);
              free(left_blank_block);
              left_block_size=left_block_size+64;
            }
            transition_cursor=my_tm.matrix[queue_cursor->current_state][ascii_encoder(queue_cursor->left_tape[queue_cursor->position])];
            break;
        }

        struct tape_id_t *next=queue_cursor->next;

        //se NULL
        if (transition_cursor==NULL){
          //se stato di accettazione, exit_value=1, break;
          if (my_tm.accept_state[queue_cursor->current_state]=='y'){
            exit_value=1;
            break;
          }
          //se stato pozzo ma non accettazione, lo dequeuo;
          else{
            free(queue_cursor->right_tape);
            free(queue_cursor->left_tape);
            free(queue_cursor);
            my_tm.queue=next;
          }
        }
        //altrimenti
        else {
          //se rientro con le mosse
          if (my_tm.current_transitions<my_tm.maximum_transitions){
            //per ogni mossa
            while(transition_cursor->next!=NULL){
              alt_queue=enqueue(nondeterministic_move(queue_cursor, transition_cursor), alt_queue);
              transition_cursor=transition_cursor->next;
            }
            alt_queue=enqueue(deterministic_move(queue_cursor, transition_cursor), alt_queue);
          }
          //altrimenti exit_value=2
          else {
            exit_value=2;
            break;
          }
        }

        queue_cursor=next;

      }
      //incremento current_transitions
      my_tm.current_transitions++;
      //se arrivato qui, dopo aver travasato alt_queue la coda e' vuota, exit_value=0


      if (exit_value>=0){ //e' successo cioe' qualcosa
        break;
      }

      my_tm.queue=alt_queue;
      alt_queue=NULL;

      if (my_tm.queue==NULL){
        exit_value=0;
      }

      if (exit_value>=0){ //e' successo cioe' qualcosa
        break;
      }


    }

    switch (exit_value) {
      struct tape_id_t *queue_cursor;
      case 0:
        printf("0\n");
        break;
      case 1:
        printf("1\n");
        queue_cursor=my_tm.queue;
        while(queue_cursor!=NULL){
          free(queue_cursor->right_tape);
          free(queue_cursor->left_tape);
          struct tape_id_t *temp=queue_cursor;
          queue_cursor=queue_cursor->next;
          free(temp);
        }
        my_tm.queue=NULL;
        break;
      case 2:
        printf("U\n");
        queue_cursor=my_tm.queue;
        while(queue_cursor!=NULL){
          free(queue_cursor->right_tape);
          free(queue_cursor->left_tape);
          struct tape_id_t *temp=queue_cursor;
          queue_cursor=queue_cursor->next;
          free(temp);
        }
        my_tm.queue=NULL;
        break;
    }
    //i++;

    /*
    if (right_cats>100){
      right_block_size=right_block_size*8;
    }
    if (left_cats>100){
      left_block_size=left_block_size*8;
    }
    */
  }
}

int main(){
  tm_setup();
  tm_execution();
  return 0;
}
