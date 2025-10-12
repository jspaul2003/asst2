import subprocess

tests = [
    "ping_pong_equal_async",
    "ping_pong_unequal_async",
    "super_light_async",
    "super_super_light_async",
    "recursive_fibonacci_async",
    "math_operations_in_tight_for_loop_async",
    "math_operations_in_tight_for_loop_fewer_tasks_async",
    "math_operations_in_tight_for_loop_fan_in_async",
    "math_operations_in_tight_for_loop_reduction_tree_async",
    "mandelbrot_chunked_async",
    "spin_between_run_calls_async",
    "simple_run_deps_test",
    "strict_diamond_deps_async",
    "strict_graph_deps_small_async",
    "strict_graph_deps_med_async",
    "strict_graph_deps_large_async",
]

for test in tests:
    print(f"\n--- running test: {test} ---")
    result = subprocess.run(["./runtasks", "-n", "8", test])
    if result.returncode != 0:
        print(f"L BOZO Test {test} failed with exit code {result.returncode}")
    else:
        print(f"W RIZZ Test {test} completed successfully")
