# NetScope Architecture

```text
/proc/net/dev ──> sampler ──> rate calculator ──┬──> terminal UI
                                                 ├──> threshold warnings
/sys/class/net ──> interface metadata ───────────┤
/proc/net/tcp* ──> socket parser ────────────────┴──> TCP connection table
                                                        │
                                                        └──> NDJSON events
```

## Design goals

NetScope is intentionally small. It demonstrates how a C++ program can consume Linux kernel-exposed telemetry without external networking libraries.

### Data sources

- `/proc/net/dev` provides per-interface byte counters.
- `/proc/net/tcp` and `/proc/net/tcp6` provide kernel-maintained TCP socket state.

### Sampling model

The interface monitor takes two snapshots separated by the configured interval. Byte deltas are converted to bits per second:

`rate = ((current_bytes - previous_bytes) * 8) / seconds`

Counters that decrease are treated as resets/rollovers and produce a zero delta for that sample.

## Interface metadata

For each sampled interface, NetScope optionally reads `address`, `mtu`, and `operstate` from `/sys/class/net/<interface>/`. These fields are informational: unavailable or unreadable values become `unknown` and do not stop telemetry collection. Interface names are checked before constructing sysfs paths.

## Threshold alerts

The optional `--alert-mbps N` argument compares RX and TX rates independently against N megabits per second (decimal units). In terminal mode it prints warnings to standard error. In NDJSON mode the rate event uses warning severity when either direction meets or exceeds the threshold. Threshold warnings are disabled unless the option is provided.

## Audit and CI

The normal CI workflow builds in Release mode and runs CTest. A second job runs the test suite with AddressSanitizer and UndefinedBehaviorSanitizer. A separate manually triggered Claude workflow performs a read-only repository audit and requires the repository secret `ANTHROPIC_API_KEY`. It is intentionally not triggered on every push or pull request.
