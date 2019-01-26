//
// Created by Pedro Costa Nº31179 , Paulo Bento Nº 33959 on 09-03-2017.
//

#include "domino.h"
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <unistd.h>
#include <limits.h>
#include <syscall.h>
#include <stropts.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>

#define S 2000 /// espaço para guardar sequencias

char *path_file_hand = "/media/sf_newteste/threads_versao2/data/mao20P.txt";

//Mão global do cador
HAND *handG;

typedef struct sequences /// estrutura global para armazenar as sequencias e as threads
{
    char * sequences;
    char *thread_id;

} SEQUENCES;

SEQUENCES *allseq;

pthread_t thread_ids[28];
pthread_t *thread_idc;

pthread_mutex_t mutex_p=PTHREAD_MUTEX_INITIALIZER; /// trinco produtor
pthread_mutex_t mutex_c=PTHREAD_MUTEX_INITIALIZER; /// trinco consumidor

sem_t savefile; ///semaforo para threads consumidoras
sem_t saveseq; /// semaforo para threads produtoras

int prod=0;
int cons=0;

static int countc=0;
static int countp=0; /// variavel que conta as sequencias ainda para consumir
int saveG;

BLOCK * associate(BLOCK *pfirst,int pos)
{
    int i=0;
    while(pfirst!=NULL)
    {
        if(i==pos)
        {
            return pfirst;
        }
        pfirst=pfirst->pnext;
        i++;
    }
    return NULL;
}

