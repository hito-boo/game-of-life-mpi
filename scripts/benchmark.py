"""Mede o executavel C e salva amostras, estatisticas e ambiente em CSV/JSON."""
import argparse
import csv
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import platform
import random
import re
import shlex
import statistics
import subprocess
from gerar_entrada import gerar, positivo, densidade_valida


def executar(comando, verificar=False):
    """Rejeita erros, ausencia de tempo e verificacao nao confirmada."""
    processo = subprocess.run(comando, capture_output=True, text=True, timeout=300)
    if processo.returncode:
        raise RuntimeError(f"Falha em {comando!r}:\n{processo.stdout}\n{processo.stderr}")
    if verificar and "Verificacao: OK" not in processo.stdout:
        raise RuntimeError("O executavel nao confirmou a igualdade dos resultados.")
    medicao = re.search(r"Tempo da simulacao (?:sequencial|paralela): ([0-9.]+) s", processo.stdout)
    if medicao is None or float(medicao.group(1)) <= 0:
        raise RuntimeError("Tempo ausente ou nulo: aumente o tamanho ou as geracoes.")
    return float(medicao.group(1))


def versao_mpi(mpirun):
    """Obtem a versao do lancador MPI."""
    try:
        saida = subprocess.run([mpirun, "--version"], capture_output=True, text=True, timeout=30)
        return (saida.stdout or saida.stderr).strip().splitlines()[0]
    except (OSError, subprocess.SubprocessError, IndexError):
        return None


def montar_comando(opcoes, executavel, entrada, modo, processos):
    """Monta o comando de execucao com mpirun."""
    comando = [opcoes.mpirun, *shlex.split(opcoes.mpi_args), "-np", str(processos),
               str(executavel), str(entrada), "--sem-impressao"]
    comando.append("--sequencial" if modo == "sequencial" else "--paralelo")
    return comando


def ler_opcional(caminho):
    try:
        return Path(caminho).read_text().strip()
    except OSError:
        return None


def salvar_csv(caminho, registros):
    with caminho.open("w", newline="", encoding="utf-8") as arquivo:
        escritor = csv.DictWriter(arquivo, fieldnames=list(registros[0]))
        escritor.writeheader()
        escritor.writerows(registros)


def medir(opcoes):
    """Verifica cada configuracao e intercala a ordem das medicoes."""
    executavel = opcoes.executavel.resolve()
    saida = opcoes.saida.resolve()
    saida.mkdir(parents=True, exist_ok=True)
    metadados = {
        "inicio_utc": datetime.now(timezone.utc).isoformat(),
        "sistema": platform.platform(), "python": platform.python_version(),
        "cpu": next((l.split(":", 1)[1].strip() for l in
                     (ler_opcional("/proc/cpuinfo") or "").splitlines()
                     if l.startswith("model name")), platform.processor()),
        "cpus_logicas": os.cpu_count(),
        "afinidade": sorted(os.sched_getaffinity(0)) if hasattr(os, "sched_getaffinity") else None,
        "cgroup_cpu_max": ler_opcional("/sys/fs/cgroup/cpu.max"),
        "executavel_sha256": hashlib.sha256(executavel.read_bytes()).hexdigest(),
        "parametros": {k: str(v) if isinstance(v, Path) else v for k, v in vars(opcoes).items()},
        "mpi": versao_mpi(opcoes.mpirun),
        "medicao": "CLOCK_MONOTONIC no processo 0; paralelo inclui distribuicao, calculo e coleta (MPI); exclui leitura, E/S e verificacao",
        "entradas": [],
    }
    configuracoes = [("sequencial", 1)] + [("paralelo", n) for n in sorted(set(opcoes.processos))]
    amostras = []
    for tamanho in sorted(set(opcoes.tamanhos)):
        entrada = saida / "entradas" / f"tabuleiro_{tamanho}.txt"
        vivas = gerar(entrada, tamanho, tamanho, opcoes.geracoes, opcoes.densidade, opcoes.semente)
        metadados["entradas"].append({"arquivo": str(entrada.relative_to(saida)), "vivas": vivas,
            "sha256": hashlib.sha256(entrada.read_bytes()).hexdigest()})
        comandos = {}
        for modo, processos in configuracoes:
            comando = montar_comando(opcoes, executavel, entrada, modo, processos)
            comandos[(modo, processos)] = comando
            if modo == "paralelo":  # --verificar compara o paralelo com o sequencial
                executar(comando + ["--verificar"], verificar=True)
        for repeticao in range(1, opcoes.repeticoes + 1):
            ordem = configuracoes.copy()
            random.Random(opcoes.semente + tamanho + repeticao).shuffle(ordem)
            for modo, processos in ordem:
                tempo = executar(comandos[(modo, processos)])
                amostras.append(dict(linhas=tamanho, colunas=tamanho, geracoes=opcoes.geracoes,
                                     modo=modo, processos=processos, repeticao=repeticao, tempo_s=tempo))
                salvar_csv(saida / "tempos.csv", amostras)
        print(f"{tamanho} x {tamanho}: corretude confirmada e medicoes concluidas.", flush=True)
    resumo = []
    for tamanho in sorted(set(opcoes.tamanhos)):
        base = statistics.median(r["tempo_s"] for r in amostras
                                 if r["linhas"] == tamanho and r["modo"] == "sequencial")
        for modo, processos in configuracoes:
            tempos = [r["tempo_s"] for r in amostras
                      if r["linhas"] == tamanho and r["modo"] == modo and r["processos"] == processos]
            mediana = statistics.median(tempos)
            speedup = base / mediana
            resumo.append(dict(linhas=tamanho, colunas=tamanho, geracoes=opcoes.geracoes,
                               modo=modo, processos=processos, amostras=len(tempos), mediana_s=mediana,
                               media_s=statistics.mean(tempos),
                               desvio_s=statistics.stdev(tempos) if len(tempos) > 1 else 0,
                               minimo_s=min(tempos), maximo_s=max(tempos),
                               speedup=speedup, eficiencia=speedup / processos))
    salvar_csv(saida / "resumo.csv", resumo)
    metadados["fim_utc"] = datetime.now(timezone.utc).isoformat()
    (saida / "ambiente.json").write_text(json.dumps(metadados, indent=2, ensure_ascii=False) + "\n",
                                         encoding="utf-8")
    print(f"Resultados em {saida}.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executavel", type=Path, default=Path("jogo_da_vida"))
    parser.add_argument("--saida", type=Path, default=Path("resultados"))
    parser.add_argument("--tamanhos", nargs="+", type=positivo, default=[128, 512, 1024])
    parser.add_argument("--processos", nargs="+", type=positivo, default=[1, 2, 4, 8],
                        help="quantidades de processos MPI")
    parser.add_argument("--mpirun", default="mpirun", help="lancador MPI")
    parser.add_argument("--mpi-args", default="",
                        help="argumentos extras do lancador")
    parser.add_argument("--geracoes", type=positivo, default=100)
    parser.add_argument("--repeticoes", type=positivo, default=5)
    parser.add_argument("--densidade", type=densidade_valida, default=0.3)
    parser.add_argument("--semente", type=int, default=42)
    medir(parser.parse_args())
