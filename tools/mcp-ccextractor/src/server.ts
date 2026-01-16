import { z } from "zod";
import {
  StdioServerTransport,
  Server,
} from "@modelcontextprotocol/sdk/server";
import { runCCExtractor } from "./ccextractor";

const extractSubtitlesInput = z.object({
  input_path: z.string().min(1, "input_path is required"),
  output_path: z.string().optional(),
  format: z.string().optional(),
  extra_args: z.array(z.string()).optional(),
  timeout_seconds: z.number().int().positive().max(3600).optional(),
});

const extractSubtitlesOutput = z.object({
  exit_code: z.number().nullable(),
  output_file: z.string().nullable(),
  stdout: z.string().nullable(),
  stderr: z.string(),
  timed_out: z.boolean(),
});

const listVersionOutput = z.object({
  exit_code: z.number().nullable(),
  version_text: z.string(),
});

const dryRunInput = extractSubtitlesInput.omit({ timeout_seconds: true });

const dryRunOutput = z.object({
  command: z.string(),
  warnings: z.array(z.string()),
});

async function main() {
  const transport = new StdioServerTransport();
  const server = new Server({
    name: "ccextractor-mcp",
    version: "0.1.0",
    transport,
  });

  server.tool("extract_subtitles", {
    description:
      "Run CCExtractor on an input media file and extract subtitles, either to a file or stdout.",
    inputSchema: extractSubtitlesInput,
    outputSchema: extractSubtitlesOutput,
    async handler(args) {
      const parsed = extractSubtitlesInput.parse(args.params ?? {});

      const result = await runCCExtractor({
        inputPath: parsed.input_path,
        outputPath: parsed.output_path,
        format: parsed.format,
        extraArgs: parsed.extra_args,
        timeoutSeconds: parsed.timeout_seconds,
      });

      return extractSubtitlesOutput.parse({
        exit_code: result.exitCode,
        output_file: result.outputFile,
        stdout: parsed.output_path ? null : result.stdout,
        stderr: result.stderr,
        timed_out: result.timedOut,
      });
    },
  });

  server.tool("list_ccextractor_version", {
    description: "Return the CCExtractor version and build info.",
    inputSchema: z.object({}).optional(),
    outputSchema: listVersionOutput,
    async handler() {
      const result = await runCCExtractor({
        inputPath: "--version", // CCExtractor treats this as CLI arg if no real input
      } as any);

      return listVersionOutput.parse({
        exit_code: result.exitCode,
        version_text: result.stdout || result.stderr,
      });
    },
  });

  server.tool("dry_run_args", {
    description:
      "Validate CCExtractor arguments and return the constructed CLI command without running a full extraction.",
    inputSchema: dryRunInput,
    outputSchema: dryRunOutput,
    async handler(args) {
      const parsed = dryRunInput.parse(args.params ?? {});

      const cmdParts = [
        process.env.CCEXTRACTOR_PATH || "/ccextractor",
        parsed.input_path,
      ];

      if (parsed.output_path) {
        cmdParts.push("-o", parsed.output_path);
      } else {
        cmdParts.push("--stdout");
      }

      if (parsed.format) {
        cmdParts.push("--output_format", parsed.format);
      }

      if (parsed.extra_args && parsed.extra_args.length) {
        cmdParts.push(...parsed.extra_args);
      }

      const warnings: string[] = [];
      if (!parsed.input_path.startsWith("/")) {
        warnings.push(
          "input_path is not absolute; ensure it resolves correctly inside the container or MCP runtime.",
        );
      }

      return dryRunOutput.parse({
        command: cmdParts.join(" "),
        warnings,
      });
    },
  });

  await server.connect();
}

main().catch((err) => {
  console.error("Fatal server error:", err);
  process.exit(1);
});
