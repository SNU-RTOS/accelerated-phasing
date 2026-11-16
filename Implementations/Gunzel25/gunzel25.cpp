// SPDX-License-Identifier: MIT
// Upstream copyright (c) 2024 ESRTS
// C++ implementation and additions copyright (c) 2026 Jongwon Lee, Namcheol Lee, Saehwa Kim, and Seongsoo Hong
// Preserve LICENSE.upstream and ../../LICENSE; origin and modifications: README.md.
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using Time = std::int64_t;

enum class ApplicableCase {
    MaxHarmonic,
    TwoKMaxHarmonic,
    Unsupported,
};

struct SolveResult {
    ApplicableCase applicable_case;
    Time latency = 0;
    std::vector<Time> phases;
};

bool checked_add(Time lhs, Time rhs, Time& result) {
    return !__builtin_add_overflow(lhs, rhs, &result);
}

bool checked_multiply(Time lhs, Time rhs, Time& result) {
    return !__builtin_mul_overflow(lhs, rhs, &result);
}

bool checked_lcm(Time lhs, Time rhs, Time& result) {
    return checked_multiply(lhs / std::gcd(lhs, rhs), rhs, result);
}

Time maximum_period(const std::vector<Time>& periods) {
    Time maximum = 0;
    for (const Time period : periods) {
        if (period > maximum) {
            maximum = period;
        }
    }
    return maximum;
}

Time second_largest_period(const std::vector<Time>& periods) {
    Time largest = 0;
    Time second_largest = 0;
    for (const Time period : periods) {
        if (period > largest) {
            second_largest = largest;
            largest = period;
        } else if (period < largest && period > second_largest) {
            second_largest = period;
        }
    }
    return second_largest;
}

bool is_max_harmonic(const std::vector<Time>& periods) {
    const Time maximum = maximum_period(periods);
    for (const Time period : periods) {
        if (maximum % period != 0) {
            return false;
        }
    }
    return true;
}

bool is_two_k_max_harmonic(const std::vector<Time>& periods) {
    std::vector<Time> distinct_periods;
    for (const Time period : periods) {
        if (std::find(distinct_periods.begin(), distinct_periods.end(), period) ==
            distinct_periods.end()) {
            distinct_periods.push_back(period);
        }
    }
    std::sort(distinct_periods.begin(), distinct_periods.end());
    if (distinct_periods.size() < 2) {
        return false;
    }

    Time hyperperiod = 1;
    for (const Time period : distinct_periods) {
        Time next = 0;
        if (!checked_lcm(hyperperiod, period, next)) {
            throw std::overflow_error("period hyperperiod exceeds int64 range");
        }
        hyperperiod = next;
    }

    const Time largest = distinct_periods.back();
    const Time second_largest = distinct_periods[distinct_periods.size() - 2];
    Time twice_largest = 0;
    if (!checked_multiply(largest, 2, twice_largest)) {
        throw std::overflow_error("twice the largest period exceeds int64 range");
    }
    if (hyperperiod != twice_largest || hyperperiod % second_largest != 0) {
        return false;
    }
    for (std::size_t i = 0; i + 2 < distinct_periods.size(); ++i) {
        if (largest % distinct_periods[i] != 0 ||
            second_largest % distinct_periods[i] != 0) {
            return false;
        }
    }
    return true;
}

class Gunzel25Solver {
public:
    Gunzel25Solver(const std::vector<Time>& periods, ApplicableCase applicable_case)
        : periods_(periods), applicable_case_(applicable_case) {}

    SolveResult solve() const {
        if (applicable_case_ == ApplicableCase::MaxHarmonic) {
            return solve_max_harmonic();
        }
        return solve_two_k_max_harmonic();
    }

private:
    Time sum_periods() const {
        Time sum = 0;
        for (const Time period : periods_) {
            Time next = 0;
            if (!checked_add(sum, period, next)) {
                throw std::overflow_error("period sum exceeds int64 range");
            }
            sum = next;
        }
        return sum;
    }

    SolveResult solve_max_harmonic() const {
        std::vector<Time> phases(periods_.size(), 0);
        Time accumulated_period = 0;
        for (std::size_t i = 0; i < periods_.size(); ++i) {
            phases[i] = accumulated_period;
            Time next = 0;
            if (!checked_add(accumulated_period, periods_[i], next)) {
                throw std::overflow_error("phase construction exceeds int64 range");
            }
            accumulated_period = next;
        }

        const Time maximum = maximum_period(periods_);
        const Time period_sum = sum_periods();
        Time latency = 0;
        if (!checked_add(period_sum, maximum, latency)) {
            throw std::overflow_error("latency exceeds int64 range");
        }
        return {ApplicableCase::MaxHarmonic, latency, std::move(phases)};
    }

