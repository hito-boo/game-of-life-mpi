# Jogo da Vida de Conway

Implementações sequencial e paralela (MPI) do Jogo da Vida de Conway em C.
O projeto simula a evolução de um tabuleiro finito, permite acompanhar as
gerações no terminal, exporta o resultado como imagem em preto e branco e mede
o tempo gasto no cálculo.

Este repositório faz parte do trabalho da disciplina de Programação Paralela da
Universidade Estadual de Maringá (UEM). Ambas as versões estão integradas ao
executável, com verificação de corretude opcional. Ele é a evolução da versão
com pthreads, agora com processos MPI.

## Integrantes

- Gabriel Libardi Lulu - RA 134728
- Matheus Cenerini Jacomini - RA 134700
- Vitor da Rocha Machado - RA 132769

## Regras da simulação

Cada posição do tabuleiro representa uma célula morta (`0`) ou viva (`1`). Em
cada geração, todas as células são atualizadas simultaneamente:

- Uma célula viva sobrevive quando possui dois ou três vizinhos vivos.
- Uma célula viva morre por solidão ou superpopulação nos demais casos.
- Uma célula morta nasce quando possui exatamente três vizinhos vivos.

O tabuleiro é finito. Posições fora de seus limites são consideradas mortas;
portanto, as bordas não se conectam ao lado oposto.

## Requisitos

- GCC com suporte a C11
- GNU Make
- Uma implementação de MPI (OpenMPI ou MPICH) com `mpicc` e `mpirun`
- Linux/WSL com relógio monotônico
- Python 3 para os scripts de experimentos

A aplicação C dispensa bibliotecas gráficas externas. A geração dos gráficos
pelos scripts utiliza ReportLab, conforme explicado ao final. Execute sempre via `mpirun`, mesmo no modo sequencial: o MPI é inicializado antes da leitura dos argumentos.

## Compilação

Para gerar o executável otimizado (usa `mpicc`):

```bash
make
```

O executável criado se chama `jogo_da_vida`.

Para gerar uma versão separada com símbolos de depuração, AddressSanitizer e
UndefinedBehaviorSanitizer:

```bash
make debug
mpirun -np 2 ./jogo_da_vida_debug entradas/glider.txt --sem-impressao
```

Para remover objetos, dependências e executáveis gerados:

```bash
make clean
```

## Formato da entrada

A primeira linha contém o número de linhas, o número de colunas e a quantidade
de gerações:

```text
linhas colunas geracoes
```

As linhas seguintes informam as coordenadas das células inicialmente vivas:

```text
linha coluna
```

Exemplo de um oscilador blinker em um tabuleiro `5 x 5`:

```text
5 5 4

2 1
2 2
2 3
```

As coordenadas começam em zero. Linhas vazias são opcionais e podem aparecer
entre as coordenadas. Coordenadas repetidas não alteram o resultado.
Coordenadas fora do tabuleiro geram um aviso e são ignoradas. Conteúdo
malformado faz o programa encerrar com erro e informar a linha correspondente.

## Execução

Uso geral:

```bash
mpirun -np N ./jogo_da_vida <arquivo_de_entrada> [opcoes]               # paralelo (padrão)
mpirun -np 1 ./jogo_da_vida <arquivo_de_entrada> --sequencial [opcoes]  # sequencial
```

O número de processos é definido pelo `mpirun -np N`. Não existe mais a opção
`--threads`.

Execução convencional (paralela, 4 processos):

```bash
mpirun -np 4 ./jogo_da_vida entradas/glider.txt
```

As opções disponíveis são:

- `--paralelo`: seleciona a versão paralela com MPI, usando todos os processos
  lançados pelo `mpirun` (padrão). Com `-np 1`, a versão paralela roda com um
  único processo.
- `--sequencial`: seleciona a versão sequencial. Somente o processo 0 calcula;
  se houver outros processos, o programa avisa que eles ficam ociosos.
- `--verificar`: executa a versão sequencial sobre uma cópia do estado inicial
  e compara todas as células finais com o resultado paralelo, fora do intervalo
  cronometrado. Exige o modo paralelo.
- `--sem-impressao`: omite as matrizes inicial e final.
- `--animar`: mostra todas as gerações no terminal (implica `--sequencial`).
- `--atraso MS`: define o intervalo da animação entre 0 e 60000 ms; sem
  `--animar`, o valor é ignorado.
- `--saida ARQUIVO`: salva o estado final em uma imagem PBM.
- `--ajuda` ou `-h`: mostra a ajuda do programa.

Também é possível executar por meio do Makefile:

```bash
make run NP=4
make run NP=4 ARQUIVO=entradas/blinker.txt ARGS="--sem-impressao"
make run-seq
make verificar NP=4
```

Em máquinas com menos núcleos do que processos, o OpenMPI exige
`MPI_ARGS=--oversubscribe`:

```bash
make run NP=8 MPI_ARGS=--oversubscribe
```

Para executar com 4 processos e conferir o resultado:

```bash
mpirun -np 4 ./jogo_da_vida entradas/glider.txt --verificar --sem-impressao
```

