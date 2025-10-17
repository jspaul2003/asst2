#include "tasksys.h"
#include <thread>
#include <mutex>
#include <stdlib.h>
#include <queue>
#include <condition_variable>
#include <iostream>

IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
ITaskSystem::~ITaskSystem() {}

/*
 * ================================================================
 * Serial task system implementation
 * ================================================================
 */

const char* TaskSystemSerial::name() {
    return "Serial";
}

TaskSystemSerial::TaskSystemSerial(int num_threads): ITaskSystem(num_threads) {
}

TaskSystemSerial::~TaskSystemSerial() {}

void TaskSystemSerial::run(IRunnable* runnable, int num_total_tasks) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                          const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemSerial::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelSpawn::name() {
    return "Parallel + Always Spawn";
}

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads): ITaskSystem(num_threads) {
    this->num_threads = num_threads;
    thread_pool = new std::thread[num_threads];
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {
    delete[] thread_pool;
}

void TaskSystemParallelSpawn::run_helper(IRunnable* runnable, int num_total_tasks, std::mutex* iteration_lock, int* job) {
    // loop thru all tasks, assign the task to thread
    int job_number;
    while(true) {
        iteration_lock->lock();
        if (*job >= num_total_tasks) {
            iteration_lock->unlock();
            break;
        }
        else {
            job_number = *job;
            (*job)++;
        }
        iteration_lock->unlock();
        runnable->runTask(job_number, num_total_tasks);
    }
}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {
    std::mutex* iteration_lock = new std::mutex(); // lock system
    int* job = new int(0); // which task are we on? or lowest unassigned task index
    for (int i = 0; i < num_threads; i++) {
        thread_pool[i] = std::thread(&TaskSystemParallelSpawn::run_helper, this, runnable, num_total_tasks, iteration_lock, job);
    }
    for (int i = 0; i < num_threads; i++) {
        thread_pool[i].join();
    }
    delete iteration_lock;
    delete job;
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSpinning::name() {
    return "Parallel + Thread Pool + Spin";
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads) {
    this->num_threads = num_threads;
    this->lock = new std::mutex();
    this->job_status = new std::condition_variable();
    this->kill_flag = false;
    this->thread_pool = new std::thread[num_threads];
    this->job_number = 0;
    this->jobs_complete = 0;
    this->num_total_tasks = 0;
    for (int i = 0; i < num_threads; i++) {
        thread_pool[i] = std::thread(&TaskSystemParallelThreadPoolSpinning::spinning, this);
    }
}

void TaskSystemParallelThreadPoolSpinning::spinning() {
    int task_id;
    int completed;
    int my_job_count;
    int BATCH_SIZE;
    int max_iter;
    while (true) {
        BATCH_SIZE = std::max(1, num_total_tasks/num_threads/3); 
        // 3 picked to even out thread batches in a manner similar to lecture
        // hopefully enabling work being spread out better
        // smaller batch size allows for more even work distribution but for small 
        // tasks it is a waste of time to run the thread for so little work...

        if (kill_flag) {
            break;
        }
        lock->lock();
        // grab next task
        task_id = job_number.fetch_add(BATCH_SIZE);
        if (task_id >= num_total_tasks) {
            lock->unlock();
            continue;
        }
        lock->unlock();
        
        max_iter = std::min(task_id+BATCH_SIZE, num_total_tasks);

        my_job_count = 0;
        for (int i = task_id; i < max_iter; i++) {
            runnable->runTask(i, num_total_tasks);
            my_job_count++;
        }

        completed = jobs_complete.fetch_add(my_job_count) + my_job_count;
        if (completed >= num_total_tasks) {
            lock->lock();
            job_status->notify_one();
            lock->unlock();
        }
    }
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {
    kill_flag = true;
    for (int i = 0; i < num_threads; i++) {
        thread_pool[i].join();
    }
    delete lock;
    delete[] thread_pool;
    delete job_status;
}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {
    this->lock->lock();
    this->runnable = runnable;
    this->jobs_complete.store(0);
    this->num_total_tasks = num_total_tasks;
    this->job_number.store(0);
    this->lock->unlock();
    
    // wait for all tasks to finish 
    std::unique_lock<std::mutex> job_lock_unique(*this->lock);
    this->job_status->wait(job_lock_unique, [&]{ return jobs_complete.load() >= num_total_tasks; });
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSleeping::name() {
    return "Parallel + Thread Pool + Sleep";
}

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads): ITaskSystem(num_threads) {
    //std::cerr << "Ligma" << std::endl;
    this->num_threads = num_threads;
    this->lock = new std::mutex();
    this->job_status = new std::condition_variable();
    this->task_status = new std::condition_variable();
    this->kill_flag = false;
    this->thread_pool = new std::thread[num_threads];
    this->job_number = 0;
    this->jobs_complete = 0;
    this->num_total_tasks = 0;
    for (int i = 0; i < num_threads; i++) {
        thread_pool[i] = std::thread(&TaskSystemParallelThreadPoolSleeping::spinning, this);
    }
}

void TaskSystemParallelThreadPoolSleeping::spinning() {
    int task_id;
    int completed;
    int my_job_count;
    int BATCH_SIZE;
    int max_iter;
    while (true) {

        // put thread to sleep
        // must wait for death or task
        std::unique_lock<std::mutex> task_lock_unique(*this->lock);
        this->task_status->wait(task_lock_unique, [&]{ 
            return (job_number < num_total_tasks) || kill_flag; 
        });
        BATCH_SIZE = std::max(1, num_total_tasks/num_threads/3); 
        // 3 picked to even out thread batches in a manner similar to lecture
        // hopefully enabling work being spread out better
        // smaller batch size allows for more even work distribution but for small 
        // tasks it is a waste of time to run the thread for so little work...

        if (kill_flag) {
            break;
        }

        // grab next task
        task_id = job_number.fetch_add(BATCH_SIZE);
        if (task_id >= num_total_tasks) {
            task_lock_unique.unlock();
            continue;
        }
        task_lock_unique.unlock();
        max_iter = std::min(task_id+BATCH_SIZE, num_total_tasks);

        my_job_count = 0;
        for (int i = task_id; i < max_iter; i++) {
            runnable->runTask(i, num_total_tasks);
            my_job_count++;
        }

        completed = jobs_complete.fetch_add(my_job_count) + my_job_count;
        if (completed >= num_total_tasks) {
            lock->lock();
            job_status->notify_one();
            lock->unlock();
        }
    }
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    //std::cerr << "Destroying TaskSystemParallelThreadPoolSleeping" << std::endl;
    kill_flag = true;

    // wake up the threads so they can die
    this->task_status->notify_all();

    // join the threads
    for (int i = 0; i < num_threads; i++) {
        thread_pool[i].join();
    }

    // free up resources
    delete lock;
    delete task_status;
    delete[] thread_pool;
    delete job_status;
    //std::cerr << "Gone" << std::endl;
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {
    this->lock->lock();
    this->runnable = runnable;
    this->jobs_complete.store(0);
    this->num_total_tasks = num_total_tasks;
    this->job_number.store(0);
    this->lock->unlock();
    
    this->task_status->notify_all();
    
    // wait for all tasks to finish 
    std::unique_lock<std::mutex> job_lock_unique(*this->lock);
    this->job_status->wait(job_lock_unique, [&]{ return jobs_complete.load() >= num_total_tasks; });
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {


    //
    // TODO: CS149 students will implement this method in Part B.
    //

    return 0;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //

    return;
}
