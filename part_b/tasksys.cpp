#include "tasksys.h"
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
            job_status->notify_one();
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
    this->job_status = new std::condition_variable();
    this->task_status = new std::condition_variable();
    this->kill_flag = false;
    thread_pool = new std::thread[num_threads];

    this->ready_queue = std::queue<TaskBatch>();
    this->num_jobs_left = std::unordered_map<TaskID, int>();
    this->waiting_tasks = std::vector<WaitingTask>();
    this->next_task_id = 0;
    this->threads_at_work = 0;

    for (int i = 0; i < num_threads; i++) {
        thread_pool[i] = std::thread(&TaskSystemParallelThreadPoolSleeping::spinning, this);
    }
}

void TaskSystemParallelThreadPoolSleeping::spinning() {

    IRunnable* runnable;
    TaskID task_id;
    int num_total_tasks;
    int i;
    
    while (true) {
        std::unique_lock<std::mutex> task_lock_unique(*this->lock);
        this->task_status->wait(task_lock_unique, [&]{ return !ready_queue.empty() || kill_flag; });
        //std::cerr << "BRUH ready_queue.size(): " << ready_queue.size() << std::endl;
        
        if (kill_flag) {
            break;
        }
        
        auto& front_ready_queue = ready_queue.front();
        runnable = front_ready_queue.runnable;
        task_id = front_ready_queue.task_id;
        i = front_ready_queue.i;
        num_total_tasks = front_ready_queue.num_total_tasks;
        int to_process = front_ready_queue.batch_size;
        threads_at_work++;
        ready_queue.pop();
        task_lock_unique.unlock();

        for (int j = i; j < std::min(i + to_process, num_total_tasks); j++) {
            runnable->runTask(j, num_total_tasks);
        }

        task_lock_unique.lock();
        num_jobs_left[task_id]--;
        threads_at_work--;
        
        // if all tasks in this task_id are complete, we need to check if any new 
        // tasks in the waiting_tasks set are ready to be run

        // also check if all jobs are done
        if (num_jobs_left[task_id] == 0) {  
            // check if all jobs are done
            if (waiting_tasks.empty() && ready_queue.empty() && threads_at_work == 0) {
                job_status->notify_one();
            }

            // check if any new tasks in the waiting_tasks set are ready to be run
            for (auto iter = waiting_tasks.begin(); iter != waiting_tasks.end();) {

                auto& front_waiting_tasks = *iter;
                IRunnable* wait_runnable = front_waiting_tasks.runnable;
                TaskID wait_task_id = front_waiting_tasks.task_id;
                int wait_num_total_tasks = front_waiting_tasks.num_total_tasks;
                const std::vector<TaskID>& wait_deps = front_waiting_tasks.deps;

                bool deps_met = true;
                for (size_t j = 0; j < wait_deps.size(); j++) {
                    if (num_jobs_left[wait_deps[j]] > 0) {
                        deps_met = false;
                        break;
                    }
                }
                if (deps_met) {
                    bool was_empty = ready_queue.empty();
                    int batch_size = std::max(1, wait_num_total_tasks/num_threads);
                    int num_batches = 0;
                    for (int j = 0; j < wait_num_total_tasks; j+=batch_size) {
                        TaskBatch task_batch = {wait_runnable, wait_task_id, j, wait_num_total_tasks, batch_size};
                        ready_queue.push(task_batch);
                        num_batches++;
                    }
                    num_jobs_left[wait_task_id] = num_batches;
                    iter = waiting_tasks.erase(iter);
                    if (was_empty) {
                        task_status->notify_all();
                    }
                }
                else {
                    iter++;
                }
            }
        }
        task_lock_unique.unlock();        
    }
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
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
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {
    runAsyncWithDeps(runnable, num_total_tasks, {});
    sync();
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {
    TaskID task_id = next_task_id; 
    next_task_id++;

    lock->lock();
    
    // check if dependencies are already met
    bool deps_met = true;
    for (size_t i = 0; i < deps.size(); i++) {
        if (num_jobs_left[deps[i]] > 0) {
            deps_met = false;
            break;
        }
    }

    int batch_size = std::max(1, num_total_tasks/num_threads);
    if (deps_met){
        bool was_empty = ready_queue.empty();
        // add to ready queue with built in batching 
        int num_batches = 0;
        for (int i = 0; i < num_total_tasks; i+=batch_size) { 
            TaskBatch task_batch = {runnable, task_id, i, num_total_tasks, batch_size};
            ready_queue.push(task_batch);
            num_batches++;
        }
        // mark this task as not complete
        num_jobs_left[task_id] = num_batches;
        // notify all threads
        if (was_empty) {
            task_status->notify_all();
        }
    }
    else {
        // add to waiting tasks 
        num_jobs_left[task_id] = 1; // not zero, the task is not done!
        WaitingTask waiting_task = {runnable, task_id, num_total_tasks, deps};
        waiting_tasks.push_back(waiting_task);
    }
    lock->unlock();
    
    return task_id;    
}

void TaskSystemParallelThreadPoolSleeping::sync() {
    std::unique_lock<std::mutex> job_lock_unique(*this->lock);
    this->job_status->wait(job_lock_unique, [&]{ return ready_queue.empty() && waiting_tasks.empty() && threads_at_work == 0; });
    return;
}
