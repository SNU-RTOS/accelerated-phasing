# Fixed CEC Inputs

These are the fixed inputs used in the measurements. Each CSV contains `N`, `instance_id`, and `periods_ms`; the last column is a JSON array of integer periods in milliseconds, in task-chain order. A CEC is identified by `(workload, N, instance_id)`.

## Application-Derived Inputs

`application.csv` contains the following five application-derived CECs.

| Application ID | Case | Periods (ms) | Period-vector source |
| --- | --- | --- | --- |
| 1 | AEBS | `[10,50,10,50]` | [RTAS 2025](https://doi.org/10.1109/RTAS65571.2025.00026) |
| 2 | Camera-based Control | `[1000,2000,40]` | [RTAS 2024](https://doi.org/10.1109/RTAS61025.2024.00019) |
| 3 | Brake-by-Wire | `[20,30,40,50,60]` | [Shape-Aware E2E case studies](https://github.com/marioguenzel/Shape-Aware-E2E/blob/main/chains/case_studies.jsonl) |
| 4 | LiDAR-based Control | `[33,400,15,15,5]` | [Shape-Aware E2E case studies](https://github.com/marioguenzel/Shape-Aware-E2E/blob/main/chains/case_studies.jsonl) |
| 5 | Pilot Request Handling | `[40,60,100,100,100,60,40]` | [IJCCBS 2014](https://doi.org/10.1504/IJCCBS.2014.064667) |

The application-derived inputs preserve the reported periods and dependencies while adopting the manuscript's default LET semantics and independent task phasing. They are not measurements of the original applications' schedulers.

## Synthetic Inputs

Each of the three synthetic CSVs contains 500 distinct ordered CECs for each N=3,...,8, totaling 3,000 CECs per file. Generation uses Python's standard-library `random.Random`. Duplicate ordered period vectors within a workload/N group are rejected; repeated periods within a vector are allowed.

| Workload | Generation rule and seed for length N |
| --- | --- |
| Semi-harmonic | Draw each period independently and uniformly from `{1,2,5,10,20,50,100,200,1000}`. Use seed `20261006+N`, except N=8 uses `20261015`. Includes `(2,5)`-max-harmonic CECs. |
| `(x,y)`-hyperperiod | Draw x uniformly from 3–20, and y uniformly from the first ten integers greater than x that are coprime to x. Draw q uniformly from `1,...,floor(500/(10*y))` and set s=10q. Start with `ys,xs`; draw the remaining periods with replacement from the list consisting of `[ys,xs]` followed by the positive divisors of `xys` smaller than `xs`, in ascending order. Shuffle the completed vector. H=xys and the maximum period is <=500. Use seed `20261007+N`. |
| Randomly sampled | Draw each integer period t independently with replacement from 2–100, with probability proportional to `1/t`. Use seed `20261008+N`. |

The semi-harmonic workload comprises max-harmonic and `(2,5)`-max-harmonic CECs. The inputs are provided in `semi_harmonic.csv`.

## Regenerating Inputs

Run the following command from the repository root to regenerate inputs in a separate directory. Only the Python standard library is required. The generator refuses to overwrite existing target CSVs and produces inputs only, not measurements or solver results.

```bash
python Inputs/generate.py --output-dir build/generated_inputs
```
