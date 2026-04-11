#!/bin/bash

tests=(
    "simple_test_sync"
    "simple_test_async"
    "ping_pong_equal"
    "ping_pong_unequal"
    "super_light"
    "super_super_light"
    "recursive_fibonacci"
    "math_operations_in_tight_for_loop"
    "math_operations_in_tight_for_loop_fewer_tasks"
    "math_operations_in_tight_for_loop_fan_in"
    "math_operations_in_tight_for_loop_reduction_tree"
    "spin_between_run_calls"
    "mandelbrot_chunked"
    "ping_pong_equal_async"
    "ping_pong_unequal_async"
    "super_light_async"
    "super_super_light_async"
    "recursive_fibonacci_async"
    "math_operations_in_tight_for_loop_async"
    "math_operations_in_tight_for_loop_fewer_tasks_async"
    "math_operations_in_tight_for_loop_fan_in_async"
    "math_operations_in_tight_for_loop_reduction_tree_async"
    "mandelbrot_chunked_async"
    "spin_between_run_calls_async"
    "simple_run_deps_test"
    "strict_diamond_deps_async"
    "strict_graph_deps_small_async"
    "strict_graph_deps_med_async"
    "strict_graph_deps_large_async"
)

output_file="benchmark_results.txt"
echo "" > $output_file

for multi in 1 2 4 8; do
    echo "Setting multiplier to $multi"
    sed -i "s/int multipler = [0-9]*/int multipler = $multi;/" tasksys.cpp
    make > /dev/null 2>&1
    echo "Multiplier $multi" >> $output_file
    for test in "${tests[@]}"; do
        output=$(./runtasks -n8 $test 2>&1)
        time=$(echo "$output" | grep "Parallel + Always Spawn" | sed 's/.*\[\([0-9.]*\)\].*/\1/')
        if [ -z "$time" ]; then
            time="FAIL"
        fi
        echo "$test $time" >> $output_file
    done
    echo "" >> $output_file
done

echo "Benchmarking complete. Results in $output_file"