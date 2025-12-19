# CI Test Triage - December 2025

This PR is used to trigger CI runs and track the triage of failing regression tests.

## Purpose

Several PRs have been merged recently that improved CCExtractor behavior, but the Sample Platform
considers them regressions because the "ground truth" baseline is outdated. This PR helps:

1. Get a fresh CI run against current master
2. Systematically analyze each failing test
3. Determine whether to update ground truth or fix code

## Merged Fixes

The following PRs have been merged and this run verifies their combined effect:

- **PR #1847**: Hardsubx crash fix, memory leak fixes, rcwt exit code fix
- **PR #1848**: XDS empty content entries fix

## Status

- [x] PR #1847 merged
- [x] PR #1848 merged
- [ ] Verification CI run triggered
- [ ] Results analyzed
