# Written assignment 2

Xavier Gonzalez: xavier18
JS Paul: jspaul

## Question 1

## Question 2

## Question 3

We implemented `matMulTest`, which multiplied 5 2x2 matrices of the form {{2,2}, {0,2}}.

The test is meant to check if our implementations can still be correct in the context of matrix multplication, which involves mixing multiple linput variables. Matrix multiplication is also a very important primitive in machine learning.

I was able to verify correctnes of the solution by checking the output of the test against a hand-calculated result. The result of the test did not cause me to change my implementation, but it did give me more confidence that my implementation was correct.

It is interesting that the parallel implementations were slower than serial on this test. This is liekly because the number and size of the matrices multiplied together were small.

