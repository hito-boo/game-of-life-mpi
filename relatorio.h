#ifndef RELATORIO_H
#define RELATORIO_H

#include "opcoes.h"
#include "simulacao.h"

/*
    Exibe dimensoes, modo, processos e o estado inicial (t0).
*/
void exibir_estado_inicial(const Simulacao *simulacao, const OpcoesExecucao *opcoes,
                           int num_processos);

/*
    Exibe o estado final, o tempo medido e, se habilitado, a matriz final.
    O formato da linha de tempo e lido por scripts/benchmark.py.
*/
void exibir_resultado_final(const Simulacao *simulacao, const OpcoesExecucao *opcoes,
                            double tempo_simulacao, int tempo_foi_medido);

/*
    Salva a imagem PBM final quando --saida foi informado. Retorna 1 em sucesso.
*/
int salvar_imagem_se_solicitada(const Simulacao *simulacao, const OpcoesExecucao *opcoes);

#endif
