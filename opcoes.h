#ifndef OPCOES_H
#define OPCOES_H

typedef struct {
    const char *caminho_arquivo;
    const char *caminho_saida;
    int deve_imprimir_matrizes;
    int deve_animar;
    unsigned int atraso_milissegundos;
    int deve_paralelizar; /* Seleciona o calculo com MPI (padrao). */
    int deve_verificar;   /* Executa a outra versao e compara todas as celulas. */
} OpcoesExecucao;

/*
    Informa se --ajuda ou -h foi passado.
*/
int pediu_ajuda(int argc, char *argv[]);

/*
    Exibe as formas de uso e as opcoes disponiveis no programa.
*/
void exibir_ajuda(const char *nome_programa);

/*
    Processa os argumentos da linha de comando e preenche as opcoes. Retorna 1
    quando os argumentos sao validos e 0 quando encontra algum erro (mensagem
    exibida apenas pelo processo 0).
*/
int processar_argumentos(int argc, char *argv[], int rank, OpcoesExecucao *opcoes);

#endif
