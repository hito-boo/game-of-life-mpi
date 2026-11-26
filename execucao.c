#include "execucao.h"
#include "matriz.h"
#include "opcoes.h"
#include "relatorio.h"
#include "simulacao.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
    Modo sequencial: apenas o processo 0 trabalha.
*/
static int executar_modo_sequencial(const OpcoesExecucao *opcoes, int rank, int num_processos) {
    if (rank != 0) {
        return EXIT_SUCCESS;
    }
    if (num_processos > 1) {
        fprintf(stderr, "Aviso: modo sequencial usa apenas o processo 0.\n");
    }

    Simulacao simulacao;
    double tempo_simulacao = 0.0;
    int tempo_foi_medido = 0;
    int codigo_saida = EXIT_FAILURE;

    if (!preparar_simulacao(opcoes->caminho_arquivo, &simulacao)) {
        return EXIT_FAILURE;
    }

    if (opcoes->deve_animar) {
        if (!simular_com_animacao(&simulacao, opcoes->atraso_milissegundos)) {
            goto liberar_recursos;
        }
    } else {
        exibir_estado_inicial(&simulacao, opcoes, num_processos);
        executar_simulacao_sequencial(&simulacao, &tempo_simulacao, &tempo_foi_medido);
    }

    exibir_resultado_final(&simulacao, opcoes, tempo_simulacao, tempo_foi_medido);
    if (!salvar_imagem_se_solicitada(&simulacao, opcoes)) {
        goto liberar_recursos;
    }

    codigo_saida = EXIT_SUCCESS;
liberar_recursos:
    liberar_simulacao(&simulacao);
    return codigo_saida;
}

/*
    Modo paralelo: todos os processos chamam simular_paralelo. Somente o processo 0 le o
    arquivo, imprime, verifica e salva a imagem. Se a leitura falhar, o processo 0 chama
    simular_paralelo com a simulacao zerada (linhas = 0) para sinalizar o cancelamento.
*/
static int executar_modo_paralelo(const OpcoesExecucao *opcoes, int rank, int num_processos) {
    Simulacao simulacao;
    int **referencia = NULL;
    int **auxiliar_referencia = NULL;
    double tempo_simulacao = 0.0;
    int tempo_foi_medido = 0;
    int preparado = 0;
    int codigo_saida = EXIT_FAILURE;

    memset(&simulacao, 0, sizeof(simulacao));

    if (rank == 0) {
        preparado = preparar_simulacao(opcoes->caminho_arquivo, &simulacao);
        if (preparado && opcoes->deve_verificar) {
            referencia = alocar_matriz(simulacao.linhas, simulacao.colunas);
            auxiliar_referencia = alocar_matriz(simulacao.linhas, simulacao.colunas);
            if (referencia == NULL || auxiliar_referencia == NULL) {
                preparado = 0;
                liberar_simulacao(&simulacao);
            } else {
                copiar_matriz(referencia, simulacao.atual, simulacao.linhas, simulacao.colunas);
            }
        }
        if (preparado) {
            exibir_estado_inicial(&simulacao, opcoes, num_processos);
        }
    }

    int sucesso = executar_simulacao_paralela(&simulacao, rank, num_processos,
                                              &tempo_simulacao, &tempo_foi_medido);

    if (rank != 0) {
        return sucesso ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    if (!preparado || !sucesso) {
        goto liberar_recursos;
    }

    exibir_resultado_final(&simulacao, opcoes, tempo_simulacao, tempo_foi_medido);

    if (opcoes->deve_verificar
        && !verificar_resultado(&simulacao, referencia, auxiliar_referencia)) {
        goto liberar_recursos;
    }
    if (!salvar_imagem_se_solicitada(&simulacao, opcoes)) {
        goto liberar_recursos;
    }

    codigo_saida = EXIT_SUCCESS;
liberar_recursos:
    liberar_matriz(referencia, simulacao.linhas);
    liberar_matriz(auxiliar_referencia, simulacao.linhas);
    liberar_simulacao(&simulacao);
    return codigo_saida;
}

int executar_programa(int argc, char *argv[], int rank, int num_processos) {
    if (pediu_ajuda(argc, argv)) {
        if (rank == 0) {
            exibir_ajuda(argv[0]);
        }
        return EXIT_SUCCESS;
    }

    OpcoesExecucao opcoes;
    if (!processar_argumentos(argc, argv, rank, &opcoes)) {
        return EXIT_FAILURE;
    }

    if (opcoes.deve_paralelizar) {
        return executar_modo_paralelo(&opcoes, rank, num_processos);
    }
    return executar_modo_sequencial(&opcoes, rank, num_processos);
}