void *threadhandler(void * pos)
{
    int i;
    i=(int) pos;
    HAND * handst;
    handst = (HAND *) malloc(sizeof(HAND));
    handst->pfirst=NULL;
    handst->hand_size=0;

    //handst=handG;
    handst=copyHand(handG,handst,handG->hand_size);
    BLOCK * peca = (BLOCK *)malloc(sizeof(BLOCK));
    peca=handst->pfirst;
    peca=associate(peca,i);


    BLOCK *bdummie = (BLOCK *)malloc(sizeof(BLOCK));
    bdummie->left_side=peca->left_side;
    bdummie->right_side=peca->right_side;
    bdummie->available=0;
    bdummie->pprev=NULL;
    bdummie->pnext=NULL;

    SEQUENCE *seq;
    seq = (SEQUENCE *) malloc(sizeof(SEQUENCE));
    seq->pfirst = NULL;
    seq->size_of_sequence = 0;

    seq->pfirst=bdummie;
    seq->pfirst->pnext=NULL;
    seq->pfirst->pprev=bdummie;
    seq->size_of_sequence++;
    peca->available=0;
    recursive_backtrack(handst,seq,1);
}
void *threadsaverhandler(void * pos)
{
    int i;
    char buffer[PIPE_BUF];
    i=(int) pos;
    while(1)
    {
        sem_wait(&savefile);
        pthread_mutex_lock(&mutex_c);
        cons = (cons+1) % S;
        sprintf(buffer,"%s-%s\n",allseq[cons].thread_id,allseq[cons].sequences);
        write(saveG,buffer,strlen(buffer));
        countc++;
        pthread_mutex_unlock(&mutex_c);
        sem_post(&saveseq);

    }
}
int main(int argc, const char * argv[])
{
    struct sigaction sa;
    sigset_t block_mask;
    unsigned int tts;
    int thread_cons= atoi(argv[1]);
    struct timeval tfather,tfather_end, tthread_time, tthread_end, tthread_consumer;

    gettimeofday(&tfather, NULL);

    //Vars threads
    int i=0;
    int save=0;

    thread_idc = (pthread_t*) malloc(sizeof(pthread_t)* thread_cons);

    srand((unsigned)time(NULL));

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char fname[64];
    strftime(fname, sizeof(fname), "%a %b %d %Hh %Y",tm);

    char path_save_file[150] = "./data/"; //Caminho para a pasta de dados.
    strcat(path_save_file,fname);
    strcat(path_save_file,".txt");
    strcat(path_save_file,"\0");
    //printf("path file:%s\n",path_save_file);

    save = open(path_save_file, O_WRONLY| O_CREAT | O_TRUNC | O_APPEND, 0666);
    if(save==-1)
    {
        perror ("Opening Destination File");
        exit(1);
    }
    saveG=save;
    DECK *deck = NULL;
    deck = (DECK *) malloc(sizeof(DECK));
    deck->pfirst = NULL;
    deck->num_blocks_available = NUM_BLOCKS;

    //Inicialização do baralho
    deck->pfirst = create_deck(deck->pfirst);

    handG = (HAND *) malloc(sizeof(HAND));
    handG->pfirst=NULL;
    handG->hand_size=0;

    handG = read_hand_from_file(handG, path_file_hand);
    print_hand(handG);
    //Lista com todas as sequências guardadas
    allseq = (SEQUENCES*) malloc(sizeof(SEQUENCES)* S); /// aloca espaço para a armazenar as sequencias e o thread_id
    for(i=0; i<S; i++)
    {
        allseq[i].thread_id=(char *) malloc(sizeof(char)*150);
        allseq[i].sequences =  (char *) malloc(sizeof(char)*150);
    }
    sem_init(&savefile,0,0); /// semaforo as threads do consumiras , iniciado a 0, pois só quando a thread produzir é que abre o semaforo para consumir
    sem_init(&saveseq,0,S); /// semaforo as threads do produtoras , iniciado a S

    gettimeofday(&tthread_time, NULL);
    for(i=0; i<handG->hand_size; i++) /// cria threads produtoras
    {
        pthread_create(&thread_ids[i],NULL,&threadhandler,(void*) i);
        printf("recursive thread %d created %lu \n",i,(unsigned long)thread_ids[i]);
    }
    gettimeofday(&tthread_consumer, NULL);
    for(i=0; i<thread_cons; i++) /// cria threads consumidoras
    {
        pthread_create(&thread_idc[i],NULL,&threadsaverhandler,(void*) i);
        printf("saver thread %d created %lu \n",i,(unsigned long)thread_idc[i]);
    }
    for(i=0; i<handG->hand_size; i++)
    {
        pthread_join(thread_ids[i],NULL);
        printf("prcoducer thread %d finish\n",i);
    }
    gettimeofday(&tthread_end, NULL);
    printf("waiting...\n");
    while(countp-countc>0) ///código de espera.
    {
        printf("...\n");
    }
    printf("done.\n");
    gettimeofday(&tfather_end, NULL);
    printf("time of thread = %f seconds\n",(double) (tthread_end.tv_usec - tthread_time.tv_usec) / 1000000 +(double) (tthread_end.tv_sec - tthread_time.tv_sec));
    printf("time of write = %f seconds\n",(double) (tthread_end.tv_usec - tthread_consumer.tv_usec) / 1000000 +(double) (tthread_end.tv_sec - tthread_consumer.tv_sec));
    printf("time of waiting = %f seconds\n",(double) (tfather_end.tv_usec - tthread_end.tv_usec) / 1000000 +(double) (tfather_end.tv_sec - tthread_end.tv_sec));
    printf("father PID %d# total time = %f seconds\n",getpid(),(double) (tfather_end.tv_usec - tfather.tv_usec) / 1000000 +(double) (tfather_end.tv_sec - tfather.tv_sec));
    exit(0);
}

void save_file_sequences_write(int save)
{
    int bytes;
    char buffer[PIPE_BUF];
    int i=0;
    int n,file;
    while(1)
    {
        cons = (cons+1) %S;
        sprintf(buffer,"%s-%s\n",allseq[cons].thread_id,allseq[cons].sequences);
        write(save,buffer,strlen(buffer));
    }
}
/**
 * create_deck - Função responsável por criar o baralho de peças do domino.
 * @param pblock - Apontador para uma lista de peças inicialmente vazia.
 * @return lista de peças do domino.
 */
BLOCK *create_deck(BLOCK *pblock)
{
    short i = 0, valMax = 6, valMin = 0, blocks = NUM_BLOCKS;
    for (valMax = 6; valMax >= 0; valMax--)
    {
        valMin = 0;
        for (i = 0; i <= valMax; i++)
        {
            pblock = insert(pblock, valMax, valMin, 1);
            blocks--;
            valMin++;
        }
    }
    return pblock;
}

/**
 * create_hand - Função responsável por distribuir peças do baralho para a mão do jogador.
 * @param phand - Apontador para a mão do jogador.
 * @param pdeck - Apontador para o baralho de peças.
 * @param num_blocks_hand - Número de peças a serem alocada à mão do jogador.
 * @return Apontador para a primeira peça da mão do jogador.
 */
