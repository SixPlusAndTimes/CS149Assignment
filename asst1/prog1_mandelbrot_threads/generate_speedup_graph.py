#!/usr/bin/env python3
import matplotlib.pyplot as plt
import numpy as np

# Data collected from test runs
threads = [1, 2, 3, 4, 5, 6, 7, 8]
speedups = [0.98, 1.98, 1.66, 2.41, 2.47, 3.22, 3.33, 3.95]

# Create figure
fig, ax = plt.subplots(figsize=(10, 6))

# Plot actual speedup
ax.plot(threads, speedups, 'bo-', linewidth=2, markersize=8, label='Actual Speedup')

# Plot ideal linear speedup
ideal_speedup = threads
ax.plot(threads, ideal_speedup, 'r--', linewidth=2, label='Ideal Linear Speedup')

# Add 4-core limit line
ax.axhline(y=4, color='g', linestyle=':', linewidth=2, label='4-Core Physical Limit')

# Configure the plot
ax.set_xlabel('Number of Threads', fontsize=12, fontweight='bold')
ax.set_ylabel('Speedup (relative to serial)', fontsize=12, fontweight='bold')
ax.set_title('Mandelbrot Set Computation: Speedup vs. Number of Threads', fontsize=14, fontweight='bold')
ax.grid(True, alpha=0.3)
ax.legend(fontsize=11)
ax.set_xticks(threads)
ax.set_ylim(0, 9)

# Add value labels on points
for i, (t, s) in enumerate(zip(threads, speedups)):
    ax.text(t, s + 0.15, f'{s:.2f}x', ha='center', fontsize=9)

plt.tight_layout()
plt.savefig('speedup_graph.png', dpi=150)
print("Graph saved as speedup_graph.png")

# Print analysis
print("\n=== SPEEDUP ANALYSIS ===")
print(f"{'Threads':<8} {'Speedup':<12} {'Efficiency':<12} {'Notes':<30}")
print("-" * 62)
for t, s in zip(threads, speedups):
    efficiency = (s / t) * 100
    if t == 1:
        notes = "Baseline"
    elif t == 2:
        notes = "Nearly linear, ~2x gain"
    elif t == 3:
        notes = "SUBLINEAR DROP! (1.66x < 3x)"
    elif t == 4:
        notes = "Good, ~4 cores utilized"
    elif t <= 8:
        notes = "Below linear, hyperthreading"
    print(f"{t:<8} {s:<12.2f}x {efficiency:<12.1f}% {notes:<30}")

print("\n=== KEY OBSERVATIONS ===")
print("1. Super-linear at 2 threads: 1.98x (very close to 2x)")
print("2. ANOMALY at 3 threads: 1.66x (significant drop)")
print("3. 4 threads: 2.41x (close to physical core count)")
print("4. 5-8 threads: Hyperthreading, diminishing returns")
print("5. 8 threads: 3.95x (approaching practical limit of 4x for 4 cores)")
