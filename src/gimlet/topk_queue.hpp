#pragma once

#include <iostream>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <functional>

namespace cool {
  template <typename T>
  struct identity {
    const T& operator()(const T& t) { return t; }
  };
  
  /**
   * topk_queue
   */

  template<typename T, typename Container = std::vector<T>, typename Compare = std::less<typename Container::value_type>>
  struct topk_queue : private Container {
    using Container::empty;
    using Container::size;
    using typename Container::value_type;
    using typename Container::const_reference;

  private:
    size_t K_;

    struct Comparator {
      Compare comp_;
      Comparator(const Compare& comp) : comp_(comp) {}
      
      bool operator()(const value_type& v1, const value_type& v2) const {
	return comp_(v2, v1);
      }
    } comparator_;
          
  public:	
    topk_queue(size_t K, Compare comp = Compare{}) : Container(), K_(K), comparator_(comp) {
    }    
    topk_queue(const topk_queue<T, Container, Compare>&) = default;
    topk_queue(topk_queue<T, Container, Compare>&&) = default;

    bool full() const {
      return this->size() >= K_;
    }
    
    void set_maxsize(size_t K) {
      K_ = K;
      while(this->size() > K) {
	std::pop_heap(this->begin(), this->end(), comparator_);
	this->pop_back();	
      }
    }

    void push(const value_type& value) {
      if((this->size() < K_) || (comparator_(value, this->front()))) {
	  this->push_back(value);
	  std::push_heap(this->begin(), this->end(), comparator_);
	  if(this->size() > K_) {
	    std::pop_heap(this->begin(), this->end(), comparator_);
	    this->pop_back();
	  }
      }
    }

    const value_type& last() const {
      return this->front();
    }

    T identity(T elt) { return elt; }
    
    template<typename OutputIt, typename Transform = cool::identity<T>>
    OutputIt purge(OutputIt outIt, Transform f = Transform{}) {
      std::sort_heap(this->begin(), this->end(), comparator_);
      std::transform(this->begin(), this->end(), outIt, f);
      this->clear();
      return outIt;
    }
  
  };
}
