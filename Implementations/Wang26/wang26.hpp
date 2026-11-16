// SPDX-License-Identifier: MIT
// Upstream copyright (c) 2026 Shumo Wang
// C++ implementation and additions copyright (c) 2026 Jongwon Lee, Namcheol Lee, Saehwa Kim, and Seongsoo Hong
// Preserve LICENSE.upstream and ../../LICENSE; origin and modifications: README.md.
// Wang'26: zeroLET offset exploration, DOI 10.1109/TCAD.2026.3725519.
// Paper Eqs. (7), (10), (14), (15), (20), (25)--(27).
// Integer-time MRT and continuous-time E2E supremum are separate outputs.
#ifndef ACCELERATED_PHASING_WANG26_HPP
#define ACCELERATED_PHASING_WANG26_HPP

#include <algorithm>
#include <cstdint>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

namespace wang26 {
using Time = std::int64_t;

// Match the archived backward Martinez'18 arithmetic: signed 64-bit times,
// GCC checked preparation, and mathematical modulo. No 128-bit division.
inline Time checked_add(Time a, Time b) {
    Time result;
    if (__builtin_add_overflow(a, b, &result))
        throw std::overflow_error("time sum exceeds int64 range");
    return result;
}
inline Time checked_multiply(Time a, Time b) {
    Time result;
    if (__builtin_mul_overflow(a, b, &result))
        throw std::overflow_error("time product exceeds int64 range");
    return result;
}
inline Time floor_mod(Time x, Time positive_denominator) {
    Time r = x % positive_denominator;
    if (r < 0) r += positive_denominator;
    return r;
}

struct LimitExceeded : std::runtime_error { using std::runtime_error::runtime_error; };
struct Limits {
    // Zero means no user-imposed work limit. Never return a partial optimum.
    std::uint64_t phase_vectors = 0;
    std::uint64_t forward_job_chains = 0;
};
struct Result {
    Time latency = 0;             // continuous-time supremum, for manuscript comparisons
    Time discrete_mrt = 0;        // paper Definition 2, integer external event instants
    Time zerolet_mrt = 0;         // paper Eq. (15), T_1 - 1 boundary
    std::vector<Time> phases;     // restored read phases, each in [0,T_i)
    std::vector<Time> zerolet_offsets;
    std::uint64_t evaluated_phase_vectors = 0;
};

class Solver {
public:
    // Default LET: L_i=T_i. Explicit zero lengths must be passed as a full vector.
    explicit Solver(std::vector<Time> periods, std::vector<Time> lengths = {})
        : periods_(std::move(periods)), lengths_(std::move(lengths)) {
        if (periods_.empty()) throw std::invalid_argument("require N >= 1");
        if (lengths_.empty()) lengths_ = periods_;
        if (lengths_.size() != periods_.size())
            throw std::invalid_argument("LET interval count mismatch");
        domains_.assign(periods_.size(), 1);
        prefixes_.resize(periods_.size());
        Time sum_periods = 0;
        for (std::size_t i = 0; i < periods_.size(); ++i) {
            const Time t = periods_[i];
            if (t <= 0 || lengths_[i] < 0)
                throw std::invalid_argument("require positive periods and nonnegative LET intervals");
            prefixes_[i] = sum_lengths_;
            sum_lengths_ = checked_add(sum_lengths_, lengths_[i]);
            sum_periods = checked_add(sum_periods, t);
            if (i) domains_[i] = std::gcd(hyperperiod_, t);
            hyperperiod_ = checked_multiply(hyperperiod_ / std::gcd(hyperperiod_, t), t);
            if (phase_count_ > std::numeric_limits<std::uint64_t>::max() /
                               static_cast<std::uint64_t>(domains_[i]))
                throw std::overflow_error("phase count exceeds uint64 range");
            phase_count_ *= static_cast<std::uint64_t>(domains_[i]);
        }
        // Same conservative release bound as the archived backward Martinez'18.
        // Normalized z_i lies in [0,T_i), first is in [0,H), and each hop
        // adds 0..T_i-1. Thus current < H+sum(T_i), and z_i-current fits int64.
        // The bound also permits unchecked, exact int64 arithmetic in the hot loop.
        checked_add(hyperperiod_, checked_multiply(sum_periods, 2));
        // General LET restoration and both public latency outputs must also fit.
        checked_add(sum_periods, sum_lengths_);
        jobs_per_vector_ = hyperperiod_ / periods_.front();
    }