HAND *create_hand(HAND *phand, DECK *pdeck, short num_blocks_hand)
{
    int index = 0;
    short i = 0, upLim = pdeck->num_blocks_available;
    HAND *pprim = phand;
    BLOCK *paux = NULL;
    for (i = 0; i < num_blocks_hand; i++)
    {
        index = uniform_index_block(0, upLim);
        paux = get_block(pdeck, index);
        phand->pfirst = insert(phand->pfirst, paux->left_side, paux->right_side, paux->available);
        upLim--;
        pdeck->num_blocks_available = upLim;
    }
    pprim->hand_size = num_blocks_hand;
    return pprim;
}


HAND *copyHand(HAND * oHand,HAND * dHand, int nsize)
{
    short i = 0,j=nsize, num_blocks = 0, leftSide = 0, rightSide = 0;
    HAND *pprim = dHand;
    num_blocks=nsize;
    BLOCK * baux = oHand->pfirst;
    while(baux!=NULL)
    {
        leftSide= baux->left_side;
        rightSide= baux->right_side;
        dHand->pfirst = insert(dHand->pfirst, leftSide, rightSide, 1);
        baux=baux->pnext;
    }
    pprim->hand_size = num_blocks;
    return pprim;
}

/**
 * uniform_index_block - Gera um valor aleatório entre um intervalo de valores.
 * @param val_min - valor mais baixo do intervalo.
 * @param val_max - valor mais alto do intervalo.
 * @return valor gerado.
 */
int uniform_index_block(short val_min, short val_max)
{
    return val_min + rand() % (val_max - val_min + 1);
}

/**
 * get_block - devolve uma peça.
 * @param pdeck - Apontador para o baralho de peças.
 * @param index - Posição da peça a returnar.
 * @return Uma peça do baralho.
 */
BLOCK *get_block(DECK *pdeck, int index)
{

    BLOCK *pblock = pdeck->pfirst;
    BLOCK *paux = NULL, *pant = NULL;
    int i = 0;

    if (pblock == NULL)
    {
        printf("Empty\n");
        return NULL;
    }
    paux = pblock;

    while (paux != NULL && i != index)
    {
        pant = paux;
        paux = paux->pnext;
        i++;
    }

    if (pant == NULL)
    {
        pdeck->pfirst = paux->pnext;
        paux->pnext = NULL;
        return paux;
    }

    if (paux != NULL)
    {
        pant->pnext = paux->pnext;
        paux->pnext = NULL;
        return paux;
    }
    return paux;
}

/**
 * read_hand_from_file - Cria a mão de peças do jogador atrevés da leitura de um ficheiro.
 * @param phand - Apontador para a mão do jogador.
 * @param pathFile - Caminho para o ficheiro.
 * @return Apontador para a primeira peça da mão do jogador.
 */
HAND *read_hand_from_file(HAND *phand, char *pathFile)
{
    short i = 0, num_blocks = 0, leftSide = 0, rightSide = 0;;
    HAND *pprim = phand;
    FILE *fp = NULL;
    fp = fopen(pathFile, "r");
    if (fp != NULL)
    {
        fscanf(fp, "%hi", &num_blocks);
        for (i = 0; i < num_blocks; i++)
        {
            fscanf(fp, "%*c %*c %hi %*s %hi %*c", &leftSide, &rightSide);
            phand->pfirst = insert(phand->pfirst, leftSide, rightSide, 1);
        }
    }
    fclose(fp);
    pprim->hand_size = num_blocks;
    return pprim;
}

/**
 * print_hand - Imprime a mão do jogador.
 * @param phand - Apontador para a mão do jogador.
 */
void print_hand(HAND *phand)
{
    HAND *paux = phand;
    BLOCK *pblock = paux->pfirst;
    while (pblock != NULL)
    {
        printf("[%d %d]\n", pblock->left_side, pblock->right_side);
        pblock = pblock->pnext;
    }
}

/**
 * insert - Aloca memória para uma peça e inseri-a num lista de peças.
 * @param pblock - Apontador para uma lista de peças.
 * @param left_side - Valor esquerdo da peça.
 * @param right_side - Valor direito da peça.
 * @param avaiable - Valor que indica se a peça esta dispinivel. (0/1)
 * @return Apontador para uma lista de peças.
 */