Sem `--verificar`, somente o modo escolhido é executado. A verificação mostra
`Verificacao: OK` quando os resultados são idênticos. Uma divergência gera a
mensagem `Verificacao: FALHOU` e encerra com código de erro. `--verificar` não
pode ser usado com `--sequencial`, pois compara o paralelo com o sequencial.
`--sequencial` e `--paralelo` não podem ser usados juntos.

## Animação no terminal

Para acompanhar as gerações com um intervalo de 150 ms:

```bash
./jogo_da_vida entradas/glider.txt --animar --atraso 150
```

Na animação, células vivas aparecem em branco e células mortas em preto. Cada
célula ocupa dois caracteres para compensar a proporção do terminal. A opção
requer uma saída interativa e uma janela suficientemente grande para o
tabuleiro. Matrizes grandes devem ser executadas com `--sem-impressao`.

A medição de desempenho não é feita durante a animação, pois atrasos e desenho
dos quadros distorceriam o resultado.
`--animar` funciona apenas no modo sequencial e sem `--verificar`.

## Exportação PBM

O estado final pode ser salvo no formato PBM textual (`P1`):

```bash
mpirun -np 2 ./jogo_da_vida entradas/blinker.txt --sem-impressao --saida resultado.pbm
```

No arquivo PBM, células vivas são pixels pretos e células mortas são pixels
brancos. O formato pode ser aberto por visualizadores de imagem compatíveis ou
convertido posteriormente por ferramentas externas. Somente o processo 0 grava
o arquivo.

## Medição de tempo

Nas execuções sem animação, o programa informa o tempo do modo escolhido:

```text
Tempo da simulacao paralela: 0.012345678 s
```

O intervalo medido (no processo 0) inclui a evolução das gerações. No modo
paralelo, inclui também o envio das dimensões, a alocação das matrizes locais,
a distribuição inicial das faixas, as trocas de bordas, as barreiras e a coleta
do resultado. Ficam fora a leitura, a alocação das matrizes completas, cópias
de referência, contagens, impressão, exportação e verificação. Portanto, o
tempo paralelo considera o custo completo da comunicação entre processos.
O tempo é obtido com `clock_gettime` (`tempo.c`), pois `MPI_Wtime` não faz
parte das rotinas MPI permitidas no trabalho.
Para experimentos, use matrizes maiores e desative a impressão:

```bash
mpirun -np 4 ./jogo_da_vida entradas/grande.txt --sem-impressao
```

## Testes automatizados

Execute a suíte com:

```bash
make test
```

Os 22 testes cobrem tabuleiro vazio, célula isolada, bloco estável, blinker,
glider, zero gerações, `1 x 1`, bordas, matrizes retangulares, comparação,
leitura, layout contíguo e cópia de matrizes e divisão de linhas entre
processos (`particao`). Os testes não usam MPI e são compilados com `gcc`.
O comando `make test-sequencial` executa a mesma suíte.
A comparação entre as versões sequencial e paralela é feita pelo próprio
programa quando a opção `--verificar` é informada.

Durante casos inválidos, avisos e erros aparecem propositalmente. A suíte
apresenta:

```text
22 testes executados: 22 aprovados e 0 falharam.
```

## Organização do código

- `main.c`: inicia e encerra o MPI e chama o programa.
- `execucao.c` e `execucao.h`: fluxo da aplicação nos modos sequencial e
  paralelo.
- `opcoes.c` e `opcoes.h`: leitura e validação dos argumentos.
- `simulacao.c` e `simulacao.h`: preparação, execução cronometrada, animação e
  verificação da simulação.
- `relatorio.c` e `relatorio.h`: impressão dos resultados e salvamento da imagem.
- `entrada.c` e `entrada.h`: leitura e validação do arquivo de entrada.
- `matriz.c` e `matriz.h`: alocação contígua, liberação, troca, cópia,
  comparação e impressão.
- `logica.c` e `logica.h`: regras e simulação sequencial.
- `particao.c` e `particao.h`: divisão das linhas do tabuleiro entre processos.
- `paralelo.c` e `paralelo.h`: simulação com MPI por faixas de linhas (único
  módulo que usa MPI).
- `renderizacao.c` e `renderizacao.h`: animação ANSI e exportação PBM.
- `tempo.c` e `tempo.h`: consulta ao relógio monotonicamente crescente.
- `testes/`: suíte automatizada e entradas usadas nos testes.
- `entradas/`: exemplos prontos para execução.
- `scripts/`: geração de entradas, benchmark e gráficos.

## Limitações atuais

- O número de processos deve estar entre 1 e o número de linhas.
- Uma faixa de linhas (`linhas da faixa x colunas`) deve caber em um `int`,
  limite do contador de mensagens MPI.
- O processo 0 lê o arquivo, mantém o tabuleiro completo e executa sozinho a
  distribuição e a coleta, que são sequenciais (um envio por processo).
