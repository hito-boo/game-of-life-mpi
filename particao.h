#ifndef PARTICAO_H
#define PARTICAO_H

/*
    Calcula a faixa de linhas consecutivas do processo 'rank' ao dividir 'linhas' linhas entre
    'num_processos' processos (as primeiras linhas % num_processos faixas recebem uma linha a mais).
    Preenche a primeira linha da faixa e a quantidade de linhas e retorna 1; retorna 0 se os
    parametros forem invalidos (inclusive num_processos > linhas).
*/
int calcular_faixa(int linhas, int num_processos, int rank, int *linha_inicio, int *quantidade_linhas);

#endif
