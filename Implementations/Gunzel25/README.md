# Günzel’25 C++ Reimplementation

This C++17 reimplementation is based on Günzel and Becker, *Optimal Task Phasing for End-To-End Latency in Harmonic and Semi-Harmonic Automotive Systems*, DOI `10.1109/RTAS65571.2025.00026`, and the authors' Python code. It implements optimal task phasing for max-harmonic and `(2,k)`-max-harmonic CECs.

- Upstream source: [the referenced version of ESRTS/OptimalTaskPhasing](https://github.com/ESRTS/OptimalTaskPhasing/tree/df812328781ab31b66e6f45f0f24e2a854c5d449)
- Main references: `optimalPhasingMaxHarm` and `optimalPhasing2kMaxHarm` in `OptimalPhasing.py`, together with helper functions for period conditions, maximum periods, and period transitions
- Upstream notice: [LICENSE.upstream](LICENSE.upstream), MIT, Copyright (c) 2024 ESRTS

`gunzel25.cpp` contains both the solver and the command-line interface (CLI); no separate local header is required.

## Preserved Method and Implementation Changes

| Aspect | Treatment |
| --- | --- |
| Phase construction | Preserves phase construction from cumulative period sums for max-harmonic CECs, and the branching and gamma correction based on transitions between the two largest distinct periods for `(2,k)`-max-harmonic CECs. |
| Latency | Preserves the authors' closed-form expressions based on the period sum, maximum periods, and transition count. |
| Data structures and units | Replaces Python task objects and microsecond-based representations with C++ vectors and integer milliseconds. Preserves the upstream convention of returning the computed phases without reducing them modulo their task periods. |
| Applicability checks | Checks the supported period relations. Uses exceptions and JSON error output in place of the upstream assertion and diagnostic paths, without repeating duplicate assertion checks. |

Günzel’25 accepts positive integer periods. Inputs may be rejected if an input value or an intermediate calculation exceeds the supported signed 64-bit range.

For inputs that are not max-harmonic, the CLI checks the `(2,k)`-max-harmonic condition. If that condition also fails, it rejects the input with `status: "error"` and exit code 3.

The procedures and per-case results for comparing the phases and latencies returned by the authors' Python implementation and this C++ implementation on all 3,000 CECs in the fixed semi-harmonic workload and the two applicable application-derived CECs are provided in [Validation/](../../Validation/).

