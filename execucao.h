#ifndef EXECUCAO_H
#define EXECUCAO_H

/*
    Executa o programa completo em cada processo: argumentos, modo sequencial ou paralelo,
    saida e verificacao. Retorna o codigo de saida do processo.
*/
int executar_programa(int argc, char *argv[], int rank, int num_processos);

#endif