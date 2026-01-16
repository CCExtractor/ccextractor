---
description: How to use Ralph for autonomous development with shared MCP infrastructure
---

# Ralph Workflow for CCExtractor

This workflow explains how to use Ralph (autonomous Claude Code) alongside IDE extensions like Kilo Code and Antigravity, all sharing the same MCP infrastructure.

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                    Docker MCP Gateway                            │
│  (docker mcp gateway run)                                        │
├─────────────────────────────────────────────────────────────────┤
│  filesystem │ github │ context7 │ playwright │ duckduckgo │ n8n │
└──────┬──────────────┬──────────────────────────┬────────────────┘
       │              │                          │
       ▼              ▼                          ▼
┌─────────────┐ ┌─────────────┐           ┌─────────────┐
│   Ralph     │ │  Kilo Code  │           │ Antigravity │
│ (CLI loop)  │ │ (VS Code)   │           │   (IDE)     │
└─────────────┘ └─────────────┘           └─────────────┘
```

## Prerequisites

// turbo
1. Ensure Docker MCP Gateway is running:
```bash
docker mcp gateway run
```

## Starting Ralph for Autonomous Work

// turbo
2. Navigate to the CCExtractor project:
```bash
cd /home/rahul/Desktop/ccextractor
```

3. Edit the task plan (optional):
```bash
nano .ralph/@fix_plan.md
```

// turbo
4. Start Ralph with monitoring:
```bash
ralph --monitor --prompt .ralph/PROMPT.md
```

## Using IDE Extensions Simultaneously

While Ralph is running autonomously, you can:
- Use **Kilo Code** in VS Code for interactive coding
- Use **Antigravity** for code review and assistance
- All tools share the same MCP servers!

## Stopping Ralph

5. To stop Ralph gracefully:
- Press `Ctrl+C` in the Ralph terminal
- Or wait for it to detect completion

## Checking Ralph Status

// turbo
6. Check current status:
```bash
ralph --status
```

## Tips

- Ralph works best for well-defined, repetitive tasks
- Use IDE extensions for exploratory/interactive work
- Edit `.ralph/@fix_plan.md` to guide Ralph's priorities
- Check logs in `.ralph/logs/` for debugging
