#include "paralelo.h"
#include "logica.h"
#include "matriz.h"
#include "particao.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <mpi.h>

#define TAG_CABECALHO 0
#define TAG_STATUS_LOCAL 1
#define TAG_STATUS_GLOBAL 2
#define TAG_DISTRIBUICAO 3
#define TAG_PARA_BAIXO 4
#define TAG_PARA_CIMA 5
#define TAG_RESULTADO 6

void iniciar_paralelo(int *argc, char ***argv, int *rank, int *num_processos) {
    MPI_Init(argc, argv);
    MPI_Comm_rank(MPI_COMM_WORLD, rank);
    MPI_Comm_size(MPI_COMM_WORLD, num_processos);
}

void finalizar_paralelo(void) {
    MPI_Finalize();
}

/*
    O processo 0 envia {linhas, colunas, geracoes} a todos; os demais recebem.
*/
static void distribuir_cabecalho(int rank, int num_processos, int cabecalho[3]) {
    MPI_Status status;

    if (rank == 0) {
        for (int destino = 1; destino < num_processos; destino++) {
            MPI_Send(cabecalho, 3, MPI_INT, destino, TAG_CABECALHO, MPI_COMM_WORLD);
        }
    } else {
        MPI_Recv(cabecalho, 3, MPI_INT, 0, TAG_CABECALHO, MPI_COMM_WORLD, &status);
    }
}

/*
    Cada processo informa ao processo 0 se teve sucesso na alocacaoo; o processo 0 devolve
    a todos o resultado global (1 somente se todos tiveram sucesso). Mantem o retorno
    consistente.
*/
static int confirmar_sucesso_global(int rank, int num_processos, int sucesso_local) {
    MPI_Status status;
    int sucesso_global = sucesso_local;

    if (rank == 0) {
        for (int origem = 1; origem < num_processos; origem++) {
            int sucesso_origem;
            MPI_Recv(&sucesso_origem, 1, MPI_INT, origem, TAG_STATUS_LOCAL, MPI_COMM_WORLD, &status);
            sucesso_global = sucesso_global && sucesso_origem;
        }
        for (int destino = 1; destino < num_processos; destino++) {
            MPI_Send(&sucesso_global, 1, MPI_INT, destino, TAG_STATUS_GLOBAL, MPI_COMM_WORLD);
        }
    } else {
        MPI_Send(&sucesso_local, 1, MPI_INT, 0, TAG_STATUS_LOCAL, MPI_COMM_WORLD);
        MPI_Recv(&sucesso_global, 1, MPI_INT, 0, TAG_STATUS_GLOBAL, MPI_COMM_WORLD, &status);
    }
    return sucesso_global;
}

/*
    O processo 0 envia a cada processo sua faixa de linhas e copia a propria para a matriz local.
    As faixas ocupam as linhas 1..quantidade da matriz local (as linhas 0 e quantidade + 1 sao de borda).
*/
static void distribuir_tabuleiro(int **tabuleiro, int **local, int linhas, int colunas, int rank, int num_processos, int inicio, int quantidade) {
    MPI_Status status;

    if (rank == 0) {
        memcpy(local[1], tabuleiro[inicio], (size_t)quantidade * (size_t)colunas * sizeof(int));
        for (int destino = 1; destino < num_processos; destino++) {
            int inicio_destino, quantidade_destino;
            calcular_faixa(linhas, num_processos, destino, &inicio_destino, &quantidade_destino);
            MPI_Send(tabuleiro[inicio_destino], quantidade_destino * colunas, MPI_INT, destino, TAG_DISTRIBUICAO, MPI_COMM_WORLD);
        }
    } else {
        MPI_Recv(local[1], quantidade * colunas, MPI_INT, 0, TAG_DISTRIBUICAO, MPI_COMM_WORLD, &status);
    }
}

/*
    Troca de bordas com o vizinho de rank maior: envia a ultima linha propria e recebe a
    primeira linha dele na linha de borda inferior.
*/
static void trocar_com_vizinho_abaixo(int **local, int quantidade, int colunas, int vizinho) {
    MPI_Status status;
    MPI_Send(local[quantidade], colunas, MPI_INT, vizinho, TAG_PARA_BAIXO, MPI_COMM_WORLD);
    MPI_Recv(local[quantidade + 1], colunas, MPI_INT, vizinho, TAG_PARA_CIMA, MPI_COMM_WORLD, &status);
}

/*
    Troca de bordas com o vizinho de rank menor: recebe a ultima linha dele na linha de borda
    superior e envia a primeira linha propria. Quem tem rank maior recebe primeiro.
*/
static void trocar_com_vizinho_acima(int **local, int colunas, int vizinho) {
    MPI_Status status;

    MPI_Recv(local[0], colunas, MPI_INT, vizinho, TAG_PARA_BAIXO, MPI_COMM_WORLD, &status);
    MPI_Send(local[1], colunas, MPI_INT, vizinho, TAG_PARA_CIMA, MPI_COMM_WORLD);
}

