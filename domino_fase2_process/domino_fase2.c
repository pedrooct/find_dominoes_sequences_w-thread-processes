//
// Created by Pedro Costa Nº31179 , Paulo Bento Nº 33959 on 09-03-2017.
//

#include "domino_fase2.h"
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

struct timeval end_time, end_write; // variaveis globais para uardar o tempo

void handler_record_time(int end) // handle para guardar os tempo
{
   switch(end){
        case SIGUSR1:
            gettimeofday(&end_time,NULL);
            break;
        case SIGUSR2:
            gettimeofday(&end_write,NULL);
            break;
            }
}

int main(int argc, const char * argv[])
{
    struct sigaction sa;
    sigset_t block_mask;
    unsigned int tts;

    struct timeval tfather,tfather_end;

    gettimeofday(&tfather, NULL);

    //Vars processo
    int pid[28]= {};
    int save;
    int i=0;

    char buff[PIPE_BUF];
    srand((unsigned)time(NULL));

    memset (&sa, 0, sizeof (sa));
    sa.sa_handler = &handler_record_time;
	sa.sa_flags = SA_RESTART;
    sigaddset (&block_mask, SIGINT);
    sa.sa_mask = block_mask;



    sigaction (SIGUSR1, &sa, NULL);
    sigaction (SIGUSR2, &sa, NULL);


    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char fname[64];
    strftime(fname, sizeof(fname), "%a %b %d %Hh %Y",tm);

    char path_save_file[150] = "./data/"; //Caminho para a pasta de dados.
    char *path_file_hand = "./data/mao14P20832S.txt";

    strcat(path_save_file,fname);
    strcat(path_save_file,".txt");
    strcat(path_save_file,"\0");

    save = open(path_save_file, O_WRONLY| O_CREAT | O_TRUNC | O_APPEND, 0666);
    if(save==-1)
    {
        perror ("Opening Destination File");
        exit(1);
    }

    DECK *deck = NULL;
    deck = (DECK *) malloc(sizeof(DECK));
    deck->pfirst = NULL;
    deck->num_blocks_available = NUM_BLOCKS;

    //Inicialização do baralho
    deck->pfirst = create_deck(deck->pfirst);

    //Mão do jogador
    HAND *hand;
    hand = (HAND *) malloc(sizeof(HAND));
    hand->pfirst=NULL;
    hand->hand_size=0;

    //hand = create_hand(hand,deck,10);
    hand = read_hand_from_file(hand, path_file_hand);
    print_hand(hand);

    //Lista de sequência de peças
    SEQUENCE *seq;
    seq = (SEQUENCE *) malloc(sizeof(SEQUENCE));
    seq->pfirst = NULL;
    seq->size_of_sequence = 0;

    //Lista com todas as sequências guardadas
    ALLSEQUENCES *all_sequences;
    all_sequences = (ALLSEQUENCES *) malloc(sizeof(ALLSEQUENCES));
    all_sequences->pfirst = NULL;
    all_sequences->path_file = (char *) malloc(sizeof(char)*150);
    strcpy(all_sequences->path_file, path_save_file);
    all_sequences->number_of_sequences = 0;

    short inserted=0;
    BLOCK * peca=NULL;
    peca=hand->pfirst;
    //atribuir mão


    //criar pipe

    int pipe_talk[2]; // array de pipe
    pipe(pipe_talk); // criar o pipe

    for(i=0; i<hand->hand_size; i++)
    {
        BLOCK *bdummie = (BLOCK *)malloc(sizeof(BLOCK));
        bdummie->left_side=peca->left_side;
        bdummie->right_side=peca->right_side;
        bdummie->available=0;
        bdummie->pprev=NULL;
        bdummie->pnext=NULL;


        if(inserted==0)
        {
            seq->pfirst=bdummie;
            seq->pfirst->pnext=NULL;
            seq->pfirst->pprev=bdummie;
        }
        inserted++;
        seq->size_of_sequence++;
        peca->available=0;

        if((pid[i]=fork())==-1)
        {
            perror("fork");
            exit(1);
        }
        //criar filhos
        if(pid[i]==0)
        {
            close(pipe_talk[0]); // fechar canal de leitura nos filhos
            int ppid= getppid();
            recursive_backtrack(hand,seq, all_sequences, inserted);
            printf("PID %d # %ld\n",getpid(),all_sequences->number_of_sequences);
            kill(ppid,SIGUSR1);
            // inicio de escrita para o pipe;
            SEQSTRING *paux;
            paux = all_sequences->pfirst;
            while(paux!=NULL)
            {
                    sprintf(buff,"%d#%s#\n",getpid(),paux->sequence); // parseInt para colocar a sequencia no protocolo
                    write(pipe_talk[1], buff, (strlen(buff))); // envio para o canal do pipe[1]
                    paux=paux->pnext;
            }
            close(pipe_talk[1]); // no fim fechamos canal de escrita nos filhos para retornar 0
            exit(0);
        }
        peca->available=1;
        BLOCK *paux=seq->pfirst->pprev;
        seq->pfirst->pprev=seq->pfirst->pprev->pprev;
        seq->pfirst->pprev->pnext=NULL;

        free(paux);
        seq->size_of_sequence--;
        inserted--;
        peca=peca->pnext;
    }
    close(pipe_talk[1]); // fechar canal do pai de escrita , visto que este só lê
    save_file_sequences_write(all_sequences,save,pipe_talk[0]); // função para ler dos pipes e escrever para os ficheiros
    for(i=0;i<hand->hand_size;i++)
    {
        waitpid(pid[i],NULL,0);
    }
    close(pipe_talk[0]); // no fim fechar canal de leitura
    gettimeofday(&tfather_end, NULL);
    printf("time of recursive_bactrack = %f seconds\n",(double) (end_time.tv_usec - tfather.tv_usec) / 1000000 +(double) (end_time.tv_sec - tfather.tv_sec));
    printf("time of pipe comunication = %f seconds\n",(double) (tfather_end.tv_usec - end_time.tv_usec) / 1000000 +(double) (tfather_end.tv_sec - end_time.tv_sec));
    printf("father PID %d# total time = %f seconds\n",getpid(),(double) (tfather_end.tv_usec - tfather.tv_usec) / 1000000 +(double) (tfather_end.tv_sec - tfather.tv_sec));
    return 0;
}

