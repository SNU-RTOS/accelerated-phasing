// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Jongwon Lee, Namcheol Lee, Saehwa Kim, and Seongsoo Hong
// License: ../../LICENSE. Method and implementation notes: README.md.
// Proposed: exact fixed-order search with singleton acceleration (SA)
// and lower-bound pruning (LBP). Extension order is right-then-left.
// F5 Lxx refers to the current 26-line Fig. 5, not C++ source line numbers.
// C++ depth d is paper depth d+1; levels_[d].sigma+1 is paper sigma_{d+1}.
// Q[j] is the paper's ordered endpoint pair (s^(j),w^(j)); M is H/T_p.
// alpha/beta are the assigned left/right boundary phases of the task subchain.
// No full working phase vector is stored: candidate phases live in recursion
// frames. An improving descendant recovers its remaining extension-order phases;
// callers recover ancestor phases on return. phi_star_ is complete at root return,
// as described in the paper's Complexity Analysis for Fig. 5 lines 12 and 22.
// Precomputation, lazy candidate deduplication and phase recovery are interleaved
// differently from the pseudocode; each F5 tag identifies its semantic operation.
// Counts: ENABLE_COUNTS=1 enables diagnostics; the default 0 removes them
// at preprocessing time, including storage and result/output fields.
// Input contract: N>=3, positive integer periods, and values whose intermediate
// endpoint, hyperperiod and latency arithmetic fits in int64_t. The caller
// is responsible for validating this arithmetic range.
// Ablations only: DISABLE_SINGLETON_OPT disables both single/path SA;
//                DISABLE_LBP disables child/state lower-bound pruning.
#ifndef ENABLE_COUNTS
#define ENABLE_COUNTS 0
#endif
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_set>

namespace {
using Time = std::int64_t;
struct Pair { Time s, w; };
#if ENABLE_COUNTS
// Diagnostic counters; no candidate sorting/scoring instrumentation remains.
struct Stats {
    std::uint64_t visited_nodes=0, generated_nodes=0, pruned_nodes=0;
    std::uint64_t full_phase_latency_evaluations=0;
    std::uint64_t evaluated_leaves=0, materialized_states=0, endpoint_updates=0;
    std::uint64_t singleton_completions=0, skipped_suffix_steps=0;
};
#endif
#if ENABLE_COUNTS
struct Result { Time latency; std::vector<Time> phases; Stats stats; };
#else
struct Result { Time latency; std::vector<Time> phases; };
#endif
Time mod(Time x, Time m) { Time r=x%m; return r<0 ? r+m : r; }

class Solver {
    // [F5 L02-L05, L13] Per-depth constants, SA flags and ordered endpoint state.
    // Level.g is the incoming extension's g; Level.B is the remaining bound B.
    struct Level {
        std::size_t sigma=0, M=1;
        Time H=1, g=1, B=0;
        bool right=true, sgl=false, path=true;
        std::vector<Pair> Q;
        std::vector<Time> endpoint_residues, chain_lengths;
        std::unordered_set<Time> seen;
    };
    // [F5 L06] Zero incumbent phases; INT64_MAX represents +infinity.
    // The working-vector assignments are represented by recursion-frame phases.
    std::vector<Time> T_, phi_star_;
    std::vector<Level> levels_;
    Time L_star_=std::numeric_limits<Time>::max();
#if ENABLE_COUNTS
    Stats stats_;
#endif
public:
    // [F5 L01-L06 BEGIN] Constructor and member initializers implement setup.
    explicit Solver(std::vector<Time> periods)
        : T_(std::move(periods)), phi_star_(T_.size(),0), levels_(T_.size()) {
        // [F5 L01] First maximum pivot and the paper's fixed extension order.
        const std::size_t p=std::max_element(T_.begin(),T_.end())-T_.begin();
        set_fixed_order(p);
        // [F5 L02] H and M at each depth; g at nonroot levels is incoming gcd.
        // U is read from the adjacent boundary period below, not stored separately.
        Time h=T_[p];
        for (auto& l:levels_) {
            const Time t=T_[l.sigma];
            l.g=std::gcd(h,t);
            h=(h/l.g)*t;
            l.H=h;
            l.M=static_cast<std::size_t>(h/T_[p]);
        }
        // [F5 L02] B_N=0 by initialization; accumulate B backward.
        // B is also required for exact path SA with LBP disabled.
        for (std::size_t i=levels_.size()-1;i>0;--i)
            levels_[i-1].B=levels_[i].B+(2*T_[levels_[i].sigma]-levels_[i].g);
#ifndef DISABLE_SINGLETON_OPT
        // [F5 L03-L04] Compute the two flag arrays in one reverse traversal.
        for (std::size_t i=levels_.size()-1;i>0;--i) {
            auto& l=levels_[i];
            const std::size_t boundary=l.right ? l.sigma-1 : l.sigma+1;
            // [F5 L03] sgl_k=[g_k divides U_k].
            l.sgl=(T_[boundary]%l.g==0);
            // [F5 L04] path_d=sgl_(d+1) AND path_(d+1); terminal true is neutral.
            levels_[i-1].path=l.sgl && l.path;
        }
#endif
        // [F5 L05] Q_1=((0),(2*T_p)); solve() supplies zero boundary phases.
        levels_[0].Q.push_back({0,2*T_[p]});
    }
    // [F5 L01-L06 END]
    Result solve() {
        // [F5 L07] Search(1,Q_1,2*T_p), with zero phases at both root boundaries.
        search(0,levels_[0].Q[0].w,0,0);
        // [F5 L08] The full optimal phase vector is recovered before this return.
#if ENABLE_COUNTS
        return {L_star_,std::move(phi_star_),stats_};
#else
        return {L_star_,std::move(phi_star_)};
#endif
    }
private:
    // [F5 L01 helper] Right to the last task, then left back to the first.
    void set_fixed_order(std::size_t p) {
        levels_[0].sigma=p;
        std::size_t d=1;
        for (std::size_t i=p+1;i<T_.size();++i) levels_[d++].sigma=i;
        for (std::size_t i=p;i>0;) {
            levels_[d].right=false;
            levels_[d++].sigma=--i;
        }
    }

