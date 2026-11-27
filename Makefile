# Compilador MPI e flags comuns
MPICC ?= mpicc
CC = $(MPICC)
CPPFLAGS = -I.
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
DEPFLAGS = -MMD -MP
PYTHON ?= python3
LDFLAGS =
DEBUG_CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O0 -g3 -fsanitize=address,undefined
DEBUG_LDFLAGS = -fsanitize=address,undefined

# Lancador MPI e numero de processos
MPIRUN ?= mpirun
MPI_ARGS ?=
NP ?= 4

# Nomes dos executaveis
TARGET = jogo_da_vida
DEBUG_TARGET = jogo_da_vida_debug
TEST_TARGET = testes/teste_logica
TEST_CC ?= gcc

# Arquivos-fonte, cabecalhos, objetos e dependencias geradas automaticamente
SRCS = main.c execucao.c opcoes.c simulacao.c relatorio.c entrada.c matriz.c logica.c particao.c paralelo.c renderizacao.c tempo.c
HEADERS = execucao.h opcoes.h simulacao.h relatorio.h entrada.h matriz.h logica.h particao.h paralelo.h renderizacao.h tempo.h
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)
TEST_SRCS = testes/teste_logica.c entrada.c matriz.c logica.c particao.c

# Arquivo de entrada padrao para 'make run' (pode ser sobrescrito na linha de comando)
ARQUIVO ?= entradas/glider.txt
ARGS ?=

.PHONY: all clean debug run run-seq verificar test test-sequencial benchmark

# Regra padrao: compila o executavel
all: $(TARGET)

# Ligacao (linking) dos arquivos objeto para gerar o executavel
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS)

# Regra generica de compilacao, incluindo arquivos .d para dependencias de cabecalhos
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

# Gera um executavel separado com simbolos de depuracao e sanitizadores
debug: $(DEBUG_TARGET)

$(DEBUG_TARGET): $(SRCS) $(HEADERS)
	$(CC) $(CPPFLAGS) $(DEBUG_CFLAGS) -o $(DEBUG_TARGET) $(SRCS) $(DEBUG_LDFLAGS)

# Executa o modo paralelo com NP processos
# Uso: make run NP=4 ARQUIVO=entradas/meu_teste.txt ARGS="--sem-impressao"
run: $(TARGET)
	$(MPIRUN) $(MPI_ARGS) -np $(NP) ./$(TARGET) $(ARQUIVO) $(ARGS)

# Executa o modo sequencial (um processo, sem comunicacao)
run-seq: $(TARGET)
	$(MPIRUN) $(MPI_ARGS) -np 1 ./$(TARGET) $(ARQUIVO) --sequencial $(ARGS)

# Executa o modo paralelo e compara com a versao sequencial (--verificar)
verificar: $(TARGET)
	$(MPIRUN) $(MPI_ARGS) -np $(NP) ./$(TARGET) $(ARQUIVO) --verificar $(ARGS)

# Executa a suite de testes
test: test-sequencial

test-sequencial: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRCS) entrada.h matriz.h logica.h particao.h
	$(TEST_CC) $(CPPFLAGS) $(CFLAGS) -o $(TEST_TARGET) $(TEST_SRCS) $(LDFLAGS)

# Parametros podem ser passados em BENCH_ARGS; veja --help do script.
benchmark: $(TARGET)
	$(PYTHON) scripts/benchmark.py --executavel ./$(TARGET) $(BENCH_ARGS)

# Remove os objetos, dependencias e executaveis gerados
clean:
	rm -f $(OBJS) $(DEPS) $(TARGET) $(DEBUG_TARGET) $(TEST_TARGET)

-include $(DEPS)
