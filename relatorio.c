#include "relatorio.h"
#include "logica.h"
#include "matriz.h"
#include "renderizacao.h"
#include <stdio.h>

void exibir_estado_inicial(const Simulacao *simulacao, const OpcoesExecucao *opcoes,
                           int num_processos) {
    printf("Dimensoes: %d x %d | Geracoes: %d\n", simulacao->linhas, simulacao->colunas,
           simulacao->geracoes);
    printf("Modo: %s | Processos: %d\n", opcoes->deve_paralelizar ? "paralelo" : "sequencial",
           opcoes->deve_paralelizar ? num_processos : 1);
    printf("Estado inicial (t0) - Celulas vivas: %d\n",
           contar_celulas_vivas(simulacao->atual, simulacao->linhas, simulacao->colunas));
    if (opcoes->deve_imprimir_matrizes) {
        imprimir_matriz(simulacao->atual, simulacao->linhas, simulacao->colunas);
    }
}

void exibir_resultado_final(const Simulacao *simulacao, const OpcoesExecucao *opcoes,
                            double tempo_simulacao, int tempo_foi_medido) {
    printf("\nEstado final apos %d geracoes - Celulas vivas: %d\n", simulacao->geracoes,
           contar_celulas_vivas(simulacao->atual, simulacao->linhas, simulacao->colunas));
    if (tempo_foi_medido) {
        printf("Tempo da simulacao %s: %.9f s\n",
               opcoes->deve_paralelizar ? "paralela" : "sequencial", tempo_simulacao);
    } else if (opcoes->deve_animar) {
        printf("Tempo nao medido: desative --animar para avaliar o desempenho.\n");
    } else {
        printf("Tempo nao medido: falha ao consultar o relogio.\n");
    }
    if (!opcoes->deve_animar && opcoes->deve_imprimir_matrizes) {
        imprimir_matriz(simulacao->atual, simulacao->linhas, simulacao->colunas);
    }
}

int salvar_imagem_se_solicitada(const Simulacao *simulacao, const OpcoesExecucao *opcoes) {
    if (opcoes->caminho_saida == NULL) {
        return 1;
    }
    if (!salvar_matriz_pbm(opcoes->caminho_saida, simulacao->atual, simulacao->linhas,
                           simulacao->colunas)) {
        return 0;
    }
    printf("Imagem final salva em: %s\n", opcoes->caminho_saida);
    return 1;
}
