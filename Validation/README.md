# Validation of the Comparison Implementations

For a fair execution-time comparison, we reimplemented the core logic of Wang’26 and Günzel’25 in C++17. The manuscript reports execution times measured for these C++ implementations under the same Linux environment and [measurement protocol](../Protocols/README.md) as Proposed. Their functional correspondence with the authors' code was checked separately as described below. Implementation changes are documented in [Wang26](../Implementations/Wang26/README.md) and [Gunzel25](../Implementations/Gunzel25/README.md).

This folder provides CSVs containing **inputs, comparison outputs, and outcomes from validation rerun on 2026-11-16 using the current public C++ sources**. The reference sources were downloaded from the pinned versions below, checked for byte equality with the retained originals, and used without modification. Functional validation ran on Windows with GCC/G++ 13.2.0 and Python 3.11.9, separately from the Linux performance measurements. For functional validation, C++ was compiled with `-std=c++17 -O3 -DNDEBUG` and the authors' C with `-std=c11 -O2`. Functional-validation execution times were not used in the manuscript. A separate execution-time comparison with the authors' code is reported below.

## Wang’26

The references are Wang et al., *The zeroLET Task Model and Its Application to Offset Design Space Exploration*, DOI `10.1109/TCAD.2026.3725519`, and [the pinned version of the authors' C code](https://github.com/wsm6636/ZEROLET/tree/6c3f0f983a6a7c6c04848fbfffb8afa8e714fda8).

We used the authors' `evaluation_zero_let.c`, `analysis_zero_let.c`, and `analysis_zero_let.h`. A validation I/O adapter excluded the original `main`, enumerated candidates through the original `generate_offsets_eq27_g` and `generate_offsets_eq27`, and called the original histogram-based maximum reaction time (MRT) evaluator, `run_analysis_zero_let`, for each candidate. Reference values therefore came from executing the authors' code. Fixed-phase comparisons called the same original evaluator.

| File | Validation coverage | Outcome |
| --- | --- | --- |
| [upstream_optimization.csv](Wang26/upstream_optimization.csv) | All 1,331 ordered period vectors with N=3 and each period in 2–12 | Minimum zeroLET MRT and evaluated phase vector counts agree between the authors' C and our C++ |
| [upstream_fixed_phase.csv](Wang26/upstream_fixed_phase.csv) | 320 general-LET, fixed-phase inputs: 40 per task-chain length for N=1–8 | MRT agrees across the authors' C, our C++, and independent event calculations |
| [independent_optimization.csv](Wang26/independent_optimization.csv) | 2,340 cases from three LET configurations on 780 small period vectors, plus 100 cases at N=5–8 | Minimum MRT agrees with independent exhaustive physical-phase search; returned phases also attain that value |
| [independent_fixed_phase.csv](Wang26/independent_fixed_phase.csv) | 54,240 fixed-phase evaluations enumerated for the three LET configurations on the small inputs | MRT agrees between independent event calculations and our C++ |

### Input Construction and Comparison Criteria

- Small inputs comprise all ordered period vectors with N=1–4 and each period in 1–5. The three LET configurations are `zero` (all lengths zero), `default` (lengths equal to periods), and `general`, with `L_i=(T_i+3*(i-1)) mod (2*T_i+1)`. Configurations may coincide; 2,340 is a case count, not the number of distinct period vectors.
- Additional inputs at N=5–8 use random seed `20261106`, with 24 generated cases per length. Periods are integers in 1–3, and LET lengths are integers in 0–2T_i. One case with all periods and LET lengths equal to 3 is added per length.
- The 320 general-LET comparisons with the authors' C use seed `20261105`. Periods are integers in 1–8, LET lengths in 0–2T_i, and input phases in −3T_i–3T_i. These general-LET and short-task-chain checks cover additional functionality supported by the implementation; they do not replace the manuscript's default-LET experimental inputs.
- Independent calculations follow physical LET reads and writes for every integer external-event time within one hyperperiod. At time t, task i's next read is `phi_i + ceil((t-phi_i)/T_i)*T_i`, and its write occurs `L_i` later. Negative job indices and data transfer at simultaneous writes and reads are allowed. Optimization checks fix the first task's phase at zero and exhaustively enumerate each remaining physical phase over `0..T_i-1`. This reference calculation uses neither the zeroLET transformation nor the GCD-reduced phase search space.
- Wang's original MRT concerns integer external events. Restoring LET gives `discrete_mrt = zerolet_mrt + sum(L_i)`, and the manuscript's continuous-time E2E supremum is `latency = discrete_mrt + 1`. The data use milliseconds, so 1 ms is added once to the complete result. Comparisons with the authors' MRT account for this distinction.
- Multiple optimal phase vectors may exist. Wang validation checks the minimum objective value and verifies that the C++ output phases attain it; identical phase vector strings across implementations are not required.

## Günzel’25

The references are Günzel and Becker, *Optimal Task Phasing for End-To-End Latency in Harmonic and Semi-Harmonic Automotive Systems*, DOI `10.1109/RTAS65571.2025.00026`, and [the pinned version of the authors' Python code](https://github.com/ESRTS/OptimalTaskPhasing/tree/df812328781ab31b66e6f45f0f24e2a854c5d449).

We imported the original `OptimalPhasing.py` together with `Task.py`, `Time.py`, and `DPT_Offset.py`, and directly called `optimalPhasingMaxHarm` or `optimalPhasing2kMaxHarm` according to the original applicability conditions. Python dependencies were `drs 2.0.0`, `networkx 3.3`, `numpy 2.4.6`, and `scipy 1.17.1`.

[upstream_comparison.csv](Gunzel25/upstream_comparison.csv) contains comparisons on all 3,000 [fixed semi-harmonic workload inputs](../Inputs/semi_harmonic.csv) and the two applicable application inputs, AEBS and Camera-based Control. The synthetic inputs comprise 2,877 max-harmonic cases and 123 `(2,k)`-max-harmonic cases. **Returned phases and latencies agree between the authors' Python and our C++ for all 3,002 inputs.**

In `upstream_comparison.csv`, `applicable_case` identifies the mathematical class: `max_harmonic` or `two_k_max_harmonic`.

Millisecond inputs were multiplied by 1,000 for the original Python microsecond interface, and returned values were divided by 1,000. Exact divisibility by 1,000 was checked. The original cumulative phases were compared directly, without reducing them modulo their task periods.

## Auxiliary Execution-Time Comparison with the Authors' Code

To check whether our public C++ reimplementations slowed down the comparison methods relative to the authors' code, we separately timed core calls on the same inputs on 2026-11-16. **In this measurement, the C++ reimplementations had shorter representative times for all 1,331 Wang inputs and all 3,002 Günzel inputs.** This provides empirical evidence against a performance advantage for Proposed arising from inflated execution times in our comparison implementations.

| Comparison | Inputs | Inputs with faster C++ | Median per-input upstream/C++ time ratio | Data |
| --- | ---: | ---: | ---: | --- |
| Wang authors' C / C++ reimplementation | 1,331 | 1,331 | 14.41 | [runtime_comparison.csv](Wang26/runtime_comparison.csv) |
| Günzel authors' Python / C++ reimplementation | 3,002 | 3,002 | 16.20 | [runtime_comparison.csv](Gunzel25/runtime_comparison.csv) |

The environment was an Intel Core i7-14650HX, Windows build 26200, GCC/G++ 13.2.0, and Python 3.11.9. The Python measurement process was pinned to logical CPU 0, and the native process inherited the same affinity; timed calls ran sequentially. C and C++ were compiled with `-std=c11` and `-std=c++17`, respectively, both using `-O3 -DNDEBUG -g0 -s`. The authors' Python functions ran in normal mode. Native blocks were timed with `std::chrono::steady_clock`, and Python blocks with `time.perf_counter_ns`.

- Wang uses the same 1,331 inputs as `upstream_optimization.csv`. We timed the authors' C candidate enumeration and histogram-based minimum zeroLET MRT calculation, and C++ solver construction and optimization. Each measured block also checked the returned minimum zeroLET MRT against the validated reference value.
- Günzel uses the same 3,002 inputs as `upstream_comparison.csv`. Original task objects and C++ period vectors were prepared, and the applicability branch selected, before timing the corresponding phasing functions. Python object construction and unit conversion, and the external C++ applicability-branch selection, were outside the timer. C++ solver construction and result-vector creation were included. Each measured block checked the returned latency against the validated reference value.
- Process startup, module imports, and I/O were excluded. Each function was called three times before each measured block. Preliminary measurements set an implementation-specific repetition count targeting approximately 200 microseconds per block, capped at 1–2,000 calls. Five traversals used shuffled input orders; upstream-first and C++-first measurement pairs alternated by traversal position. Timer overhead was not subtracted.
- Each element of a `*_batch_mean_ns` array is a block's total time divided by its call count. Each input's representative time, `*_median_batch_mean_ns`, is the median of its five block means. `upstream_over_cpp` is the upstream representative time divided by the C++ representative time for that input; the table reports the median of these ratios. `cpp_faster` indicates a shorter C++ representative time. These statistics therefore differ from the [manuscript's medians of individual call times](../Protocols/README.md).

For Wang, the difference includes omitting histograms and distribution statistics and evaluating immediate forward job chains from the first task. For Günzel, it includes Python/C++ language and data-structure differences. These auxiliary results demonstrate faster comparison implementations for the tested environment and inputs; they do not guarantee superiority in every environment or the fastest possible implementations. These Windows values are not incorporated into the manuscript's Linux execution times or Proposed speedups.

## CSV Interpretation and Validation Scope

- All `*_ms` fields use milliseconds, and vector fields are JSON arrays. `case_id` is unique within each Wang CSV. In `independent_fixed_phase.csv`, `optimization_case_id` refers to `case_id` in `independent_optimization.csv`. Günzel CSVs use `(workload, N, instance_id)` to identify the corresponding [Inputs/](../Inputs/README.md) records.
- `upstream_*` denotes outputs from the actual authors' code, `cpp_*` outputs from the public C++ implementations, and `independent_*` values from separate physical-event calculations. `match`, `phases_match`, and `latency_match` indicate agreement and are all `True`. In Wang optimization data, `full_physical_phase_vectors` counts the independent exhaustive search, whereas `cpp_evaluated_phase_vectors` counts the reduced zeroLET search.
- Functional validation covers the stated finite inputs. It is not a proof of equivalence for arbitrary inputs or a reproduction of execution times reported in the source papers. This folder provides reference versions, comparison procedures, and per-case data; it does not redistribute the upstream sources or validation tools. Upstream licenses are preserved in the respective [implementation folders](../Implementations/README.md).
