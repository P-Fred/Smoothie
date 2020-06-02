#pragma once

#include <cstdio>
#include <iostream>
#include <chrono>

namespace cool {
  using namespace std;
  typedef double time_type;

  class Timer {
    unsigned long timeInMilliSeconds() {
      auto t = std::chrono::steady_clock::now();
      return std::chrono::duration_cast<std::chrono::milliseconds>(t - start_).count();
    }
    double timeInSeconds() {
      return ((double) timeInMilliSeconds()) / 1000.;
    }
    double alarmLength_, running_, paused_, last_;
    std::chrono::time_point<std::chrono::steady_clock> start_;
    bool on_;

    void update() {
      double t = timeInSeconds();
      if(on_) {
	running_ += (t - last_);
      } else {
	paused_ += (t - last_);
      }
      last_ = t;
    }
  public:
    Timer(double alarmLength = 0) : alarmLength_(alarmLength), 
				    running_(0), paused_(0), last_(0.),
				    start_(std::chrono::steady_clock::now()), on_(false) {
    }
    inline double runningLength() {
      if(on_) update();
      return running_;
    }
    inline double pausedLength() {
      if(! on_) update();
      return paused_;
    }
    void start() {
      if(on_) return;
      running_ = 0;
      paused_ = 0;
      on_ = true;
      last_ = timeInSeconds();
    }
    double stop() {
      if(on_) {
	update();
	on_ = false;
      }
      return running_;
    }
    void restart() {
      if(on_) return;
      update();
      on_ = true;
    }

    inline bool top() {
      if(on_ && (alarmLength_ > 0)) {
	update();
	if(running_ >= alarmLength_) {
	  running_ = 0;
	  return true;
	}
      }
      return false;
    }
  };
}
