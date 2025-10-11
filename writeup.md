# Written assignment 2

Xavier Gonzalez: xavier18
JS Paul: jspaul

## Question 1

## Question 2

There are four different implementations in part A:
* serial
* parallel and always spawn
* parallel and use a thread pool and spin
* parallel and thread pool and sleep.

There are certain test cases where serial performs beter than any parallel implementation. This is because there is a certain overhead to spawning threads. 

For example, in `super super lightweight`, on myth, serials runs in around 9 ms, while the best (most sophisticated) parallel implementaion is around 10 ms. 

In contrast, on myth, `fibonacci` takes around 1 second for serial, and only around 300-500 ms for the parallel implementations.

Let's compare and contrast these two tasks as way of understanding the benefits of serial implementation vs spawning threads.

`fibonacci` is implemented recursively and involves computing the 25th element of the fibonacci sequence. As a result, there is a lot of arithmetic to do in either implementation. Moreover, if we spawn threads and do them in parallel, each thread still has a lot of arithmetic to do, making the overhead of spawning thredas worth it. That's why on this test, parallel is better than sequential.

`super super lightweight`, on the other hand, is a task that does no arithmetic at all. It only involves copying data from one array to another. Each operation is so lightweight that the overhead of spawning a thread is often not worth it. However, some amount of tuning matters. For example, on the myth machine, if we spawn with 3 threads, we can actualy get parallel+sleeping to be slightly faster than serial. So there is some balance: doing all of the operation in parallel is less costly than spawning one thread. But, already by the tiem we are spawning 4 threads, the overhead of spawning is too much and serial is faster.

<TODO: still need to discuss spawn every launch vs thread pool>

In what situations did the spawn-every-launch implementation perform as well as the more advanced parallel implementations that use a thread pool? When does it not?

it looks like we currently fail on `spin_betwee_run_calls`

always spawn is roughly comparable on `ping_pong` (equal and unequal), `recursive_fibonacci`, `math_operations_in_tight_for_loop_reduction_tree`, and `mandelbrot_chunked`. On the other tasks, thread poling does better. 

