# Proposed and Ablation Variants

This is the exact branch-and-bound implementation of Fig. 5 in the manuscript. Candidates are examined in the order in which their endpoint residues first occur. The `F5 Lxx` comments refer to line numbers in the manuscript's algorithm.

| Entry point | Singleton acceleration (SA) | Lower-bound pruning (LBP) |
| --- | --- | --- |
| `proposed.cpp` | Enabled | Enabled |
| `proposed_without_sa.cpp` | Disabled | Enabled |
| `proposed_without_lbp.cpp` | Enabled | Disabled |
| `proposed_without_sa_lbp.cpp` | Disabled | Disabled |

The four entry points share `proposed.hpp`. Disabling SA disables both step-level SA and path-level SA; disabling LBP disables both child-level and state-level LBP. Inputs must contain N>=3 positive integer periods, and the caller must ensure that intermediate calculations fit within the signed 64-bit range.

## Instrumentation

The default setting, `ENABLE_COUNTS=0`, removes diagnostic storage, counter increments, and diagnostic output during preprocessing. Use this default for timing comparisons. To enable instrumentation, run the following commands from the repository root:

```bash
mkdir -p build
g++ -std=c++17 -O3 -DNDEBUG -g0 -s -DENABLE_COUNTS=1 Implementations/Proposed/proposed.cpp -o build/proposed_counts
echo "4 10 50 10 50" | ./build/proposed_counts
```

The same option applies to all three ablation variants. `full_phase_latency_evaluations` is the manuscript's evaluated phase vector count. It includes complete phase vectors rejected by child-level LBP after their exact latency has been computed, as well as path-level SA completions. Partial phase assignments are excluded. `evaluated_leaves`, `generated_nodes`, `visited_nodes`, and endpoint updates are distinct metrics.
