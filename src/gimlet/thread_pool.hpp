#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>
#include <deque>
#include <iostream>
#include <chrono>

namespace cool {

  class ThreadPool {
    using function = std::function<void()>;
    std::vector<std::thread> threads_;
    std::deque<function> tasks_;
    std::mutex mutex_;
    std::condition_variable tasksToDo_;
    std::condition_variable idle_;
    bool end_;
    unsigned int running_;
  public:
    ThreadPool(size_t nThreads);
    ~ThreadPool();

    inline size_t size() const { return threads_.size(); }
    
    template<typename Func, typename... Args>
    ThreadPool& operator() (const Func& func, Args... args) {
      {
	std::lock_guard<std::mutex> lock(mutex_);
	if(end_) return *this;
	tasks_.emplace_back(func, args...);
      }
      tasksToDo_.notify_one();

      return *this;
    }

    template<typename Func, typename... Args>
    void emplace_back(const Func& func, Args... args) {
      {
	std::lock_guard<std::mutex> lock(mutex_);
	if(end_) return;
	tasks_.emplace_back(func, args...);
      }
      tasksToDo_.notify_one();
    }
    void join();
  };

  inline double timeInMicroSeconds() {
    static auto start = std::chrono::high_resolution_clock::now();
    auto t = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(t - start).count();
  }
  
  template<typename Iterator, typename TaskFactory>
  void run_parallel(const Iterator& begin, const Iterator& end, TaskFactory factory, size_t sizeLimit) {
    using task_type = decltype(factory(*begin));

    if(begin != end) {
      std::vector<std::thread> threads;
      std::vector<task_type> tasks;
      auto it = begin;
      tasks.push_back(factory(*it));
      
      for(++it; it != end; ++it) {
	task_type task = factory(*it);
	size_t s = task.size();
	if(s > sizeLimit)
	  threads.emplace_back(task);
	else
	  tasks.push_back(std::move(task));
      }
      for(auto& task : tasks)
	task();

      for(auto& thread : threads) thread.join();
      // threads.clear();
      // tasks.clear();
    }
  }
  
  template<typename Iterator, typename TaskFactory>
  void run(const Iterator& begin, const Iterator& end, TaskFactory factory) {
    using task_type = decltype(factory(*begin));

    for(auto it = begin; it != end; ++it) {
      task_type task = factory(*it);
      task();
    }
  }  
}
