# Final Per-CEC Results

These results use the [fixed inputs](../Inputs/README.md). The key for `runtime.csv` and `phase_vector_counts.csv` is `(workload, N, instance_id, method)`; the key for `application_latency.csv` is `instance_id`. Workload names are `max_harmonic`, `xy_hyperperiod`, `randomly_sampled`, and `application`.

| method | Program entry point (relative to the repository root) |
| --- | --- |
| `proposed_sa_lbp` | `Implementations/Proposed/proposed.cpp` |
| `proposed_sa_only` | `Implementations/Proposed/proposed_without_lbp.cpp` |
| `proposed_lbp_only` | `Implementations/Proposed/proposed_without_sa.cpp` |
| `proposed_none` | `Implementations/Proposed/proposed_without_sa_lbp.cpp` |
| `wang26` | `Implementations/Wang26/wang26.cpp` |
| `gunzel25` | `Implementations/Gunzel25/gunzel25.cpp` |

## Files and Units

- `runtime.csv`: 45,027 directly measured method/CEC results. `runtime_median_ns` is the median of individual solver call times, including the initial call; divide by 1,000 to convert to microseconds. `phases_ms` is a JSON array, and `e2e_latency_ms` is the returned minimum latency. Different methods may return different optimal phase vectors.
- `phase_vector_counts.csv`: 48,027 rows. `evaluated_phase_vectors` is the number of latency evaluations of complete phase vectors. `determination` distinguishes Wang's analytical counts, Günzel's construction of one phase vector, and Proposed's separate count collection. Wang counts are also provided for CECs without measured execution times.
- `application_latency.csv`: optimal, zero-phase, and worst latency for the five applications, together with phase vectors attaining the optimal and worst values. The zero phase vector has all phases set to zero. Worst latency is defined on the **1 ms integer phase grid**. Fig. 1(a) normalizes each latency by `zero_latency_ms`.
- For every CEC with results from at least two methods, all recorded minimum E2E latencies agree.

## Timing Coverage

| Method | Synthetic workloads | Applications |
| --- | --- | --- |
| All four Proposed configurations | All three workloads, N=3–8 | All five |
| Wang’26 | Max-harmonic and `(x,y)`-hyperperiod, N=3–5; randomly sampled, N=3–8 | All five |
| Günzel’25 | Max-harmonic, N=3–8 | AEBS and Camera-based Control |

Missing timing rows indicate unmeasured or inapplicable cases, not zero execution times. Wang's stage predictions and measurement decisions are listed in the [prediction model table](../Protocols/Prediction_Model/README.md). Günzel is applied only when its period conditions hold.
