# Wang’26 Execution-Time Prediction Model

This model predicts Wang’26 execution time for each input. The author reviewed the returned predictions and selected synthetic workload/N stages using a 24-hour criterion. Predictions are not reported as measured execution times. The final coefficients and validation summary are in [model.json](model.json); stage predictions and measurement decisions are listed below.

The predicted time for a single solver call, in nanoseconds, is:

```text
setup_N + factor_N * probe_ns * (D / probe_phases) * ((H / T_first) / probe_jobs)
```

D is the total number of non-equivalent integer zeroLET phase vectors required for complete exploration. H is the hyperperiod, and `H/T_first` is the total number of immediate forward job chains starting at the first task within one hyperperiod for each phase vector. `probe_ns` is the median execution time across repeated measurements of a block with limited numbers of phase vectors and immediate forward job chains per vector. `probe_phases` is the number of phase vectors actually evaluated under the probe limit; `probe_jobs` is the number of immediate forward job chains actually evaluated per phase vector under the probe limit. Thus, `D/probe_phases` and `(H/T_first)/probe_jobs` are scaling factors for extrapolating partial-exploration time to complete-exploration time. Each factor is 1 when the corresponding range is evaluated in full. `setup_N` is an additive correction in nanoseconds, and `factor_N` is a dimensionless multiplicative correction.

Calibration used 124 training calls that completed the full exploration. Samples with uncorrected predictions below 5,000 ns were used to set `setup_N`, and those at or above 5,000 ns were used to set `factor_N`, using median-based estimates separately for each N. At N=8, no short-call training samples were available, so the default `setup_N=500` ns was used. Prediction errors were checked using 135 held-out calls, including 16 longer calls, on separate inputs not used to determine the coefficients.

Initial probes used five measurements per input and at most 10,000 immediate forward job chains per phase vector. Subsequent larger-block checks covered 40 inputs and a further 144 inputs at N=6, using three measurements per input and up to 100,000 immediate forward job chains per phase vector. For randomly sampled N=8, an additional 48 inputs were checked in the same way using three measurements and up to 1,000,000 immediate forward job chains per phase vector; these observations were incorporated into the final prediction sum. Larger-block measurements check bounded portions of the search and are not counted as independent validations using complete solver calls.

Held-out absolute relative error is `|predicted_time-actual_time|/actual_time`. Its median is 4.07%, and P90 is 20.39%, using the 122nd of the 135 sorted errors. The maximum underprediction factor, defined as the maximum actual/predicted time ratio, is 2.327. Linear extrapolation has limitations when only subsets of phase vectors or immediate forward job chains are evaluated. Predictions are not guaranteed upper bounds or confidence intervals. This artifact provides the final model and decisions, without the individual probe records or model-development logs.

## Stage Predictions and Measurement Decisions

For each synthetic workload/N stage, the prediction is the sum of one predicted call for each of its 500 CECs. The application-derived row sums the five application cases, to which the synthetic exclusion criterion does not apply. These sums differ from the total experiment duration, which includes repeated calls, process startup, and other costs.

`True` means measured and `False` means not measured. The author excluded a synthetic stage when its predicted sum for one call per CEC exceeded 86,400 seconds (24 hours). For each workload, the table lists stages through the first `False` and omits subsequent longer stages. Predictions are rounded to six decimal places from the retained records for display; threshold comparisons use unrounded values.

| Workload | N | Predicted sum for one call per CEC (s) | Measured |
| --- | --- | ---: | --- |
| Application-derived | mixed | 1,792.347912 | True |
| Semi-harmonic | 3 | 0.051094 | True |
| Semi-harmonic | 4 | 18.633936 | True |
| Semi-harmonic | 5 | 928.832991 | True |
| Semi-harmonic | 6 | 225,823.551681 | False |
| `(x,y)`-hyperperiod | 3 | 0.062910 | True |
| `(x,y)`-hyperperiod | 4 | 8.670589 | True |
| `(x,y)`-hyperperiod | 5 | 908.018431 | True |
| `(x,y)`-hyperperiod | 6 | 91,464.316387 | False |
| Randomly sampled | 3 | 0.001353 | True |
| Randomly sampled | 4 | 0.043317 | True |
| Randomly sampled | 5 | 1.138821 | True |
| Randomly sampled | 6 | 32.690177 | True |
| Randomly sampled | 7 | 613.029162 | True |
| Randomly sampled | 8 | 16,959.373477 | True |

Wang’26 execution times were directly measured for 6,000 synthetic CECs and five application-derived CECs, totaling 6,005 CECs.
Missing stages are neither zero-time nor timeout results and are not filled with predictions.