- Em modo sequencial com vários processos, apenas o processo 0 trabalha.
- A animação é exclusiva da versão sequencial.
- A animação depende de um terminal com suporte a sequências ANSI.
- O tabuleiro usa bordas mortas, sem comportamento toroidal.
- A imagem PBM representa apenas a geração final.
- A impressão e a animação não são adequadas para tabuleiros muito grandes.

## Módulo paralelo

`simular_paralelo` recebe as referências das duas matrizes, linhas, colunas,
gerações, o rank do processo e o número de processos, nessa ordem. É chamada
por **todos** os processos. Retorna 1 no sucesso e 0 em erro, e o retorno é o
mesmo em todos os processos.

No processo 0, as matrizes e as dimensões são as lidas do arquivo e, ao final,
`*matriz_atual` contém o tabuleiro completo no estado final. Nos demais
processos, as matrizes são `NULL` e as dimensões valem 0; eles as recebem do
processo 0. Se o processo 0 falhar ao preparar a simulação (arquivo inválido,
falta de memória), ele chama a função com `linhas = 0` e todos retornam 0 sem
calcular.

A comunicação grande ocorre apenas no início e no fim. Cada processo é dono de
uma faixa exclusiva de linhas consecutivas (`calcular_faixa`) e mantém uma
matriz local com duas linhas a mais: a linha 0 e a última guardam cópias das
linhas dos processos vizinhos. Nos processos das extremidades essas linhas
ficam zeradas, o que representa as bordas mortas do tabuleiro. Assim,
`calcular_proxima_geracao` é reaproveitada sem alterações.

As fases são:

1. O processo 0 envia as dimensões e as gerações a todos; todos validam e
   confirmam a alocação das matrizes locais.
2. O processo 0 distribui as faixas de linhas.
3. Barreira.
4. A cada geração, cada processo troca apenas as linhas de borda com os
   vizinhos (`MPI_Send`/`MPI_Recv`), calcula sua faixa e troca as matrizes. Os
   processos de rank par e ímpar invertem a ordem de envio e recebimento para
   evitar deadlock.
5. Barreira.
6. Cada processo envia sua faixa ao processo 0, que monta o resultado.
7. Barreira.

As rotinas MPI utilizadas são `MPI_Init`, `MPI_Comm_size`, `MPI_Comm_rank`,
`MPI_Send`, `MPI_Recv`, `MPI_Barrier` e `MPI_Finalize`, além de
`MPI_COMM_WORLD` e `MPI_Status`. Os nomes e RAs do grupo constam em `main.c`; a
divisão efetiva das atividades deve ser registrada pelo grupo.

Para verificar o resultado paralelo contra a versão sequencial:

```bash
mpirun -np 4 ./jogo_da_vida entradas/glider.txt --verificar --sem-impressao
```

O alvo normal compila e liga com `mpicc`.

## Entradas maiores e experimentos

Para gerar uma entrada retangular reproduzível:

```bash
python3 scripts/gerar_entrada.py entradas/grande.txt --linhas 1000 --colunas 1200 --geracoes 100 --densidade 0.3 --semente 42
mpirun -np 4 ./jogo_da_vida entradas/grande.txt --verificar --sem-impressao
```

Para medir 3 tamanhos, cada um com 100 gerações, nas versões sequencial e
paralela com 1, 2, 4 e 8 processos, usando 5 repetições:

```bash
make benchmark
```

Os parâmetros podem ser personalizados:

```bash
make benchmark BENCH_ARGS="--tamanhos 256 768 --processos 1 2 4 --geracoes 80 --repeticoes 3 --saida resultados/outro_experimento"
```

Para executar com mais processos do que núcleos no OpenMPI, acrescente
`--mpi-args "--oversubscribe"` em `BENCH_ARGS`.

Antes das medições, o script confirma a corretude de cada configuração
paralela com `--verificar`. Não há execuções extras de aquecimento. As
repetições medidas não usam `--verificar`, e a ordem dos modos é embaralhada de
forma reproduzível a cada repetição. Cada execução parte do mesmo arquivo. Rode
sem outras cargas intensivas. A configuração sequencial roda com `--sequencial`
e a paralela com `--paralelo`, ambas via `mpirun -np N`.

O diretório de saída (`resultados/` por padrão) recebe:

- `tempos.csv`: amostras individuais (segundos).
- `resumo.csv`: média, mediana, desvio amostral, extremos, speedup e eficiência.
- `ambiente.json`: sistema, processador, afinidade, versão do MPI, parâmetros e hashes.
- `entradas/`: arquivos gerados; podem ser recriados pela semente e parâmetros.

Speedup = mediana sequencial / mediana paralela. Eficiência = speedup / número
de processos. O resultado com um processo paralelo permite observar o custo da
infraestrutura, mas pequenas diferenças também sofrem variação de medição.
Os resultados locais salvos não representam uma garantia para outras máquinas.

Os gráficos SVG podem ser regenerados com a dependência opcional ReportLab:

```bash
python3 -m pip install reportlab
python3 scripts/plotar_resultados.py
```

Python e ReportLab são ferramentas auxiliares: a simulação e a verificação do
executável são implementadas em C. O relatório técnico será preparado depois.