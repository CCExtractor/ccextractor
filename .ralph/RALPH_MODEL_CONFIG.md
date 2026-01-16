# Ralph Loop Model Configuration Guide

## Current Default Model
**`opencode/glm-4.7-free`**

## How to Change Models

### Method 1: Environment Variable (Temporary, Recommended for Testing)
```bash
RALPH_MODEL="opencode/claude-sonnet-4-5" /ralph-loop Build API
RALPH_MODEL="opencode/glm-4.7-free" /ralph-loop Fix bugs
```

### Method 2: CLI Flag (One-Time)
```bash
/ralph-loop --model opencode/claude-sonnet-4-5 Build API
```

### Method 3: Direct Config Edit (Project-Specific)
Edit `.ralph-tui/config.toml` in your project:
```toml
[[agents]]
name = "opencode"
plugin = "opencode"
default = true
options = { model = "opencode/claude-sonnet-4-5" }
```

### Method 4: Global Config (All Projects)
Create global config:
```bash
mkdir -p ~/.config/ralph-tui
cat > ~/.config/ralph-tui/config.toml << EOF
defaultAgent = "opencode"

[[agents]]
name = "opencode"
plugin = "opencode"
default = true
options = { model = "opencode/claude-sonnet-4-5" }
EOF
```

## Available Models
Run `opencode models` to see all available options.

Popular choices:
- `opencode/claude-sonnet-4-5` - Latest Claude Sonnet
- `opencode/claude-sonnet-4` - Stable Claude Sonnet
- `opencode/glm-4.7-free` - GLM 4.7 Free (default)
- `opencode/glm-4.6` - GLM 4.6
- `opencode/gpt-5` - GPT-5
- `opencode/gpt-5.1` - GPT-5.1

## Priority Order (Highest to Lowest)
1. CLI flag: `/ralph-loop --model <name>`
2. Environment variable: `RALPH_MODEL=<name>`
3. Project config: `.ralph-tui/config.toml`
4. Global config: `~/.config/ralph-tui/config.toml`
5. Script default: `opencode/glm-4.7-free`

## Verification
Check current config:
```bash
ralph-tui config show
```

## Troubleshooting
If you see "Model not found" error:
1. Verify model name: `opencode models | grep <model>`
2. Check config: `ralph-tui config show --toml`
3. Use full model name (e.g., `opencode/claude-sonnet-4-5`, not `claude-sonnet-4-5`)