BLOCK *insert(BLOCK *pblock, short left_side, short right_side, short avaiable)
{

    BLOCK *pnew = NULL;
    BLOCK *paux = pblock;

    pnew = (BLOCK *) malloc(sizeof(BLOCK));
    pnew->left_side = left_side;
    pnew->right_side = right_side;
    pnew->available = avaiable;
    pnew->pnext = pnew;
    pnew->pprev = pnew;

    if (paux == NULL)
    {
        return pnew;
    }

    paux->pprev->pnext = pnew;
    pnew->pprev = paux->pprev;
    paux->pprev = pnew;
    pnew->pnext = NULL;

    return paux;
}

/**
 * recursive_backtrack - Função recursiva responsável pela pesquisa exaustiva de sequências de peças.
 * @param phand - Apontador para a mão do jogador.
 * @param pseq - Apontador para a primeira peça da lista de sequência.
 * @param pall_sequences - Apontador para uma lista de todas as sequências geradas.
 * @param inserted - Número de peças inseridas na sequencia.
 * @return 0 - Quando termina a procura.
 */
int recursive_backtrack(HAND *phand, SEQUENCE *pseq, short inserted)
{
    int i = 0;
    int k=0;
    BLOCK *block = NULL;
    block = phand->pfirst;

    for (i = 0; i < phand->hand_size; i++)
    {
        if (block->available == 1)
        {
            if (is_current_assignment_consistent(pseq, block, inserted) == ASSIGNMENT_CONSISTENT)
            {

                BLOCK *pnew = (BLOCK *) malloc(sizeof(BLOCK));
                pnew->left_side = block->left_side;
                pnew->right_side = block->right_side;
                pnew->available = 0;
                pnew->pnext = NULL;
                pnew->pprev = NULL;

                if(inserted == 0)
                {
                    pseq->pfirst = pnew;
                    pseq->pfirst->pnext = NULL;
                    pseq->pfirst->pprev = pnew;
                }
                else
                {
                    pnew->pprev = pseq->pfirst->pprev;
                    pnew->pprev->pnext = pnew;
                    pnew->pnext = NULL;
                    pseq->pfirst->pprev = pnew;
                }

                inserted++;
                pseq->size_of_sequence++;
                block->available = 0;

                if(pseq->size_of_sequence == phand->hand_size)
                {
                    save_sequence(pseq);
                }
                recursive_backtrack(phand, pseq, inserted);

                block->available = 1;
                BLOCK *paux = pseq->pfirst->pprev;
                pseq->pfirst->pprev = pseq->pfirst->pprev->pprev;
                pseq->pfirst->pprev->pnext = NULL;

                free(paux);
                pseq->size_of_sequence--;
                inserted--;
            }
        }
        block = block->pnext;
    }

    //pthread_exit((void *)pall_sequences);
    return 0;
}

/**
 * is_current_assignment_consistent - Função responsável por verificar se a próxima peça a inserir na sequência é
 * consistente ao não. Se necessário a função is_current_assignment_consistent invoca funções auxiliares de forma
 * a inverter peças tornando a sequência consistente.
 * @param pseq - Apontador para a primeira peça da lista de sequência.
 * @param pnew_block - Apontador para a nova peça a inserir na sequencia.
 * @param inserted - Número de peças inseridas na sequencia.
 * @return (1/0) se a sequênia é ou não consistente com a nova peça.
 */
int is_current_assignment_consistent(SEQUENCE *pseq, BLOCK *pnew_block, short inserted)
{
    if (inserted == 0)
    {
        return ASSIGNMENT_CONSISTENT;
    }
    else if (pnew_block->left_side == pseq->pfirst->pprev->right_side)
    {
        return ASSIGNMENT_CONSISTENT;
    }
    else if (pnew_block->right_side == pseq->pfirst->pprev->right_side)
    {
        invert_block(pnew_block);
        return ASSIGNMENT_CONSISTENT;
    }
    else if (inserted == 1 && pnew_block->left_side == pseq->pfirst->left_side )
    {
        invert_block_sequence(pseq);
        return ASSIGNMENT_CONSISTENT;
    }
    else if (inserted == 1 && pnew_block->right_side == pseq->pfirst->left_side )
    {
        invert_block_sequence(pseq);
        invert_block(pnew_block);
        return ASSIGNMENT_CONSISTENT;
    }
    else
    {
        return ASSIGNMENT_FAILURE;
    }
}