/*
    Atualiza as linhas de borda da matriz local com as linhas dos processos vizinhos.
    Ranks pares fazem primeiro a troca com o vizinho de baixo e depois com o de cima; os
    impares fazem o contrario. Assim em cada par de vizinhos um envia enquanto o outro recebe,
    sem deadlock. As bordas do tabuleiro (sem vizinho) permanecem zeradas (celulas mortas).
*/
static void trocar_bordas(int **local, int quantidade, int colunas, int rank, int num_processos) {
    int tem_acima = rank > 0;
    int tem_abaixo = rank < num_processos - 1;

    if (rank % 2 == 0) {
        if (tem_abaixo) trocar_com_vizinho_abaixo(local, quantidade, colunas, rank + 1);
        if (tem_acima) trocar_com_vizinho_acima(local, colunas, rank - 1);
    } else {
        if (tem_acima) trocar_com_vizinho_acima(local, colunas, rank - 1);
        if (tem_abaixo) trocar_com_vizinho_abaixo(local, quantidade, colunas, rank + 1);
    }
}

/*
    Os processos enviam suas faixas ao processo 0, que monta o tabuleiro final em 'tabuleiro'.
*/
static void coletar_resultado(int **tabuleiro, int **local, int linhas, int colunas, int rank, int num_processos, int inicio, int quantidade) {
    MPI_Status status;

    if (rank == 0) {
        memcpy(tabuleiro[inicio], local[1], (size_t)quantidade * (size_t)colunas * sizeof(int));
        for (int origem = 1; origem < num_processos; origem++) {
            int inicio_origem, quantidade_origem;
            calcular_faixa(linhas, num_processos, origem, &inicio_origem, &quantidade_origem);
            MPI_Recv(tabuleiro[inicio_origem], quantidade_origem * colunas, MPI_INT, origem, TAG_RESULTADO, MPI_COMM_WORLD, &status);
        }
    } else {
        MPI_Send(local[1], quantidade * colunas, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
    }
}

int simular_paralelo(int ***matriz_atual, int ***matriz_proxima, int linhas, int colunas, int geracoes, int rank, int num_processos) {
    int cabecalho[3] = {linhas, colunas, geracoes};
    int inicio;
    int quantidade;
    int **local_atual = NULL;
    int **local_proxima = NULL;

    (void)matriz_proxima;

    /* Todos conhecem as dimensoes; linhas = 0 enviado pelo processo 0 significa cancelamento */
    distribuir_cabecalho(rank, num_processos, cabecalho);
    linhas = cabecalho[0];
    colunas = cabecalho[1];
    geracoes = cabecalho[2];

    if (linhas <= 0 || colunas <= 0 || geracoes < 0) {
        return 0;
    }
    if (!calcular_faixa(linhas, num_processos, rank, &inicio, &quantidade)) {
        if (rank == 0) {
            fprintf(stderr, "Erro: o numero de processos (%d) excede o numero de linhas (%d).\n", num_processos, linhas);
        }
        return 0;
    }
    if ((long long)((linhas + num_processos - 1) / num_processos) * colunas > INT_MAX) {
        if (rank == 0) {
            fprintf(stderr, "Erro: faixa de linhas grande demais para uma mensagem MPI.\n");
        }
        return 0;
    }

    /* Matrizes locais: linha 0 e linha quantidade + 1 sao as bordas (vizinhos ou fora do tabuleiro) */
    local_atual = alocar_matriz(quantidade + 2, colunas);
    local_proxima = alocar_matriz(quantidade + 2, colunas);

    if (!confirmar_sucesso_global(rank, num_processos, local_atual != NULL && local_proxima != NULL)) {
        liberar_matriz(local_atual, quantidade + 2);
        liberar_matriz(local_proxima, quantidade + 2);
        return 0;
    }

    /* Comunicacao inicial: cada processo recebe sua faixa */
    distribuir_tabuleiro(rank == 0 ? *matriz_atual : NULL, local_atual, linhas, colunas, rank, num_processos, inicio, quantidade);

    MPI_Barrier(MPI_COMM_WORLD);

    /* Geracoes: somente as bordas sao trocadas entre processos a cada geracao */
    for (int geracao = 0; geracao < geracoes; geracao++) {
        trocar_bordas(local_atual, quantidade, colunas, rank, num_processos);
        calcular_proxima_geracao(local_atual, local_proxima, quantidade + 2, colunas, 1, quantidade + 1);
        trocar_matrizes(&local_atual, &local_proxima);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /* Comunicacao final: o processo 0 reune as faixas em *matriz_atual */
    coletar_resultado(rank == 0 ? *matriz_atual : NULL, local_atual, linhas, colunas, rank, num_processos, inicio, quantidade);

    MPI_Barrier(MPI_COMM_WORLD);

    liberar_matriz(local_atual, quantidade + 2);
    liberar_matriz(local_proxima, quantidade + 2);
    return 1;
}