    SolveResult solve_two_k_max_harmonic() const {
        const Time largest = maximum_period(periods_);
        const Time second_largest = second_largest_period(periods_);
        if (second_largest == 0) {
            throw std::invalid_argument("two-k input requires two distinct periods");
        }

        // Validate applicability once inside the measured algorithm.  The
        // artifact contains the same pure predicate in an assertion; retaining
        // that duplicate call would add no algorithmic work or information.
        if (!is_two_k_max_harmonic(periods_)) {
            throw std::invalid_argument("input is not (2,k)-max-harmonic");
        }

        const Time gamma = largest % second_largest;
        std::vector<std::size_t> period_switches;
        const Time switch_largest = maximum_period(periods_);
        const Time switch_second_largest = second_largest_period(periods_);
        Time previous_top_period = -1;

        for (std::size_t i = 0; i < periods_.size(); ++i) {
            const Time period = periods_[i];
            if (period != switch_largest && period != switch_second_largest) {
                continue;
            }
            if (previous_top_period != -1 && previous_top_period != period) {
                period_switches.push_back(i);
            }
            previous_top_period = period;
        }

        const Time rounded_half_transitions =
            static_cast<Time>((period_switches.size() + 1) / 2);

        Time balanced_gap = 0;
        if (!checked_multiply(rounded_half_transitions, gamma, balanced_gap)) {
            throw std::overflow_error("balanced gap exceeds int64 range");
        }
        const bool use_balanced_gap = balanced_gap < largest;

        const auto first_largest = std::find(periods_.begin(), periods_.end(), largest);
        const std::size_t first_largest_index =
            static_cast<std::size_t>(first_largest - periods_.begin());

        std::vector<Time> phases(periods_.size(), 0);
        for (std::size_t i = 1; i < periods_.size(); ++i) {
            Time gap = 0;
            const bool is_period_switch =
                std::find(period_switches.begin(), period_switches.end(), i) !=
                period_switches.end();
            if (use_balanced_gap && i != first_largest_index && is_period_switch &&
                periods_[i] == largest) {
                gap = gamma;
            }

            Time next_phase = 0;
            if (!checked_add(phases[i - 1], periods_[i - 1], next_phase) ||
                !checked_add(next_phase, gap, phases[i])) {
                throw std::overflow_error("phase construction exceeds int64 range");
            }
        }

        Time extra = 0;
        if (!checked_add(largest, std::min(balanced_gap, largest), extra)) {
            throw std::overflow_error("latency term exceeds int64 range");
        }
        const Time period_sum = sum_periods();
        Time latency = 0;
        if (!checked_add(period_sum, extra, latency)) {
            throw std::overflow_error("latency exceeds int64 range");
        }

        return {ApplicableCase::TwoKMaxHarmonic, latency, std::move(phases)};
    }

    const std::vector<Time>& periods_;
    ApplicableCase applicable_case_;
};

const char* case_name(ApplicableCase applicable_case) {
    switch (applicable_case) {
        case ApplicableCase::MaxHarmonic:
            return "max_harmonic";
        case ApplicableCase::TwoKMaxHarmonic:
            return "two_k_max_harmonic";
        case ApplicableCase::Unsupported:
            return "unsupported";
    }
    return "unsupported";
}

void print_error(const char* status, const std::string& message) {
    std::cout << "{\"status\":\"" << status << "\"}\n";
    std::cerr << message << '\n';
}

int solve_and_print(const std::vector<Time>& periods) {
    const auto start = std::chrono::steady_clock::now();
    const ApplicableCase applicable_case =
        is_max_harmonic(periods)
            ? ApplicableCase::MaxHarmonic
            : ApplicableCase::TwoKMaxHarmonic;
    const Gunzel25Solver solver(periods, applicable_case);
    const SolveResult result = solver.solve();
    const auto finish = std::chrono::steady_clock::now();
    const auto runtime_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();

    if (result.applicable_case == ApplicableCase::Unsupported) {
        std::cout << "{\"status\":\"unsupported\",\"latency\":null,"
                     "\"phases\":[],\"runtime_ns\":"
                  << runtime_ns << ",\"case\":\"unsupported\"}\n" << std::flush;
        return 4;
    }

    std::cout << "{\"status\":\"ok\",\"latency\":" << result.latency
              << ",\"phases\":[";
    for (std::size_t i = 0; i < result.phases.size(); ++i) {
        if (i != 0) {
            std::cout << ',';
        }
        std::cout << result.phases[i];
    }
    std::cout << "],\"runtime_ns\":" << runtime_ns << ",\"case\":\""
              << case_name(result.applicable_case) << "\"}\n" << std::flush;
    return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    const bool stream_mode = argc == 2 && std::string(argv[1]) == "--stream";
    if (argc != 1 && !stream_mode) {
        print_error("invalid", "usage: gunzel25 [--stream]");
        return 2;
    }

    while (true) {
        std::int64_t signed_task_count = 0;
        if (!(std::cin >> signed_task_count)) {
            if (stream_mode && std::cin.eof()) {
                return 0;
            }
            print_error("invalid", "expected a positive task count followed by periods");
            return 2;
        }
        if (signed_task_count <= 0) {
            print_error("invalid", "expected a positive task count followed by periods");
            return 2;
        }
        const auto task_count = static_cast<std::uint64_t>(signed_task_count);
        if (task_count > std::numeric_limits<std::size_t>::max()) {
            print_error("invalid", "task count is too large for this platform");
            return 2;
        }
        std::vector<Time> periods(static_cast<std::size_t>(task_count));
        for (Time& period : periods) {
            if (!(std::cin >> period) || period <= 0) {
                print_error("invalid", "all periods must be positive int64 values");
                return 2;
            }
        }
        if (!stream_mode) {
            std::cin >> std::ws;
            if (!std::cin.eof()) {
                print_error("invalid", "unexpected data after the period vector");
                return 2;
            }
        }
        try {
            const int result_code = solve_and_print(periods);
            if (result_code != 0) {
                return result_code;
            }
        } catch (const std::overflow_error& error) {
            print_error("invalid", error.what());
            return 2;
        } catch (const std::exception& error) {
            print_error("error", error.what());
            return 3;
        } catch (...) {
            print_error("error", "unknown solver failure");
            return 3;
        }
        if (!stream_mode) {
            return 0;
        }
    }
}
