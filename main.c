#include "execucao.h"
#include "paralelo.h"

/*
Trabalho - Programacao Paralela

Alunos:
    - Gabriel Libardi Lulu (RA: 134728)
    - Matheus Cenerini Jacomini (RA: 134700)
    - Vitor da Rocha Machado (RA: 132769)

Modo de execucao:
    make
    mpirun -np 4 ./jogo_da_vida entrada.txt [--verificar] [--sem-impressao]
    ./jogo_da_vida entrada.txt --sequencial

    ou

    make run NP=4 ARQUIVO=entradas/arquivo.txt

Limpeza do diretorio:
    make clean
*/

int main(int argc, char *argv[]) {
    int rank = 0;
    int num_processos = 1;

    iniciar_paralelo(&argc, &argv, &rank, &num_processos);
    int codigo_saida = executar_programa(argc, argv, rank, num_processos);
    finalizar_paralelo();

    return codigo_saida;
}
