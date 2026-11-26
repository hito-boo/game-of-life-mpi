#include "paralelo.h"
#include "logica.h"
#include "matriz.h"
#include "particao.h"
#include <stdio.h>
#include <mpi.h>

void iniciar_paralelo(int *argc, char ***argv, int *rank, int *num_processos) {
    MPI_Init(argc, argv);
    MPI_Comm_rank(MPI_COMM_WORLD, rank);
    MPI_Comm_size(MPI_COMM_WORLD, num_processos);
}

void finalizar_paralelo(void) {
    // Liberar memoria
    MPI_Finalize();
}

int simular_paralelo(int ***matriz_atual, int ***matriz_proxima, int linhas, int colunas, int geracoes, int rank, int num_processos) {
    MPI_Status status;
    int inicio, count, final;
    calcular_faixa(linhas, num_processos, rank, &inicio, &count);
    final = inicio + count;

    if (rank != 0) {
        // Aloca buffer para receber dados e para a resposta
        int *buffer_entrada = alocar_matriz(count + 2, colunas);
        int *buffer_saida = alocar_matriz(count, colunas);
    }

    while (geracoes > 0) {

        if (rank == 0) {
            // Envio de cada parte a cada processo
            for (int rank_p = 1; rank_p < num_processos; rank_p++) {
                int inicio_p, count_p;
                calcular_faixa(linhas, num_processos, rank_p, &inicio_p, &count_p);
                MPI_Send(matriz_atual[inicio_p - 1], count_p + 2, MPI_INT, rank_p, rank_p, MPI_COMM_WORLD);
            }

            // Calcula a sua parte
            calcular_proxima_geracao(matriz_atual, matriz_proxima, linhas, colunas, inicio, final);
            
            // Espera todos terminarem
            MPI_Barrier(MPI_COMM_WORLD);

            // Coleta e troca de matrizes
            for (int rank_p = 1; rank_p < num_processos; rank_p++) {
                int inicio_p, count_p;
                calcular_faixa(linhas, num_processos, rank_p, &inicio_p, &count_p);
                MPI_Recv(matriz_proxima[inicio_p], count_p, MPI_INT, rank_p, rank_p, MPI_COMM_WORLD, &status);
            }

            trocar_matrizes(matriz_atual, matriz_proxima);

            MPI_Barrier(MPI_COMM_WORLD);

        } else {
            // Recebe dados
            MPI_Recv(&buffer_entrada, count_p + 2, MPI_INT, rank_p, rank_p, MPI_COMM_WORLD, &status);
            
            // Calcula a sua parte
            calcular_proxima_geracao(&buffer_entrada, &buffer_saida, linhas, colunas, 1, count);

            // Envia resultado
            MPI_Send(&buffer_saida, count_p, MPI_INT, rank_p, rank_p, MPI_COMM_WORLD);

            MPI_Barrier(MPI_COMM_WORLD);

            // Espera o recebimento e a troca de matrizes
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }

    if (rank != 0) {
        // Libera buffer de entrada e de saida
        liberar_matriz(buffer_entrada, count + 2);
        liberar_matriz(buffer_saida, count);
    }

    return 1;
}
