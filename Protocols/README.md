# Experimental Protocol

## Environment and Build

The manuscript's experiments use Ubuntu 22.04.5, Linux 6.8.0-138-generic, an Intel Core i7-14650HX processor, and 31 GiB RAM. All methods are single-threaded C++17 programs compiled with g++ 11.4.0 using `-std=c++17 -O3 -DNDEBUG -g0 -s`. Methods run sequentially on the same isolated physical P-core. The fixed inputs comprise 500 CECs per synthetic workload/N stage and five application-derived CECs.

## Streaming CEC Inputs

All programs accept consecutive CEC input records with `--stream` and return one JSON line per CEC. After building, run the following example from the repository root:

```bash
printf '4 10 50 10 50\n3 1000 2000 40\n' | ./build/proposed --stream
```

This command demonstrates streaming input; it does not perform the full repetition protocol described below.

## Timing and Repetitions

`std::chrono::steady_clock` measures fresh solver construction, the algorithm call, and result extraction. Process startup, input parsing, warm-up, output, and CSV writing are excluded. Timer overhead is not subtracted. Proposed diagnostics are disabled for timing, and counts are collected separately.

The initial shuffled traversal calls the solver once for every CEC in the stage. Each CEC's initial execution time determines its fixed number of additional calls.

| Initial execution time | Additional calls |
| --- | ---: |
| >=1 s | 0 |
| [10 ms,1 s) | 10 |
| [100 us,10 ms) | 100 |
| (1 us,100 us) | 1,000 |
| <=1 us | 10,000 |

Each additional traversal starts a new process and visits the relevant CEC subset in shuffled order. Measurements are not consecutive batch averages for a single CEC. The representative time is the median of the individual call times, including the initial call. The CLI field `runtime_ns` is the time of one call, not this aggregate. The public CSVs contain the final representative times, not the individual repetition samples.

## Statistics and Counts

- Synthetic median and P99 values are computed from the 500 representative times in each method/workload/N group. P99 is the 495th sorted value.
- Median and P99 speedups are ratios of the corresponding group statistics, computed before rounding.
- Synthetic ablation results are geometric means of the 500 matched per-CEC variant/Base time ratios. Base has both SA and LBP disabled and denotes the manuscript's “Proposed without SA and LBP.”
- Proposed's evaluated phase vector count includes complete phase vectors rejected by child-level LBP after their exact latency has been computed, as well as path-level SA completions. Partial assignments are excluded. This count differs from generated or visited nodes and from total computational work. SA preserves this count, while LBP can reduce it.
- Application latency comparisons use optimal, zero, and worst phase vectors on a 1 ms integer phase grid.

## Measurement Exclusions

Günzel’25 is used only when its period conditions hold. The author reviewed the Wang’26 predictions and excluded an entire synthetic workload/N timing stage if the predicted sum of one call for each of its 500 CECs **exceeded 86,400 seconds**. This synthetic exclusion criterion does not apply to the five application-derived CECs.

The [prediction model description](Prediction_Model/README.md) provides the model setup, coefficients, validation limitations, and stage decisions. Excluded timings are absent from the results and are not replaced with predictions.
