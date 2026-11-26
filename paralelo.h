#ifndef PARALELO_H
#define PARALELO_H

/*
    Inicializa o MPI e informa o identificador (rank) deste processo e o numero total de processos.
*/
void iniciar_paralelo(int *argc, char ***argv, int *rank, int *num_processos);

/*
    Encerra o MPI.
*/
void finalizar_paralelo(void);

/*
    Executa o Jogo da Vida de Conway com MPI. Chamada por todos os processos.
    No processo 0, as matrizes e as dimensoes sao as lidas do arquivo e, ao final, *matriz_atual
    contem o estado final completo. Nos demais processos as matrizes sao NULL e as dimensoes 0.
    Retorna 1 em caso de sucesso e 0 em caso de erro.
*/
int simular_paralelo(int ***matriz_atual, int ***matriz_proxima, int linhas, int colunas, int geracoes, int rank, int num_processos);

#endif
