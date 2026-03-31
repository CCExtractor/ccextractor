#!/usr/bin/env bash
set -euo pipefail

INPUT=""
RUNS=3
OUTDIR="/tmp/ccx_bench_out"
CSV=""

usage() {
  cat <<EOF
Usage:
  $0 --input <file> [--runs N] [--outdir DIR] [--csv FILE]

Runs CCExtractor multiple times and prints wall time + throughput (MB/s).
Optionally writes results to a CSV.

Example:
  bash tools/bench_linux_throughput.sh --input /path/in.ts --runs 5 --csv /tmp/results.csv
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --input) INPUT="$2"; shift 2 ;;
    --runs) RUNS="$2"; shift 2 ;;
    --outdir) OUTDIR="$2"; shift 2 ;;
    --csv) CSV="$2"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown arg: $1"; usage; exit 2 ;;
  esac
done

if [[ -z "$INPUT" ]]; then
  echo "ERROR: --input is required"
  usage
  exit 2
fi

if [[ ! -f "$INPUT" ]]; then
  echo "ERROR: input file not found: $INPUT"
  exit 2
fi

if [[ ! -x "./ccextractor" ]]; then
  echo "ERROR: ./ccextractor not found or not executable."
  echo "Build CCExtractor first, then run this script from the repo root."
  exit 2
fi

mkdir -p "$OUTDIR"

bytes=$(wc -c < "$INPUT" | tr -d ' ')
mb=$(awk "BEGIN { printf \"%.3f\", $bytes / 1048576 }")

if [[ -n "$CSV" ]]; then
  mkdir -p "$(dirname "$CSV")"
  echo "run,seconds,mb,mb_per_s" > "$CSV"
fi

echo "Input: $INPUT"
echo "Size: ${mb} MB"
echo "Runs: $RUNS"
echo "Output dir: $OUTDIR"
[[ -n "$CSV" ]] && echo "CSV: $CSV"
echo

for i in $(seq 1 "$RUNS"); do
  out="$OUTDIR/out_${i}.srt"
  rm -f "$out"

  start=$(date +%s.%N)
  ./ccextractor "$INPUT" -o "$out" >/dev/null 2>&1 || true
  end=$(date +%s.%N)

  seconds=$(awk "BEGIN { printf \"%.6f\", $end - $start }")
  mbps=$(awk "BEGIN { if ($seconds > 0) printf \"%.3f\", $mb / $seconds; else print \"0\" }")

  echo "Run $i: ${seconds}s, ${mbps} MB/s"

  if [[ -n "$CSV" ]]; then
    echo "$i,$seconds,$mb,$mbps" >> "$CSV"
  fi
done