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

`always spawn` is roughly comparable on `ping_pong` (equal and unequal), `recursive_fibonacci`, `math_operations_in_tight_for_loop_reduction_tree`, and `mandelbrot_chunked`. On the other tasks, thread poling does better. 

A great case study is look at `super_light` vs `ping_pong_equal`. Both of these tasks are identical, except that `ping_pong_equal` has 2^19 elements in the arrays, while `super_light` has 2^15. In `super_light`, the thread pool implementations are much better than always spawn. But in `ping_pong_equal`, they are roughly comparable. This is because in `super_light`, each run of the thread is not so heavy, so the overhead of spawning is more significant. In contrast, in `ping_pong_equal`, each thread has a lot more work to do, so the overhead of spawning is less significant. We see this theme of very heavy compute threads not showing so much of a benefit from thread pooling in the other tests as well, as `recursive_fibonacci`,`math_operations_in_tight_for_loop_reduction_tree`, and `mandelbrot_chunked` all involve heavy computation for each task.

## Question 3

### part a

A test we implemented for part a was `matMulTest`, which multiplied 5 2x2 matrices of the form {{2,2}, {0,2}}.

The test is meant to check if our implementations can still be correct in the context of matrix multplication, which involves mixing multiple linput variables. Matrix multiplication is also a very important primitive in machine learning.

I was able to verify correctnes of the solution by checking the output of the test against a hand-calculated result. The result of the test did not cause me to change the implementation, but it did give me more confidence that the implementation was correct.

It is interesting that the parallel implementations were slower than serial on this test. This is likley because the number and size of the matrices multiplied together were small.

### part b

