# Optimal Task Phasing for LET-Based Cause–Effect Chains

This repository accompanies *Accelerated Optimal Task Phasing for LET-Based Cause–Effect Chains*. It provides implementations, fixed inputs, and final experimental results for computing minimum end-to-end (E2E) latency and a task phase vector attaining it under the default Logical Execution Time (LET) model with implicit deadlines.

| Directory | Contents |
| --- | --- |
| [Implementations/](Implementations/README.md) | Proposed and three ablation variants, Wang’26 and Günzel’25 implementations, and build/run instructions |
| [Inputs/](Inputs/README.md) | The 9,005 CECs used in the experiments and the input generator |
| [Results/](Results/README.md) | Per-CEC execution times, optimal phases, minimum latencies, and evaluated complete phase vector counts |
| [Validation/](Validation/README.md) | Procedures, inputs, per-case results, and limitations of functional comparisons with the authors' code |
| [Protocols/](Protocols/README.md) | Measurement and statistical procedures, and the Wang’26 execution-time prediction model |

## Quick Start

GNU g++ with C++17 support is required. No external C++ libraries are used. Run the following commands from the repository root.

```bash
mkdir -p build
g++ -std=c++17 -O3 -DNDEBUG -g0 -s Implementations/Proposed/proposed.cpp -o build/proposed
echo "4 10 50 10 50" | ./build/proposed
```

The minimum `latency` for this AEBS input is 170 ms. Output is one JSON line containing the phase vector and `runtime_ns` for that call. See the [implementation instructions](Implementations/README.md) for build and instrumentation options, and the [protocol](Protocols/README.md) for streaming inputs.

## Licensing and Sources

Project-authored software and its documentation are provided under the [MIT license](LICENSE). Copyright holders are Jongwon Lee, Namcheol Lee, Saehwa Kim, and Seongsoo Hong, in manuscript author order. Rights and notices for portions derived from upstream code are retained.

- Wang’26: [original MIT license](Implementations/Wang26/LICENSE.upstream), [sources and implementation changes](Implementations/Wang26/README.md)
- Günzel’25: [original MIT license](Implementations/Gunzel25/LICENSE.upstream), [sources and implementation changes](Implementations/Gunzel25/README.md)

The comparison implementations are C++ reimplementations developed in this project from code supplied by the original authors. They are not presented as unmodified upstream releases or implementations endorsed by those authors. The project MIT license does not replace the upstream MIT licenses; redistribution of the relevant code must retain the corresponding upstream notices.

The software MIT notice does not change rights in the manuscript, its PDF, or third-party materials. The code license alone does not establish a separate data license for the input and measurement CSVs. Requests to cite the paper acknowledge the research source and do not impose additional restrictions on MIT-licensed use.
