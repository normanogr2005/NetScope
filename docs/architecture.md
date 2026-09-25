# NetScope Architecture

```text
/proc/net/dev ──────┐
                    ├──> sampler ──> rate calculator ──> terminal UI
/proc/net/tcp ──────┘                          │
                                               └──> connection table
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