    // [F5 L09-L26 BEGIN]
    // [F5 L09] Search: Q_d is levels_[d].Q; L_d is passed explicitly.
    // A true return means this invocation strictly improved the incumbent;
    // the caller records its own candidate phase during backtracking.
    bool search(std::size_t d, Time L_d, Time alpha, Time beta) {
#if ENABLE_COUNTS
        ++stats_.visited_nodes;
#endif
        auto& l=levels_[d];
#ifndef DISABLE_SINGLETON_OPT
        // [F5 L10-L12 BEGIN: path SA]
        // [F5 L10] All remaining extensions satisfy the singleton condition.
        if (l.path) {
            const Time completed_latency=L_d+l.B;
#if ENABLE_COUNTS
            ++stats_.evaluated_leaves;
            ++stats_.full_phase_latency_evaluations;
            ++stats_.singleton_completions;
            stats_.skipped_suffix_steps+=levels_.size()-d-1;
#endif
#ifdef DISABLE_LBP
            // Without LBP, entry need not improve the incumbent (SA-only ablation).
            if (completed_latency>=L_star_) return false;
#endif
            // [F5 L11] Assign the unique remaining phases in task-extension order.
            // Boundary phases suffice; no remaining endpoint states are built.
            for (std::size_t i=d+1;i<levels_.size();++i) {
                const auto& step=levels_[i];
                const Time phase=(step.right ? beta : alpha)%step.g;
                phi_star_[step.sigma]=phase;
                if (step.right) beta=phase;
                else alpha=phase;
            }
            // [F5 L12] Record exact completion latency and signal improvement.
            // Current and ancestor edge phases are filled by callers on return.
            L_star_=completed_latency;
            return true;
        }
        // [F5 L10-L12 END]
#endif
        // [F5 L13-L17 BEGIN]
        // [F5 L13] Select W for right or S for left on demand; no E copy is made.
        auto& next=levels_[d+1];
        const Time t=T_[next.sigma], g=next.g, growth=2*t-g;
        const bool right=next.right;
#ifndef DISABLE_SINGLETON_OPT
        const bool use_singleton=next.sgl; // [F5 L14]
#else
        const bool use_singleton=false;
#endif
        // [F5 L16] Exact single-SA latency; independent of the L15 phase assignment.
        const Time singleton_L_prime=use_singleton ? L_d+growth : 0;
        l.endpoint_residues.resize(use_singleton ? 1 : l.M);
        if (use_singleton) {
            // [F5 L15] Phi={assigned boundary phase mod g}; phases are nonnegative.
            l.endpoint_residues[0]=(right ? beta : alpha)%g;
        } else {
            // [F5 L17] Store endpoint residues in order, possibly with duplicates.
            // Set membership is checked lazily at L18; this preserves first occurrence.
            // Cached chain lengths prepare the exact-latency scan at L20.
            l.chain_lengths.resize(l.M);
            for (std::size_t j=0;j<l.M;++j) {
                l.endpoint_residues[j]=mod(right ? l.Q[j].w : l.Q[j].s,g);
                l.chain_lengths[j]=l.Q[j].w-l.Q[j].s;
            }
            // Singleton candidates never prepare or use a hash table.
            l.seen.clear();
            l.seen.reserve(l.M);
        }
        // [F5 L13-L17 END]
        // [F5 L20 helper] Exact L^+(phase) from the paper's extension formula.
        // Both residues lie in [0,g), so a negative difference needs one +g.
        auto exact_child_latency = [&](Time phase) {
            Time L_prime=0;
            for (std::size_t j=0;j<l.M;++j) {
                Time wait=right ? phase-l.endpoint_residues[j] : l.endpoint_residues[j]-phase;
                if (wait<0) wait+=g;
                L_prime=std::max(L_prime,l.chain_lengths[j]+growth+wait);
            }
            return L_prime;
        };

        // [F5 L18-L24 BEGIN]
        bool improved=false;
        // [F5 L18] Examine candidates in first-occurrence order.
        for (Time phase:l.endpoint_residues) {
            // [F5 L17-L18] Eliminate duplicate residues before candidate evaluation.
            // Use hash membership only; never iterate or sort the hash table.
            if (!use_singleton && !l.seen.insert(phase).second) continue;
            // [F5 L19] Local phase is this frame's current task-phase assignment;
            // alpha/beta carry the boundaries needed by subsequent SA steps.
#if ENABLE_COUNTS
            ++stats_.generated_nodes;
#endif
            // [F5 L20] Same exact-latency calculation with LBP on or off.
            Time L_prime=use_singleton ? singleton_L_prime : exact_child_latency(phase);
#if ENABLE_COUNTS
            // Count exact complete-assignment latency BEFORE child-level LBP.
            if (d+2==levels_.size()) ++stats_.full_phase_latency_evaluations;
#endif
#ifndef DISABLE_LBP
            // [F5 L21] L_prime>=L_star_-B_child is L_prime+B_child>=L_star_.
            // Use the current incumbent and prune before constructing child Q.
            const Time cutoff=L_star_-next.B;
#if ENABLE_COUNTS
            if (L_prime>=cutoff) { ++stats_.pruned_nodes; continue; }
#else
            if (L_prime>=cutoff) continue;
#endif
#endif
            // [F5 L23 helper] Extend constructs Q only; L_prime is already exact.
            // shift=q*H_d and the inner-loop index is j; output k=j+q*M_d.
            auto materialize = [&]() {
                next.Q.resize(next.M);
#if ENABLE_COUNTS
                ++stats_.materialized_states;
                stats_.endpoint_updates+=next.M;
#endif
                std::size_t k=0;
                for (Time shift=0;shift<next.H;shift+=l.H) {
                    for (const Pair& e:l.Q) {
                        const Time s=e.s+shift,w=e.w+shift;
                        next.Q[k++]=right ? Pair{s,w+mod(phase-w,t)+t}
                                         : Pair{s-mod(s-phase,t)-t,w};
                    }
                }
            };
            // [F5 L22] A complete candidate needs no child endpoint state.
            if (d+2==levels_.size()) {
#if ENABLE_COUNTS
                ++stats_.evaluated_leaves;
#endif
                if (L_prime<L_star_) {
                    L_star_=L_prime;
                    phi_star_[next.sigma]=phase;
                    improved=true;
                }
            } else {
                // [F5 L23] Extend and recurse; recover this edge only on improvement.
                materialize();
                if (search(d+1,L_prime,right ? alpha : phase,right ? phase : beta)) {
                    phi_star_[next.sigma]=phase;
                    improved=true;
                }
            }
#ifndef DISABLE_LBP
            // [F5 L24] State LBP after descendant updates, with the latest L_star_.
            if (L_d+l.B==L_star_) break;
#endif
        }
        // [F5 L18-L24 END]
        // [F5 L25] Return to the caller; bool carries phase-recovery information.
        return improved;
    }
    // [F5 L26] End Search procedure.
};
// [F5 L09-L26 END]

// CLI adapter outside Fig. 5: parse records/options and time a solver call.
// N>=3, positive periods and arithmetic-range safety are caller preconditions;
// the parser does not validate all solver-domain conditions.
void run(std::vector<Time> periods) {
    const auto start=std::chrono::steady_clock::now();
    Solver solver(std::move(periods));
    const Result r=solver.solve();
    const auto ns=std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now()-start).count();
    std::cout<<"{\"status\":\"ok\",\"ordering\":\"fixed\",\"latency\":"<<r.latency<<",\"phases\":[";
    for (std::size_t i=0;i<r.phases.size();++i) {
        if (i) std::cout<<',';
        std::cout<<r.phases[i];
    }
#if ENABLE_COUNTS
    std::cout<<"],\"runtime_ns\":"<<ns
             <<",\"full_phase_latency_evaluations\":"<<r.stats.full_phase_latency_evaluations
             <<",\"visited_nodes\":"<<r.stats.visited_nodes
             <<",\"generated_nodes\":"<<r.stats.generated_nodes
             <<",\"pruned_nodes\":"<<r.stats.pruned_nodes
             <<",\"evaluated_leaves\":"<<r.stats.evaluated_leaves
             <<",\"materialized_states\":"<<r.stats.materialized_states
             <<",\"endpoint_updates\":"<<r.stats.endpoint_updates
             <<",\"singleton_completions\":"<<r.stats.singleton_completions
             <<",\"skipped_suffix_steps\":"<<r.stats.skipped_suffix_steps<<"}\n"<<std::flush;
#else
    std::cout<<"],\"runtime_ns\":"<<ns<<"}\n"<<std::flush;
#endif
}
} // namespace

int main(int argc,char** argv) {
    std::ios::sync_with_stdio(false); std::cin.tie(nullptr);
    bool stream=false;
    try {
        for (int i=1;i<argc;++i) {
            if (std::string(argv[i])=="--stream" && !stream) stream=true;
            else throw std::invalid_argument("usage: proposed [--stream]");
        }
        do {
            Time n;
            if (!(std::cin>>n)) {
                if (stream && std::cin.eof()) return 0;
                throw std::invalid_argument("expected task count and periods");
            }
            std::vector<Time> periods(static_cast<std::size_t>(n));
            for (auto& t:periods)
                if (!(std::cin>>t)) throw std::invalid_argument("expected integer period");
            if (!stream) {
                std::cin>>std::ws;
                if (!std::cin.eof()) throw std::invalid_argument("unexpected trailing input");
            }
            run(std::move(periods));
        } while (stream);
    } catch (const std::exception& e) {
        std::cout<<"{\"status\":\"error\"}\n";
        std::cerr<<e.what()<<'\n'; return 2;
    }
    return 0;
}
