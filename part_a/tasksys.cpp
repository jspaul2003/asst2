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
    this-> num_threads = num_threads;
    thread_pool = new std::thread[num_threads];
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

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
    this->job_number = 0;
    this->jobs_complete = 0;
    this->num_total_tasks = 0;
    thread_pool = new std::thread[num_threads];
    for (int i = 0; i < num_threads; i++) {
        thread_pool[i] = std::thread(&TaskSystemParallelThreadPoolSpinning::spinning, this);
    }
}

void TaskSystemParallelThreadPoolSpinning::spinning() {
    int task_id;
    while (true) {
        if (kill_flag) {
            break;
        }
        lock->lock();
        if (job_number >= num_total_tasks) {
            lock->unlock();
            std::this_thread::yield(); // schedule other threads!
            continue;
        }
        else {
            task_id = job_number;
            job_number++;
        }
        lock->unlock();
        runnable->runTask(task_id, num_total_tasks);
        lock->lock();
        jobs_complete++;
        if (jobs_complete == num_total_tasks) {
            job_status->notify_all();
        }
        lock->unlock();        
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
    this->jobs_complete = 0;
    this->num_total_tasks = num_total_tasks;
    this->job_number = 0;
    this->lock->unlock();
    
    // wait for all tasks to finish 
    std::unique_lock<std::mutex> job_lock_unique(*this->lock);
    this->job_status->wait(job_lock_unique, [&]{ return jobs_complete == num_total_tasks; });
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
    this->num_threads = num_threads;
    this->lock = new std::mutex();
    //this->task_lock = new std::mutex();
    this->job_status = new std::condition_variable();
    this->task_status = new std::condition_variable();
    this->kill_flag = false;
    thread_pool = new std::thread[num_threads];
    this->job_number = 0;
    this->jobs_complete = 0;
    this->num_total_tasks = 0;
    for (int i = 0; i < num_threads; i++) {
        thread_pool[i] = std::thread(&TaskSystemParallelThreadPoolSleeping::spinning, this);
    }
}

void TaskSystemParallelThreadPoolSleeping::spinning() {
    int task_id;
    while (true) {
        if (kill_flag) {
            //std::cerr << "thread " << std::this_thread::get_id() << " killed!" << std::endl;
            break;
        }
        lock->lock();
        if (job_number >= num_total_tasks) {
            lock->unlock();
            // put thread to sleep
            std::unique_lock<std::mutex> task_lock_unique(*this->lock);
            //std::cerr << "thread " << std::this_thread::get_id() << " PUT TO SLEEP!" << std::endl;
            this->task_status->wait(task_lock_unique, [&]{ return job_number < num_total_tasks; });
            //std::cerr << "thread " << std::this_thread::get_id() << " woke up!" << std::endl;
            continue;
        }
        else {
            //std::cerr << "SKIBIDI TOIELT" << std::endl;
            task_id = job_number;
            job_number++;
        }
        lock->unlock();
        //std::cerr << "SIGMA" << std::endl;
        runnable->runTask(task_id, num_total_tasks);
        lock->lock();
        jobs_complete++;
        if (jobs_complete == num_total_tasks) {
            job_status->notify_all();
        }
        lock->unlock();        
    }
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    //std::cerr << "DESTROY" << std::endl;
    kill_flag = true;

    // one last job for each thread: to die
    this->job_number = 0;
    this->num_total_tasks = 1;
    // wake up the threads so they can die
    this->task_status->notify_all();
    for (int i = 0; i < num_threads; i++) {
        thread_pool[i].join();
    }
    delete lock;
    //delete task_lock;
    delete task_status;
    delete[] thread_pool;
    delete job_status;
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {
    this->lock->lock();
    this->runnable = runnable;
    this->jobs_complete = 0;
    this->num_total_tasks = num_total_tasks;
    this->job_number = 0;
    this->task_status->notify_all();
    this->lock->unlock();
    
    // wait for all tasks to finish 
    std::unique_lock<std::mutex> job_lock_unique(*this->lock);
    this->job_status->wait(job_lock_unique, [&]{ return jobs_complete == num_total_tasks; });
    //std::cerr << "DONE" << std::endl;
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
