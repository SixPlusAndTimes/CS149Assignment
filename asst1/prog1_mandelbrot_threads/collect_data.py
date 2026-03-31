#!/usr/bin/env python3
import subprocess
import re
import matplotlib.pyplot as plt
import numpy as np

def run_test(threads, view=1):
    """Run mandelbrot with given thread count and return timing data"""
    cmd = f"cd /root/cs149/assignment/asst1/prog1_mandelbrot_threads && ./mandelbrot -t {threads} -v {view} 2>&1 | grep -E '(serial|thread.*ms|speedup)'"

    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    output = result.stdout

    # Extract timings
    serial_match = re.search(r'\[mandelbrot serial\]:\s+\[([\d.]+)\] ms', output)
    thread_match = re.search(r'\[mandelbrot thread\]:\s+\[([\d.]+)\] ms', output)
    speedup_match = re.search(r'\(([\d.]+)x speedup', output)

    serial_time = float(serial_match.group(1)) if serial_match else None
    thread_time = float(thread_match.group(1)) if thread_match else None
    speedup = float(speedup_match.group(1)) if speedup_match else None

    return serial_time, thread_time, speedup

def collect_data():
    """Collect timing data for different thread counts"""
    thread_counts = [1, 2, 3, 4, 5, 6, 7, 8, 16]
    results_view1 = []
    results_view2 = []

    print("Collecting timing data...")

    for threads in thread_counts:
        print(f"Testing {threads} threads...")

        # View 1
        serial1, thread1, speedup1 = run_test(threads, view=1)
        results_view1.append((threads, serial1, thread1, speedup1))

        # View 2
        serial2, thread2, speedup2 = run_test(threads, view=2)
        results_view2.append((threads, serial2, thread2, speedup2))

    return results_view1, results_view2

def create_speedup_graph(results_view1, results_view2):
    """Create speedup graph for both views"""
    threads1 = [r[0] for r in results_view1]
    speedups1 = [r[3] for r in results_view1]

    threads2 = [r[0] for r in results_view2]
    speedups2 = [r[3] for r in results_view2]

    plt.figure(figsize=(10, 6))
    plt.plot(threads1, speedups1, 'b-o', label='View 1 (Default)', linewidth=2, markersize=6)
    plt.plot(threads2, speedups2, 'r-s', label='View 2 (Zoomed)', linewidth=2, markersize=6)

    # Add ideal linear speedup line
    ideal_threads = np.array([1, 2, 4, 8, 16])
    ideal_speedup = ideal_threads
    plt.plot(ideal_threads, ideal_speedup, 'g--', label='Ideal Linear Speedup', alpha=0.7)

    plt.xlabel('Number of Threads')
    plt.ylabel('Speedup (x)')
    plt.title('Mandelbrot Generation Speedup with Interleaved Work Assignment')
    plt.legend()
    plt.grid(True, alpha=0.3)
    plt.xticks([1, 2, 3, 4, 5, 6, 7, 8, 16])

    # Add speedup values as text
    for i, (t, s) in enumerate(zip(threads1, speedups1)):
        plt.text(t, s + 0.1, f'{s:.2f}x', ha='center', va='bottom', fontsize=9)

    for i, (t, s) in enumerate(zip(threads2, speedups2)):
        plt.text(t, s - 0.2, f'{s:.2f}x', ha='center', va='top', fontsize=9)

    plt.tight_layout()
    plt.savefig('/root/cs149/assignment/asst1/prog1_mandelbrot_threads/speedup_graph.png', dpi=150, bbox_inches='tight')
    plt.show()

def print_results(results_view1, results_view2):
    """Print timing results in a nice format"""
    print("\n" + "="*60)
    print("MANDELBROT THREADING PERFORMANCE RESULTS")
    print("="*60)

    print("\nVIEW 1 (Default):")
    print("Threads | Serial Time | Thread Time | Speedup")
    print("--------|-------------|-------------|--------")
    for threads, serial, thread, speedup in results_view1:
        print(f"{threads:4d} | {serial:>11.2f} | {thread:>11.2f} | {speedup:>7.2f}")

    print("\nVIEW 2 (Zoomed):")
    print("Threads | Serial Time | Thread Time | Speedup")
    print("--------|-------------|-------------|--------")
    for threads, serial, thread, speedup in results_view2:
        print(f"{threads:4d} | {serial:>11.2f} | {thread:>11.2f} | {speedup:>7.2f}")

if __name__ == "__main__":
    results_view1, results_view2 = collect_data()
    print_results(results_view1, results_view2)
    create_speedup_graph(results_view1, results_view2)
    print_results(results_view1, results_view2)
    create_speedup_graph(results_view1, results_view2)
    print("\nSpeedup graph saved as 'speedup_graph.png'")