/**
 * invert_block - Inverte uma peça.
 * @param pblock - Peça a inverter.
 */
void invert_block(BLOCK *pblock)
{
    short aux = 0;
    aux = pblock->left_side;
    pblock->left_side = pblock->right_side;
    pblock->right_side = aux;
}

/**
 * invert_block_sequence - Inverte a primeira peça da sequencia.
 * @param pseq - Apontador para a primeira peça da lista de sequência.
 */
void invert_block_sequence(SEQUENCE *pseq)
{
    short aux = 0;
    aux = pseq->pfirst->left_side;
    pseq->pfirst->left_side = pseq->pfirst->right_side;
    pseq->pfirst->right_side = aux;
}

/**
 * save_sequence - Função responsável por guardar uma sequencia gerada na estrutura de dados que armazena todas
 * as sequencias (ALLSEQUENCES).
 * Antes de a guardar a função converte a lista de peças da sequência numa string e armazena-a
 * como string.
 * @param pall_sequences - Apontador para a lista de todas as sequências guardadas.
 * @param pSeq - Apontador para a sequência a guardar.
 */
void save_sequence(SEQUENCE *pSeq)
{
    int i = 0, size_sequence = 0;

    SEQSTRING *newSequence = (SEQSTRING*)malloc(sizeof(SEQSTRING));
    newSequence->size_of_sequence = pSeq->size_of_sequence;
    newSequence->pnext = NULL;

    BLOCK *paux = pSeq->pfirst;

    char *sequence = (char *) malloc(sizeof(char) * (newSequence->size_of_sequence * 2) + 1);
    *sequence = '\0';
    //sprintf(sequence,"%lu-%s",)
    for (i = 0; i < newSequence->size_of_sequence && paux != NULL; i++)
    {
        size_sequence = strlen(sequence);
        *(sequence + size_sequence)     = '0' + paux->left_side;
        *(sequence + (size_sequence+1)) = '0' + paux->right_side;
        *(sequence + (size_sequence+2)) = '\0';
        paux = paux->pnext;
    }

    newSequence->sequence = sequence;

    sem_wait(&saveseq);
    pthread_mutex_lock(&mutex_p);
    prod=(prod+1) % S ;
    sprintf(allseq[prod].sequences,"%s",newSequence->sequence);
    sprintf(allseq[prod].thread_id,"%lu",pthread_self());
    countp++;
    pthread_mutex_unlock(&mutex_p);
    sem_post(&savefile);

}

void save_sequences_file(ALLSEQUENCES *pall_sequences)
{

    FILE *fp = NULL;
    SEQSTRING *pauxT;
    pauxT = pall_sequences->pfirst;

    fp = fopen(pall_sequences->path_file, "a");
    setbuf(fp, NULL);

    if (fp != NULL)
    {
        while (pauxT!=NULL)
        {
            fprintf(fp,"%d-%s\n",getpid(),pauxT->sequence);
            pauxT = pauxT->pnext;
        }
    }
    fclose(fp);

}

void freeList(SEQSTRING **pfirst)
{
    SEQSTRING *tmp;
    while (*pfirst != NULL)
    {
        tmp = *pfirst;
        *pfirst = (*pfirst)->pnext;

        free(tmp);
    }
}

/**
 * print_sequences - Imprime todas as sequências guardadas.
 * @param allSequences - Apontador para a lista de sequencias.
 */
void print_sequences(ALLSEQUENCES *allSequences)
{
    SEQSTRING *pSeq = allSequences->pfirst;
    while (pSeq != NULL)
    {
        printf("%s\n",pSeq->sequence);
        pSeq= pSeq->pnext;
    }
}

void *print_state(void *params, int handSize)
{
    pthread_mutex_t trinco=PTHREAD_MUTEX_INITIALIZER;
    unsigned i=0;
    pthread_mutex_lock(&trinco);
    for(i=0; i<handSize; i++)
    {
        if(thread_ids[i]==-1)
        {
            printf("Thread %d - id : %ld - esta ocupado\n",i,thread_ids[i]);
        }
        else if(thread_ids[i]!=-1)
        {
            printf("Thread %d - id : %ld - esta livre\n",i,thread_ids[i]);
        }
    }
    pthread_mutex_unlock(&trinco);

    return NULL;
}
