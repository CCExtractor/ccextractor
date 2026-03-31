# Linux throughput benchmarking (CCExtractor)

This guide provides a simple, repeatable way to measure CCExtractor throughput on **Linux** and to evaluate the impact of tuning changes (CPU scheduling, I/O, filesystem, storage, etc.).

> Note: The commands in this document are intended for Linux. If you are developing on Windows/macOS, run the benchmarks inside a Linux environment (e.g., a Linux machine/VM/WSL) for meaningful results.

## What you get

- Wall-clock time per run
- Effective throughput (MB/s) based on input file size and run time
- Optional CSV output you can compare across branches/changes

## Prerequisites (Linux)

Install a few common tools (Debian/Ubuntu packages shown):

```bash
sudo apt-get update
sudo apt-get install -y time sysstat procps util-linux
```