```
(base) xavier18@myth62:~/cs149/asst2/part_a$ python3 ../tests/run_test_harness.py
runtasks_ref
Linux x86_64
================================================================================
Running task system grading harness... (11 total tests)
  - Detected CPU with 8 execution contexts
  - Task system configured to use at most 8 threads
================================================================================
================================================================================
Executing test: super_super_light...
Reference binary: ./runtasks_ref_linux
Results for: super_super_light
                                        STUDENT   REFERENCE   PERF?
[Serial]                                6.452     8.977       0.72  (OK)
[Parallel + Always Spawn]               52.346    54.506      0.96  (OK)
[Parallel + Thread Pool + Spin]         13.075    13.471      0.97  (OK)
[Parallel + Thread Pool + Sleep]        8.796     10.142      0.87  (OK)
================================================================================
Executing test: super_light...
Reference binary: ./runtasks_ref_linux
Results for: super_light
                                        STUDENT   REFERENCE   PERF?
[Serial]                                58.869    67.647      0.87  (OK)
[Parallel + Always Spawn]               61.208    62.724      0.98  (OK)
[Parallel + Thread Pool + Spin]         21.467    28.179      0.76  (OK)
[Parallel + Thread Pool + Sleep]        23.472    22.64       1.04  (OK)
================================================================================
Executing test: ping_pong_equal...
Reference binary: ./runtasks_ref_linux
Results for: ping_pong_equal
                                        STUDENT   REFERENCE   PERF?
[Serial]                                947.758   1121.57     0.85  (OK)
[Parallel + Always Spawn]               285.323   309.806     0.92  (OK)
[Parallel + Thread Pool + Spin]         255.319   333.545     0.77  (OK)
[Parallel + Thread Pool + Sleep]        259.204   281.96      0.92  (OK)
================================================================================
Executing test: ping_pong_unequal...
Reference binary: ./runtasks_ref_linux
Results for: ping_pong_unequal
                                        STUDENT   REFERENCE   PERF?
[Serial]                                1735.495  1735.359    1.00  (OK)
[Parallel + Always Spawn]               443.876   441.345     1.01  (OK)
[Parallel + Thread Pool + Spin]         420.18    499.972     0.84  (OK)
[Parallel + Thread Pool + Sleep]        451.679   410.736     1.10  (OK)
================================================================================
Executing test: recursive_fibonacci...
Reference binary: ./runtasks_ref_linux
Results for: recursive_fibonacci
                                        STUDENT   REFERENCE   PERF?
[Serial]                                997.595   1407.872    0.71  (OK)
[Parallel + Always Spawn]               262.939   369.192     0.71  (OK)
[Parallel + Thread Pool + Spin]         261.24    403.872     0.65  (OK)
[Parallel + Thread Pool + Sleep]        274.438   369.191     0.74  (OK)
================================================================================
Executing test: math_operations_in_tight_for_loop...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop
                                        STUDENT   REFERENCE   PERF?
[Serial]                                583.418   607.693     0.96  (OK)
[Parallel + Always Spawn]               433.88    433.7       1.00  (OK)
[Parallel + Thread Pool + Spin]         219.169   293.197     0.75  (OK)
[Parallel + Thread Pool + Sleep]        232.115   244.058     0.95  (OK)
================================================================================
Executing test: math_operations_in_tight_for_loop_fewer_tasks...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop_fewer_tasks
                                        STUDENT   REFERENCE   PERF?
[Serial]                                581.68    606.305     0.96  (OK)
[Parallel + Always Spawn]               517.557   517.022     1.00  (OK)
[Parallel + Thread Pool + Spin]         395.95    378.677     1.05  (OK)
[Parallel + Thread Pool + Sleep]        412.066   370.162     1.11  (OK)
================================================================================
Executing test: math_operations_in_tight_for_loop_fan_in...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop_fan_in
                                        STUDENT   REFERENCE   PERF?
[Serial]                                298.397   310.336     0.96  (OK)
[Parallel + Always Spawn]               115.632   115.599     1.00  (OK)
[Parallel + Thread Pool + Spin]         91.905    109.123     0.84  (OK)
[Parallel + Thread Pool + Sleep]        107.233   95.586      1.12  (OK)
================================================================================
Executing test: math_operations_in_tight_for_loop_reduction_tree...
Reference binary: ./runtasks_ref_linux
Results for: math_operations_in_tight_for_loop_reduction_tree
                                        STUDENT   REFERENCE   PERF?
[Serial]                                297.926   310.413     0.96  (OK)
[Parallel + Always Spawn]               88.666    88.2        1.01  (OK)
[Parallel + Thread Pool + Spin]         83.621    102.001     0.82  (OK)
[Parallel + Thread Pool + Sleep]        95.511    85.512      1.12  (OK)
================================================================================
Executing test: spin_between_run_calls...
Reference binary: ./runtasks_ref_linux
Segmentation fault (core dumped)
Command './runtasks -n 8 spin_between_run_calls' returned non-zero exit status 139.
STUDENT solution failed correctness check!
Results for: spin_between_run_calls
                                        STUDENT   REFERENCE   PERF?
[Serial]                                359.486   500.199     0.72  (OK)
[Parallel + Always Spawn]               183.38    255.162     0.72  (OK)
[Parallel + Thread Pool + Spin]         376.796   517.799     0.73  (OK)
[Parallel + Thread Pool + Sleep]        183.47    254.929     0.72  (OK)
================================================================================
Executing test: mandelbrot_chunked...
Reference binary: ./runtasks_ref_linux
Results for: mandelbrot_chunked
                                        STUDENT   REFERENCE   PERF?
[Serial]                                386.885   393.097     0.98  (OK)
[Parallel + Always Spawn]               53.426    54.426      0.98  (OK)
[Parallel + Thread Pool + Spin]         53.382    63.33       0.84  (OK)
[Parallel + Thread Pool + Sleep]        53.35     54.352      0.98  (OK)
================================================================================
Overall performance results
[Serial]                                : All passed Perf
[Parallel + Always Spawn]               : All passed Perf
[Parallel + Thread Pool + Spin]         : All passed Perf
[Parallel + Thread Pool + Sleep]        : All passed Perf
```


## Question 3

We implemented `matMulTest`, which multiplied 5 2x2 matrices of the form {{2,2}, {0,2}}.

The test is meant to check if our implementations can still be correct in the context of matrix multplication, which involves mixing multiple linput variables. Matrix multiplication is also a very important primitive in machine learning.

I was able to verify correctnes of the solution by checking the output of the test against a hand-calculated result. The result of the test did not cause me to change my implementation, but it did give me more confidence that my implementation was correct.

It is interesting that the parallel implementations were slower than serial on this test. This is liekly because the number and size of the matrices multiplied together were small.

