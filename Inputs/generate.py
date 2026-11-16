#!/usr/bin/env python3
"""Generate the fixed CEC inputs in a separate directory; never overwrite existing CSVs."""
from __future__ import annotations
import argparse
import csv
import json
import math
import random
from pathlib import Path

LENGTHS = range(3, 9)
SAMPLES_PER_LENGTH = 500
ENGINE_PERIODS = (1, 2, 5, 10, 20, 50, 100, 200, 1000)
RANDOM_PERIODS = tuple(range(2, 101))
RANDOM_WEIGHTS = tuple(1 / t for t in RANDOM_PERIODS)
REAL_WORLD = (
    ('AEBS', (10, 50, 10, 50)),
    ('Camera-based Control', (1000, 2000, 40)),
    ('Brake-by-Wire', (20, 30, 40, 50, 60)),
    ('LiDAR-based Control', (33, 400, 15, 15, 5)),
    ('Pilot Request Handling', (40, 60, 100, 100, 100, 60, 40)),
)
def max_harmonic(rng, length):
    return tuple(rng.choice(ENGINE_PERIODS) for _ in range(length))

def xy_hyperperiod(rng, length):
    x = rng.randint(3, 20)
    choices, y = [], x + 1
    while len(choices) < 10:
        if math.gcd(x, y) == 1:
            choices.append(y)
        y += 1
    y = rng.choice(choices)
    q = rng.randint(1, 500 // (10 * y))
    s = 10 * q
    largest, second, hyperperiod = y * s, x * s, x * y * s
    remaining = [largest, second] + [t for t in range(1, second) if hyperperiod % t == 0]
    periods = [largest, second]
    periods.extend(rng.choice(remaining) for _ in range(length - 2))
    rng.shuffle(periods)
    return tuple(periods)

def randomly_sampled(rng, length):
    return tuple(rng.choices(RANDOM_PERIODS, weights=RANDOM_WEIGHTS, k=length))

WORKLOADS = (
    ('max_harmonic', 20261006, max_harmonic),
    ('xy_hyperperiod', 20261007, xy_hyperperiod),
    ('randomly_sampled', 20261008, randomly_sampled),
)

def generate(base_seed, draw):
    rows = []
    for length in LENGTHS:
        seed = base_seed + length + (1 if draw is max_harmonic and length == 8 else 0)
        rng, seen = random.Random(seed), set()
        while len(seen) < SAMPLES_PER_LENGTH:
            periods = draw(rng, length)
            if periods in seen:
                continue
            seen.add(periods)
            rows.append((length, len(seen), periods))
    return rows

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path,
                        default=Path(__file__).resolve().parents[1] / 'build/generated_inputs')
    args = parser.parse_args()
    datasets = {'application': [(len(t), i, t) for i, (_, t) in enumerate(REAL_WORLD, 1)]}
    datasets.update({name: generate(seed, draw) for name, seed, draw in WORKLOADS})
    if any((args.output_dir / f'{name}.csv').exists() for name in datasets):
        parser.error('CSV already exists; choose a different output directory.')
    args.output_dir.mkdir(parents=True, exist_ok=True)
    for name, rows in datasets.items():
        with (args.output_dir / f'{name}.csv').open('w', encoding='utf-8', newline='') as f:
            writer = csv.writer(f, lineterminator='\n')
            writer.writerow(['N', 'instance_id', 'periods_ms'])
            for n, ident, periods in rows:
                writer.writerow([n, ident, json.dumps(periods, separators=(',', ':'))])
        print(f'{name}: {len(rows)} CECs')

if __name__ == '__main__':
    main()
