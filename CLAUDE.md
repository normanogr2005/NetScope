# Claude Code Instructions

## Project
NetScope is a Linux-only C++17 network telemetry tool. It reads interface counters from `/proc/net/dev`, TCP socket tables from `/proc/net/tcp*`, and optional interface metadata from `/sys/class/net`. Its NDJSON output is intended for ingestion by the companion Security-Lab project.

## Required engineering practices
- Preserve C++17 compatibility and the existing CMake/CTest workflow.
- Keep telemetry read-only. Do not add packet injection, credential collection, stealth, persistence, or privilege escalation.
- Validate external and kernel-provided text before parsing it. Avoid shelling out for data that is available through procfs/sysfs.
- Keep missing procfs/sysfs fields non-fatal and represent unavailable optional metadata as `unknown`.
- Never require root for the existing monitoring features.
- Add regression tests for bug fixes and new parsing behavior. Tests must remain effective in Release builds.
- Do not claim that top talkers can be calculated from interface byte counters alone. Per-process or per-remote traffic attribution requires packet/eBPF/socket instrumentation and must be documented as a separate capability.
- Keep JSON output valid NDJSON: exactly one JSON object per line, correctly escaped strings, stable event field names, and unique event IDs.
- Do not commit secrets, API keys, personal data, generated build artifacts, or local audit outputs.

## Audit mode
When asked to audit the repository:
1. Read the full source, tests, CMake configuration, GitHub Actions workflows, documentation, and dependency/action references.
2. Prioritize correctness, memory safety, undefined behavior, input validation, privilege boundaries, JSON/schema compatibility, test quality, CI supply-chain risk, and misleading documentation.
3. Report only reproducible or evidence-backed findings. Include severity, file and line, impact, a concrete reproduction or rationale, and a recommended fix.
4. Separate confirmed findings from risks that need manual verification. Do not invent vulnerabilities or report style preferences as security issues.
5. Run the build and tests when possible. Never use real network targets or destructive commands.
6. In read-only audit mode, do not modify repository files. Propose patches separately and clearly state which checks could not be run.
