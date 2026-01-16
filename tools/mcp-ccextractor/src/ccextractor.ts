import { spawn } from "node:child_process";
import { randomUUID } from "node:crypto";
import { promises as fs } from "node:fs";
import { join } from "node:path";

export interface RunOptions {
  inputPath: string;
  outputPath?: string;
  format?: "srt" | "webvtt" | string;
  extraArgs?: string[];
  timeoutSeconds?: number;
  workDir?: string;
}

export interface RunResult {
  exitCode: number | null;
  stdout: string;
  stderr: string;
  outputFile: string | null;
  timedOut: boolean;
}

const DEFAULT_TIMEOUT_SECONDS = Number(process.env.CCEXTRACTOR_TIMEOUT_SECONDS ?? 300);
const MAX_OUTPUT_BYTES = Number(process.env.CCEXTRACTOR_MAX_OUTPUT_BYTES ?? 512 * 1024);
const CCEXTRACTOR_PATH = process.env.CCEXTRACTOR_PATH || "/ccextractor";
const WORK_DIR = process.env.CCEXTRACTOR_WORKDIR || "/data";

function truncateOutput(buf: Buffer): string {
  if (buf.length <= MAX_OUTPUT_BYTES) return buf.toString("utf8");
  const slice = buf.subarray(buf.length - MAX_OUTPUT_BYTES);
  return "[truncated]\n" + slice.toString("utf8");
}

export async function runCCExtractor(opts: RunOptions): Promise<RunResult> {
  const workDir = opts.workDir || WORK_DIR;
  const timeoutSeconds = opts.timeoutSeconds ?? DEFAULT_TIMEOUT_SECONDS;

  const inputPath = opts.inputPath;
  if (!inputPath) {
    throw new Error("inputPath is required");
  }

  const args: string[] = [];

  // Input file
  args.push(inputPath);

  // Output handling
  let outputFile: string | null = null;
  if (opts.outputPath) {
    outputFile = join(workDir, opts.outputPath);
    args.push("-o", outputFile);
  } else {
    // If no outputPath, default to stdout for easy capture
    args.push("--stdout");
  }

  // Format mapping (simple for now)
  if (opts.format) {
    switch (opts.format.toLowerCase()) {
      case "srt":
        // SRT is default; no extra flag needed
        break;
      case "webvtt":
        args.push("--gui-mode", "1", "--output_format", "webvtt");
        break;
      default:
        // For now, let callers pass custom formats via extraArgs
        break;
    }
  }

  if (opts.extraArgs && opts.extraArgs.length > 0) {
    args.push(...opts.extraArgs);
  }

  // Ensure working directory exists
  await fs.mkdir(workDir, { recursive: true }).catch(() => {});

  const child = spawn(CCEXTRACTOR_PATH, args, {
    cwd: workDir,
    stdio: ["ignore", "pipe", "pipe"],
  });

  let stdoutBuf = Buffer.alloc(0);
  let stderrBuf = Buffer.alloc(0);

  child.stdout?.on("data", (chunk: Buffer) => {
    stdoutBuf = Buffer.concat([stdoutBuf, chunk]);
  });

  child.stderr?.on("data", (chunk: Buffer) => {
    stderrBuf = Buffer.concat([stderrBuf, chunk]);
  });

  let timeoutHandle: NodeJS.Timeout | null = null;
  let timedOut = false;

  const exitCode: number | null = await new Promise((resolve) => {
    timeoutHandle = setTimeout(() => {
      timedOut = true;
      child.kill("SIGKILL");
    }, timeoutSeconds * 1000);

    child.on("close", (code) => {
      if (timeoutHandle) clearTimeout(timeoutHandle);
      resolve(code);
    });
  });

  const stdout = truncateOutput(stdoutBuf);
  const stderr = truncateOutput(stderrBuf);

  return {
    exitCode,
    stdout,
    stderr,
    outputFile,
    timedOut,
  };
}