    Result solve(Limits limits = {}) const {
        const Time jobs = forward_job_chains_per_vector();
        if (limits.phase_vectors && phase_count_ > limits.phase_vectors)
            throw LimitExceeded("complete Phi exceeds --max-phase-vectors");
        if (limits.forward_job_chains && phase_count_ > limits.forward_job_chains /
                                                       static_cast<std::uint64_t>(jobs))
            throw LimitExceeded("complete exploration exceeds --max-forward-chains");
        std::vector<Time> z(periods_.size(), 0), best_z;
        Time best = std::numeric_limits<Time>::max();
        do {
            const Time value = evaluate_zerolet_mrt(z);
            if (value < best) { best = value; best_z = z; }
        } while (advance(z));
        Result result = compose(best, std::move(best_z));
        // All candidates are evaluated; the exact count was prepared once.
        result.evaluated_phase_vectors = phase_count_;
        return result;
    }

    Result evaluate_zerolet(std::vector<Time> offsets, Limits limits = {}) const {
        validate_count(offsets);
        if (limits.forward_job_chains &&
            static_cast<std::uint64_t>(forward_job_chains_per_vector()) > limits.forward_job_chains)
            throw LimitExceeded("fixed evaluation exceeds --max-forward-chains");
        // Independent period shifts relabel integer job indices and preserve event grids.
        for (std::size_t i = 0; i < offsets.size(); ++i)
            offsets[i] = floor_mod(offsets[i], periods_[i]);
        const Time value = evaluate_zerolet_mrt(offsets);
        Result result = compose(value, std::move(offsets));
        result.evaluated_phase_vectors = 1;
        return result;
    }

    Result evaluate_phases(const std::vector<Time>& phases, Limits limits = {}) const {
        validate_count(phases);
        std::vector<Time> z(phases.size());
        for (std::size_t i = 0; i < phases.size(); ++i) {
            // Reduce separately before subtracting, even for INT64_MIN offsets.
            const Time phase = floor_mod(phases[i], periods_[i]);
            const Time prefix = floor_mod(prefixes_[i], periods_[i]);
            z[i] = floor_mod(phase - prefix, periods_[i]); // Eq. (10)
        }
        return evaluate_zerolet(std::move(z), limits);
    }

    Time hyperperiod() const { return hyperperiod_; }
    Time forward_job_chains_per_vector() const { return jobs_per_vector_; }
    std::uint64_t phase_vector_count() const { return phase_count_; }
    const std::vector<Time>& domains() const { return domains_; }
    const std::vector<Time>& lengths() const { return lengths_; }

private:
    Time evaluate_zerolet_mrt(const std::vector<Time>& z) const {
        Time longest = 0;
        // Exactly H/T_1 first-task jobs, including j_1=0. No warm-up or k>=0 clamp.
        // u+floor_mod(z_i-u,T_i) is exactly z_i+ceil((u-z_i)/T_i)*T_i.
        // Same 64-bit modulo primitive as the archived backward evaluator,
        // with the direction changed from latest predecessor to earliest reader.
        for (Time j = 0; j < jobs_per_vector_; ++j) {
            const Time first = j * periods_.front() + z.front();
            Time current = first;
            for (std::size_t i = 1; i < periods_.size(); ++i)
                current += floor_mod(z[i] - current, periods_[i]);
            longest = std::max(longest, current - first);
        }
        return longest + (periods_.front() - 1); // Eqs. (7), (15)
    }

    Result compose(Time zero_mrt, std::vector<Time> z) const {
        Result result;
        result.zerolet_mrt = zero_mrt;
        result.discrete_mrt = checked_add(zero_mrt, sum_lengths_); // Theorem 1
        result.latency = checked_add(result.discrete_mrt, 1); // one INPUT time unit
        result.zerolet_offsets = std::move(z);
        result.phases.resize(periods_.size());
        for (std::size_t i = 0; i < periods_.size(); ++i)
            result.phases[i] = floor_mod(result.zerolet_offsets[i] + prefixes_[i], periods_[i]);
        return result;
    }

    void validate_count(const std::vector<Time>& offsets) const {
        if (offsets.size() != periods_.size()) throw std::invalid_argument("offset count mismatch");
    }
    bool advance(std::vector<Time>& z) const {
        for (std::size_t i = z.size(); i-- > 1;) {
            if (++z[i] < domains_[i]) return true;
            z[i] = 0;
        }
        return false;
    }
    std::vector<Time> periods_, lengths_, domains_, prefixes_;
    Time hyperperiod_ = 1, sum_lengths_ = 0, jobs_per_vector_ = 0;
    std::uint64_t phase_count_ = 1;
};
} // namespace wang26
#endif