void save_file_sequences_write(ALLSEQUENCES * pall_sequences, int save,int pipe)
{
    int bytes;
    char readBuffer[PIPE_BUF]; // PIPE_BUF é a mecro com o valor em kb do pipe , ou seja , 4096KB
    SEQSTRING *paux;
    paux=pall_sequences->pfirst;
    while(bytes = read(pipe, readBuffer, sizeof(readBuffer)!=0)) // enquanto bytes não for igual a 0 significa que existe conteudo no pipe
    { // le do pipe, que é enviado por parametro e coloca no readBuffer
        write(save,readBuffer,strlen(readBuffer));// escreve para o ficheiro a totalidade do pipe
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
int recursive_backtrack(HAND *phand, SEQUENCE *pseq, ALLSEQUENCES *pall_sequences, short inserted)
{

    int i = 0;
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
                    save_sequence(pall_sequences, pseq);
                    pall_sequences->number_of_sequences++;
                }


                recursive_backtrack(phand, pseq, pall_sequences, inserted);

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
void save_sequence(ALLSEQUENCES *pall_sequences, SEQUENCE *pSeq)
{

    int i = 0, size_sequence = 0;

    SEQSTRING *newSequence = (SEQSTRING*)malloc(sizeof(SEQSTRING));
    newSequence->size_of_sequence = pSeq->size_of_sequence;
    newSequence->pnext = NULL;

    BLOCK *paux = pSeq->pfirst;

    char *sequence = (char *) malloc(sizeof(char) * (newSequence->size_of_sequence * 2) + 1);
    *sequence = '\0';

    for (i = 0; i < newSequence->size_of_sequence && paux != NULL; i++)
    {
        size_sequence = strlen(sequence);
        *(sequence + size_sequence)     = '0' + paux->left_side;
        *(sequence + (size_sequence+1)) = '0' + paux->right_side;
        *(sequence + (size_sequence+2)) = '\0';
        paux = paux->pnext;
    }

    newSequence->sequence = sequence;

    if(pall_sequences->pfirst == NULL)
    {
        pall_sequences->pfirst = newSequence;
        printf("%s", " pall_sequences->pfirst = newSequence;\n");
    }
    else
    {
        newSequence->pnext = pall_sequences->pfirst;
        pall_sequences->pfirst = newSequence;
    }

    /*if(pall_sequences->number_of_sequences%MEMORY_MAX_SEQUENCES == 0 && pall_sequences->number_of_sequences > 0)
    {
        save_sequences_file(pall_sequences);
        freeList(&pall_sequences->pfirst);

    }*/

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
