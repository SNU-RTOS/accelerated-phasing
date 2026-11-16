# Algorithm Implementations

| Directory | Contents |
| --- | --- |
| [Proposed/](Proposed/README.md) | Complete algorithm, three ablation variants, and shared header |
| [Wang26/](Wang26/README.md) | C++ entry point and header for exhaustive search over non-equivalent integer zeroLET phase vectors |
| [Gunzel25/](Gunzel25/README.md) | C++ closed-form method for max-harmonic and `(2,k)`-max-harmonic CECs |

The method-specific READMEs describe the implementations, their sources, changes, and applicability conditions. Compile each `.cpp` into a separate executable. `Proposed/proposed.hpp` and `Wang26/wang26.hpp` are required headers and are not compiled separately.

## Build

GNU g++ with C++17 support is required. The manuscript uses g++ 11.4.0 with the flags below. The implementations use GCC overflow built-ins; MSVC builds are not supported by these instructions. No external C++ libraries or Python packages are required. Run the following commands from the repository root.

Linux:

```bash
mkdir -p build
for name in proposed proposed_without_sa proposed_without_lbp proposed_without_sa_lbp; do
    g++ -std=c++17 -O3 -DNDEBUG -g0 -s "Implementations/Proposed/${name}.cpp" -o "build/${name}"
done
g++ -std=c++17 -O3 -DNDEBUG -g0 -s Implementations/Wang26/wang26.cpp -o build/wang26
g++ -std=c++17 -O3 -DNDEBUG -g0 -s Implementations/Gunzel25/gunzel25.cpp -o build/gunzel25
echo "4 10 50 10 50" | ./build/proposed
```

## Common Input and Output

Standard input is the whitespace-separated sequence `N T_1 ... T_N`, not a CSV row. Periods, returned `phases`, and `latency` are in milliseconds; `runtime_ns` is in nanoseconds. The output is one JSON record, with `status: "ok"` indicating successful completion.

The CLI timer measures solver construction, algorithm execution, and result extraction for that call.

## Licensing and Implementation Notes

Project-authored code and documentation are provided under [the root MIT license](../LICENSE). Portions derived from the Wang’26 and Günzel’25 upstream sources also retain the corresponding `LICENSE.upstream` notices. These files preserve the original license texts from the referenced versions byte for byte.

Source headers identify the copyrights, licenses, and location of the implementation notes. Method-specific changes are documented in the corresponding READMEs; these descriptions do not replace the upstream license texts. The comparison-method READMEs link to the referenced upstream versions rather than redistributing their complete source trees. [Implementation validation](../Validation/) is separate from the Linux performance measurements.
