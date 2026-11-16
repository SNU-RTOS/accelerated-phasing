# Wang’26 C++ Reimplementation

This C++17 reimplementation is based on Wang et al., *The zeroLET Task Model and Its Application to Offset Design Space Exploration*, DOI `10.1109/TCAD.2026.3725519`, and the authors' C code.

- Upstream source: [the referenced version of wsm6636/ZEROLET](https://github.com/wsm6636/ZEROLET/tree/6c3f0f983a6a7c6c04848fbfffb8afa8e714fda8)
- Main reference files: `analysis_zero_let.c`, `analysis_zero_let.h`, and `extremes_zero_let.c`
- Upstream notice: [LICENSE.upstream](LICENSE.upstream), MIT, Copyright (c) 2026 Shumo Wang
- `wang26.hpp` contains the solver; `wang26.cpp` provides the command-line interface (CLI).

## Preserved Method

The first task's phase in the zeroLET model is fixed at zero. The remaining phases `z_i` are exhaustively searched over the integers `0 <= z_i < gcd(H_{1:i-1},T_i)`, where `H_{1:i-1}` is the hyperperiod of the preceding tasks. For each phase vector, immediate forward job chains are evaluated from the first task's jobs within one hyperperiod. After finding the minimum zeroLET maximum reaction time (MRT), the implementation adds the sum of the LET interval lengths and restores the read phases of the original LET model. The default LET interval lengths are `L_i=T_i`.

## Implementation Changes and Additions

| Aspect | Treatment in this C++ implementation |
| --- | --- |
| Latency evaluation | Uses the paper's enumeration of first-task jobs and maximum-length calculation. Omits the histograms, latency arrays, means, standard deviations, and plotting functions accompanying the upstream C program's exhaustive enumeration of integer external events. |
| Integer arithmetic | Computes the earliest reader using integer modulo expressions. Uses signed 64-bit arithmetic, overflow checks during preparation, and nonnegative modulo for negative operands. |
| Output phases | `phases` contains the restored read phases of the original LET model; `zerolet_offsets` contains the zeroLET phases. |
| Metric conversion | `discrete_mrt` is the source method's integer-event MRT. The manuscript's continuous-time E2E supremum is `latency=discrete_mrt+1`. For millisecond inputs, this adds 1 ms once to the complete result. |

Validation procedures and results are summarized in [Validation/](../../Validation/).

## Options

See the [common instructions](../README.md) for building and the default input/output format, and [Protocols/](../../Protocols/) for streaming input.

- `--evaluate`: reads N physical read phases after the periods and evaluates that vector.
- `--zero-evaluate`: reads N zeroLET phases after the periods and evaluates that vector.
- `--lets`: reads N LET interval lengths after the periods. If fixed phases are also supplied, they follow the LET interval lengths.
- `--max-phase-vectors K`, `--max-forward-chains K`: optional work limits; zero means unlimited. Exceeding a limit returns `status: "limit"` and exit code 4, without returning a partial result as an optimum. These limits are separate from the manuscript's measurement-exclusion criterion.

```bash
echo "4 10 50 10 50 0 0 0 0" | ./build/wang26 --evaluate
```

For this synchronous AEBS input, `latency` is 210 ms. Unsupported inputs or arithmetic ranges may be rejected.

