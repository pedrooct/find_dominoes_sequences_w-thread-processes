//
// Created by Pedro Costa Nº31179 , Paulo Bento Nº 33959 on 09-03-2017.
//

#include "domino_fase3.h"
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <unistd.h>
#include <limits.h>
#include <syscall.h>
#include <stropts.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define SERVER_PORT 10007

int main(int argc, const char * argv[])
{
    int c, s, bytes;
    char buf[PIPE_BUFF];			/* buffer for incoming file */


    recursive_backtrack(hand,seq, all_sequences, inserted);
    printf("PID %d # %ld\n",getpid(),all_sequences->number_of_sequences);

    struct hostent *h;			/* info about server */
    struct sockaddr_in channel;		/* holds IP address */

    if (argc != 3) {
        printf("Usage: domino_client file-name\n");
        exit(-1);
    }

    h = gethostbyname(argv[1]);		/* look up host's IP addrmm,,ess */
    if (!h) {
        perror("gethostbyname");
        exit(-1);
    }


    s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s < 0) {
        perror("socket");
        exit(-1);
    }

    memset(&channel, 0, sizeof(channel));
    channel.sin_family= AF_INET;
    memcpy(&channel.sin_addr.s_addr, h->h_addr, h->h_length);
    channel.sin_port= htons(SERVER_PORT);

    c = connect(s, (struct sockaddr *) &channel, sizeof(channel));
    if (c < 0) {
        perror("connect");
        exit(-1);
    }

    save_file_sequences_write(all_sequences,s);


    exit(0);
}

void save_file_sequences_write(ALLSEQUENCES * pall_sequences, int save,int pipe)
{
    int bytes;
    char readBuffer[PIPE_BUF];
    SEQSTRING *paux;
    paux=pall_sequences->pfirst;
    while(bytes = read(pipe, readBuffer, sizeof(readBuffer)!=0))
    {
        write(save,readBuffer,strlen(readBuffer));
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

    if(pall_sequences->number_of_sequences%MEMORY_MAX_SEQUENCES == 0 && pall_sequences->number_of_sequences > 0)
    {
        save_sequences_file(pall_sequences);
        freeList(&pall_sequences->pfirst);

    }

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
