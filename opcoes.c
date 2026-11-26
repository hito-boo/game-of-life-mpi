#include "opcoes.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ATRASO_PADRAO_MS 200U
#define ATRASO_MAXIMO_MS 60000UL

/*
    Imprime uma mensagem de erro apenas no processo 0, para nao repeti-la em cada processo.
*/
static void relatar_erro(int rank, const char *formato, ...) {
    if (rank != 0) {
        return;
    }
    va_list argumentos;
    va_start(argumentos, formato);
    vfprintf(stderr, formato, argumentos);
    va_end(argumentos);
}

int pediu_ajuda(int argc, char *argv[]) {
    for (int indice = 1; indice < argc; indice++) {
        if (strcmp(argv[indice], "--ajuda") == 0 || strcmp(argv[indice], "-h") == 0) {
            return 1;
        }
    }
    return 0;
}

void exibir_ajuda(const char *nome_programa) {
    printf("Uso:\n");
    printf("  mpirun -np N %s <arquivo_de_entrada> [opcoes]\n", nome_programa);
    printf("  %s <arquivo_de_entrada> --sequencial [opcoes]\n", nome_programa);
    printf("  %s --ajuda\n\n", nome_programa);
    printf("Opcoes:\n");
    printf("  --paralelo      Executa com MPI usando os N processos de mpirun (padrao).\n");
    printf("  --sequencial    Executa sem MPI, apenas no processo 0.\n");
    printf("  --verificar     Compara paralelo e sequencial fora do intervalo cronometrado.\n");
    printf("  --sem-impressao  Nao exibe as matrizes inicial e final fora da animacao.\n");
    printf("  --animar         Exibe todas as geracoes no terminal (modo sequencial).\n");
    printf("  --atraso MS      Define o intervalo da animacao (0 a 60000 ms).\n");
    printf("  --saida ARQUIVO  Salva o estado final como uma imagem PBM.\n");
    printf("  --ajuda, -h      Exibe esta mensagem.\n");
}

/*
    Converte o texto do atraso para um valor valido em milissegundos.
*/
static int converter_atraso(const char *texto, unsigned int *atraso_milissegundos) {
    errno = 0;
    char *fim_conversao;
    unsigned long valor = strtoul(texto, &fim_conversao, 10);

    if (texto == fim_conversao || *fim_conversao != '\0'
        || errno == ERANGE || valor > ATRASO_MAXIMO_MS) {
        return 0;
    }

    *atraso_milissegundos = (unsigned int)valor;
    return 1;
}

/*
    Le os argumentos e preenche as opcoes, validando a combinacao entre elas.
*/
static int analisar_argumentos(int argc, char *argv[], int rank, OpcoesExecucao *opcoes) {
    opcoes->caminho_arquivo = NULL;
    opcoes->caminho_saida = NULL;
    opcoes->deve_imprimir_matrizes = 1;
    opcoes->deve_animar = 0;
    opcoes->atraso_milissegundos = ATRASO_PADRAO_MS;
    opcoes->deve_paralelizar = 1;
    opcoes->deve_verificar = 0;
    int sequencial_explicito = 0;
    int paralelo_explicito = 0;

    for (int indice = 1; indice < argc; indice++) {
        if (strcmp(argv[indice], "--sequencial") == 0) {
            sequencial_explicito = 1;
        } else if (strcmp(argv[indice], "--paralelo") == 0) {
            paralelo_explicito = 1;
        } else if (strcmp(argv[indice], "--verificar") == 0) {
            opcoes->deve_verificar = 1;
        } else if (strcmp(argv[indice], "--sem-impressao") == 0) {
            opcoes->deve_imprimir_matrizes = 0;
        } else if (strcmp(argv[indice], "--animar") == 0) {
            opcoes->deve_animar = 1;
        } else if (strcmp(argv[indice], "--atraso") == 0) {
            if (indice + 1 >= argc
                || !converter_atraso(argv[indice + 1], &opcoes->atraso_milissegundos)) {
                relatar_erro(rank, "Erro: --atraso exige um valor inteiro entre 0 e 60000.\n");
                return 0;
            }
            indice++;
        } else if (strcmp(argv[indice], "--saida") == 0) {
            if (indice + 1 >= argc || argv[indice + 1][0] == '-') {
                relatar_erro(rank, "Erro: --saida exige o caminho do arquivo PBM.\n");
                return 0;
            }
            opcoes->caminho_saida = argv[indice + 1];
            indice++;
        } else if (argv[indice][0] == '-') {
            relatar_erro(rank, "Erro: opcao desconhecida: %s\n", argv[indice]);
            return 0;
        } else if (opcoes->caminho_arquivo == NULL) {
            opcoes->caminho_arquivo = argv[indice];
        } else {
            relatar_erro(rank, "Erro: informe apenas um arquivo de entrada.\n");
            return 0;
        }
    }

    if (opcoes->caminho_arquivo == NULL) {
        relatar_erro(rank, "Erro: nenhum arquivo de entrada foi informado.\n");
        return 0;
    }
    if (sequencial_explicito && paralelo_explicito) {
        relatar_erro(rank, "Erro: --sequencial e --paralelo nao podem ser usados juntos.\n");
        return 0;
    }
    if (opcoes->deve_animar && (paralelo_explicito || opcoes->deve_verificar)) {
        relatar_erro(rank, "Erro: --animar exige modo sequencial sem --verificar.\n");
        return 0;
    }

    opcoes->deve_paralelizar = !(sequencial_explicito || opcoes->deve_animar);

    if (opcoes->deve_verificar && !opcoes->deve_paralelizar) {
        relatar_erro(rank, "Erro: --verificar exige o modo paralelo.\n");
        return 0;
    }
    return 1;
}

int processar_argumentos(int argc, char *argv[], int rank, OpcoesExecucao *opcoes) {
    if (!analisar_argumentos(argc, argv, rank, opcoes)) {
        relatar_erro(rank, "Use '%s --ajuda' para consultar as opcoes.\n", argv[0]);
        return 0;
    }
    return 1;
}
