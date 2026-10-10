# Security and Quality Audit Checklist

Use this checklist with Claude Code or a human reviewer. Record findings with file/line references and reproducible evidence. Do not mark a control as passed without checking it.

## Correctness and reliability
- [ ] Interface counter parsing handles malformed and incomplete lines.
- [ ] Rate calculations handle zero/negative elapsed time, new interfaces, counter resets, and large values.
- [ ] IPv4/IPv6 endpoint decoding rejects malformed hexadecimal fields and invalid ports.
- [ ] TCP state names and socket data are represented accurately.
- [ ] Sysfs metadata lookup cannot escape the configured interface root.
- [ ] Missing procfs/sysfs files fail safely and predictably.
- [ ] Timestamp generation and event identifiers remain valid and unique enough for the documented use.
- [ ] NDJSON strings are escaped and every emitted line is a valid JSON object.

## Security boundaries
- [ ] The tool is read-only and does not require root for documented features.
- [ ] No shell interpolation, command injection, unsafe temporary paths, or unnecessary external commands are introduced.
- [ ] No secrets, tokens, private network captures, or personal information are committed.
- [ ] Error output does not leak sensitive data.
- [ ] Third-party GitHub Actions are pinned or their version-update policy is documented.
- [ ] Workflow permissions follow least privilege; AI audit runs only when explicitly triggered.
- [ ] Claude audit workflow requires an explicitly configured Anthropic secret and does not grant write permissions.

## Tests and CI
- [ ] Assertions remain active in Release builds.
- [ ] CTest passes in Release and Debug/sanitizer configurations.
- [ ] Tests cover malformed input and boundary conditions.
- [ ] Compiler warnings are reviewed.
- [ ] CI output is inspected for flaky or environment-dependent tests.

## Integration and documentation
- [ ] NDJSON events match the current Security-Lab schema.
- [ ] Examples match actual CLI behavior.
- [ ] Documented features exist in the implementation.
- [ ] Limitations are explicit. Interface counters alone cannot identify top remote endpoints or processes.
- [ ] Any claim of compatibility with Security-Lab is verified against its current schema and tests.

## Finding format

For each finding include:
- **Severity:** Critical / High / Medium / Low / Informational
- **Location:** path/to/file.cpp:line
- **Evidence:** observed code path, test output, or reproducible command
- **Impact:** realistic consequence
- **Recommendation:** concrete, minimal correction
- **Confidence:** High / Medium / Low

Finish with build/test commands executed, checks not performed, and remaining risks. A clean audit is not proof of absence of vulnerabilities.
