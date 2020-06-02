/*
 *   Copyright (C) 2017,  CentraleSupelec
 *
 *   Author : Frédéric Pennerath
 *
 *   Contributor :
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public
 *   License (GPL) as published by the Free Software Foundation; either
 *   version 3 of the License, or any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *   Contact : frederic.pennerath@centralesupelec.fr
 *
 */

#include "gimlet/thread_pool.hpp"

namespace cool {

  ThreadPool::ThreadPool(size_t nThreads) :
    threads_{}, tasks_{}, mutex_{},
    tasksToDo_{}, idle_{}, end_{false} , running_{0} {
      while(nThreads-- != 0) {      
	threads_.emplace_back([this] () {
	    std::unique_lock<std::mutex> lock(mutex_);

	    while(true) {
	      if(! tasks_.empty()) {
		function task = tasks_.front();
		tasks_.pop_front();

		++running_;
		lock.unlock();
		task();
		lock.lock();
		--running_;

		if(tasks_.empty())
		  idle_.notify_all();

	      } else if(end_) {
		break;
	      } else {
		tasksToDo_.wait(lock);
	      }
	    }
	  });
      }
    }

  ThreadPool::~ThreadPool() {
    end_ = true;
    tasksToDo_.notify_all();
    for(auto& thread : threads_) thread.join();
  }

  void ThreadPool::join() {
    std::unique_lock<std::mutex> lock(mutex_);
    while(! tasks_.empty() || running_ > 0)
      idle_.wait(lock);
  }
}
