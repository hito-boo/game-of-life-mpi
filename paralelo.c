#include "paralelo.h"
#include "logica.h"
#include "matriz.h"
#include "particao.h"
#include <stdio.h>

/* TODO: incluir <mpi.h> */

void iniciar_paralelo(int *argc, char ***argv, int *rank, int *num_processos) {
    /* TODO: MPI_Init, MPI_Comm_rank e MPI_Comm_size */
    (void)argc;
    (void)argv;
    *rank = 0;
    *num_processos = 1;
}

void finalizar_paralelo(void) {
    /* TODO: MPI_Finalize */
}

int simular_paralelo(int ***matriz_atual, int ***matriz_proxima, int linhas, int colunas, int geracoes, int rank, int num_processos) {
    /* TODO: implementar a simulacao com MPI */
    (void)matriz_atual;
    (void)matriz_proxima;
    (void)linhas;
    (void)colunas;
    (void)geracoes;
    (void)num_processos;

    if (rank == 0) {
        fprintf(stderr, "Erro: simular_paralelo ainda nao foi implementada.\n");
    }
    return 0;
}
