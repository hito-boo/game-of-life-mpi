#ifndef SIMULACAO_H
#define SIMULACAO_H

/*
    Estado da simulacao no processo 0: matriz atual, matriz da proxima geracao e
    parametros lidos do arquivo. Nos demais processos fica zerada (NULL e 0).
*/
typedef struct {
    int **atual;
    int **proxima;
    int linhas;
    int colunas;
    int geracoes;
} Simulacao;

/*
    Le o arquivo de entrada e aloca a matriz da proxima geracao. Retorna 1 em caso de
    sucesso; em caso de erro retorna 0 e deixa a estrutura zerada.
*/
int preparar_simulacao(const char *caminho_arquivo, Simulacao *simulacao);

/*
    Libera as matrizes da simulacao e zera a estrutura.
*/
void liberar_simulacao(Simulacao *simulacao);

/*
    Executa a simulacao sequencial e mede apenas o calculo.
*/
void executar_simulacao_sequencial(Simulacao *simulacao, double *tempo, int *tempo_medido);

/*
    Executa a simulacao paralela (todos os processos chamam) e mede o tempo no processo 0.
    Retorna o resultado de simular_paralelo.
*/
int executar_simulacao_paralela(Simulacao *simulacao, int rank, int num_processos, double *tempo, int *tempo_medido);

/*
    Executa a simulacao sequencial exibindo o estado inicial e cada geracao no terminal.
*/
int simular_com_animacao(Simulacao *simulacao, unsigned int atraso_milissegundos);

/*
    Calcula a versao sequencial a partir de 'referencia' (copia do estado inicial, com
    'auxiliar' como matriz de apoio) e compara todas as celulas com o resultado paralelo.
*/
int verificar_resultado(const Simulacao *simulacao, int **referencia, int **auxiliar);

#endif
