#include "particao.h"

int calcular_faixa(int linhas, int num_processos, int rank, int *linha_inicio, int *quantidade_linhas) {
    if (linhas <= 0 || num_processos <= 0 || num_processos > linhas
        || rank < 0 || rank >= num_processos) {
        return 0;
    }

    int base = linhas / num_processos;
    int resto = linhas % num_processos;

    *quantidade_linhas = base + (rank < resto ? 1 : 0);
    *linha_inicio = rank * base + (rank < resto ? rank : resto);
    return 1;
}
