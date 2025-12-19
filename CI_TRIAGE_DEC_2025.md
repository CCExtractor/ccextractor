# CI Test Triage - December 2025

This PR is used to trigger CI runs and track the triage of failing regression tests.

## Purpose

Several PRs have been merged recently that improved CCExtractor behavior, but the Sample Platform
considers them regressions because the "ground truth" baseline is outdated. This PR helps:

1. Get a fresh CI run against current master
2. Systematically analyze each failing test
3. Determine whether to update ground truth or fix code

## Failing Tests to Triage

Will be populated after CI run completes.

## Status

- [ ] CI run triggered
- [ ] Results analyzed
- [ ] Triage decisions made
