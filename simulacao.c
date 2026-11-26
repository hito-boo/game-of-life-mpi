#include "simulacao.h"
#include "entrada.h"
#include "logica.h"
#include "matriz.h"
#include "paralelo.h"
#include "renderizacao.h"
#include "tempo.h"
#include <stdio.h>
#include <string.h>

void liberar_simulacao(Simulacao *simulacao) {
    liberar_matriz(simulacao->atual, simulacao->linhas);
    liberar_matriz(simulacao->proxima, simulacao->linhas);
    memset(simulacao, 0, sizeof(*simulacao));
}

int preparar_simulacao(const char *caminho_arquivo, Simulacao *simulacao) {
    memset(simulacao, 0, sizeof(*simulacao));

    // Le o arquivo e ja obtem a matriz atual (t0) preenchida e os parametros n, m e geracoes
    simulacao->atual = ler_arquivo_entrada(caminho_arquivo, &simulacao->linhas, &simulacao->colunas, &simulacao->geracoes);
    if (simulacao->atual == NULL) {
        memset(simulacao, 0, sizeof(*simulacao));
        return 0;
    }

    // Cria a matriz da proxima geracao com as mesmas dimensoes
    simulacao->proxima = alocar_matriz(simulacao->linhas, simulacao->colunas);
    if (simulacao->proxima == NULL) {
        liberar_simulacao(simulacao);
        return 0;
    }
    return 1;
}

void executar_simulacao_sequencial(Simulacao *simulacao, double *tempo, int *tempo_medido) {
    double inicio;
    double fim;
    int relogio_disponivel = obter_tempo_monotonico(&inicio);

    simular_sequencial(&simulacao->atual, &simulacao->proxima, simulacao->linhas, simulacao->colunas, simulacao->geracoes);

    *tempo_medido = 0;
    if (relogio_disponivel && obter_tempo_monotonico(&fim)) {
        *tempo = fim - inicio;
        *tempo_medido = 1;
    }
}

int executar_simulacao_paralela(Simulacao *simulacao, int rank, int num_processos, double *tempo, int *tempo_medido) {
    double inicio = 0.0;
    double fim;
    int relogio_disponivel = 0;
    *tempo_medido = 0;

    if (rank == 0) {
        relogio_disponivel = obter_tempo_monotonico(&inicio);
    }

    if (!simular_paralelo(&simulacao->atual, &simulacao->proxima, simulacao->linhas, simulacao->colunas, simulacao->geracoes, rank, num_processos)) {
        return 0;
    }

    if (rank == 0 && relogio_disponivel && obter_tempo_monotonico(&fim)) {
        *tempo = fim - inicio;
        *tempo_medido = 1;
    }
    return 1;
}

int simular_com_animacao(Simulacao *simulacao, unsigned int atraso_milissegundos) {
    int n = simulacao->linhas;
    int m = simulacao->colunas;

    if (!iniciar_animacao_terminal(n, m)) {
        return 0;
    }

    renderizar_geracao_terminal(
        simulacao->atual, n, m, 0, simulacao->geracoes,
        contar_celulas_vivas(simulacao->atual, n, m)
    );

    for (int geracao = 0; geracao < simulacao->geracoes; geracao++) {
        aguardar_proximo_quadro(atraso_milissegundos);
        avancar_uma_geracao(&simulacao->atual, &simulacao->proxima, n, m);
        renderizar_geracao_terminal(
            simulacao->atual, n, m, geracao + 1, simulacao->geracoes,
            contar_celulas_vivas(simulacao->atual, n, m)
        );
    }

    finalizar_animacao_terminal();
    return 1;
}

int verificar_resultado(const Simulacao *simulacao, int **referencia, int **auxiliar) {
    simular_sequencial(&referencia, &auxiliar, simulacao->linhas, simulacao->colunas, simulacao->geracoes);

    if (matrizes_iguais(simulacao->atual, referencia, simulacao->linhas, simulacao->colunas)) {
        printf("Verificacao: OK - resultados sequencial e paralelo identicos.\n");
        return 1;
    }
    fprintf(stderr, "Verificacao: FALHOU - resultados sequencial e paralelo diferentes.\n");
    return 0;
}
