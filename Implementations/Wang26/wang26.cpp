// SPDX-License-Identifier: MIT
// Upstream copyright (c) 2026 Shumo Wang
// C++ implementation and additions copyright (c) 2026 Jongwon Lee, Namcheol Lee, Saehwa Kim, and Seongsoo Hong
// Preserve LICENSE.upstream and ../../LICENSE; origin and modifications: README.md.
// Wang'26 C++17 executable and benchmark entry point. See wang26.hpp.
#include "wang26.hpp"
#include <chrono>
#include <iostream>
#include <string>

namespace {
using wang26::Time;
void array(const std::vector<Time>& values) {
    std::cout << '[';
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i) std::cout << ',';
        std::cout << values[i];
    }
    std::cout << ']';
}
Time read_time() {
    std::string token;
    if (!(std::cin >> token)) throw std::invalid_argument("incomplete input record");
    std::size_t used = 0;
    const auto value = std::stoll(token, &used);
    if (used != token.size()) throw std::invalid_argument("expected integer input");
    return value;
}
std::uint64_t read_limit(const char* text) {
    const std::string token(text);
    if (token.empty() || token.front() == '-') throw std::invalid_argument("invalid work limit");
    std::size_t used = 0;
    const auto value = std::stoull(token, &used);
    if (used != token.size()) throw std::invalid_argument("invalid work limit");
    return value;
}
} // namespace

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    try {
        bool stream = false, explicit_lengths = false, physical = false, zero = false;
        wang26::Limits limits;
        for (int i = 1; i < argc; ++i) {
            const std::string arg(argv[i]);
            if (arg == "--stream") stream = true;
            else if (arg == "--lets") explicit_lengths = true;
            else if (arg == "--evaluate") physical = true;
            else if (arg == "--zero-evaluate") zero = true;
            else if ((arg == "--max-phase-vectors" || arg == "--max-forward-chains") && i + 1 < argc) {
                const auto value = read_limit(argv[++i]);
                if (arg == "--max-phase-vectors") limits.phase_vectors = value;
                else limits.forward_job_chains = value;
            } else throw std::invalid_argument("usage: wang26 [--stream] [--lets] "
                "[--evaluate | --zero-evaluate] [--max-phase-vectors N] [--max-forward-chains N]");
        }
        if (physical && zero) throw std::invalid_argument("choose one evaluation mode");
        do {
            std::cin >> std::ws;
            if (stream && std::cin.eof()) return 0;
            const Time n = read_time();
            if (n < 1 || static_cast<std::uint64_t>(n) > std::vector<Time>().max_size())
                throw std::invalid_argument("invalid task count");
            std::vector<Time> t(static_cast<std::size_t>(n)), lengths, offsets;
            for (auto& v : t) v = read_time();
            if (explicit_lengths) {
                lengths.resize(t.size());
                for (auto& v : lengths) v = read_time();
            }
            if (physical || zero) {
                offsets.resize(t.size());
                for (auto& v : offsets) v = read_time();
            }
            if (!stream) {
                std::cin >> std::ws;
                if (!std::cin.eof()) throw std::invalid_argument("unexpected trailing input");
            }
            // Same timing boundary as the existing CLI: construction, solve, result extraction.
            const auto begin = std::chrono::steady_clock::now();
            wang26::Solver solver(std::move(t), std::move(lengths));
            const auto result = physical ? solver.evaluate_phases(offsets, limits) :
                                zero ? solver.evaluate_zerolet(offsets, limits) : solver.solve(limits);
            const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now() - begin).count();
            std::cout << "{\"status\":\"ok\",\"mode\":\"" << (physical ? "evaluate" : zero ? "zero-evaluate" : "optimize")
                      << "\",\"latency\":" << result.latency
                      << ",\"discrete_mrt\":" << result.discrete_mrt
                      << ",\"zerolet_mrt\":" << result.zerolet_mrt << ",\"phases\":";
            array(result.phases);
            std::cout << ",\"zerolet_offsets\":"; array(result.zerolet_offsets);
            std::cout << ",\"let_intervals\":"; array(solver.lengths());
            std::cout << ",\"phase_domains\":"; array(solver.domains());
            std::cout << ",\"hyperperiod\":" << solver.hyperperiod()
                      << ",\"evaluated_phase_vectors\":" << result.evaluated_phase_vectors
                      << ",\"forward_job_chains_per_vector\":" << solver.forward_job_chains_per_vector();
            std::cout << ",\"runtime_ns\":" << ns << "}\n" << std::flush;
        } while (stream);
    } catch (const wang26::LimitExceeded& e) {
        std::cout << "{\"status\":\"limit\"}\n";
        std::cerr << e.what() << '\n'; return 4;
    } catch (const std::logic_error& e) {
        std::cout << "{\"status\":\"invalid\"}\n";
        std::cerr << e.what() << '\n'; return 2;
    } catch (const std::overflow_error& e) {
        std::cout << "{\"status\":\"invalid\"}\n";
        std::cerr << e.what() << '\n'; return 2;
    } catch (const std::exception& e) {
        std::cout << "{\"status\":\"error\"}\n";
        std::cerr << e.what() << '\n'; return 3;
    }
}
