#pragma once

#include <vector>
#include <type_traits>

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

namespace cool {
  template<typename Container> struct ContainerManager {
    static void reserve(Container& container, size_t n) {}
  };

  template<typename V, typename A> struct ContainerManager<std::vector<V,A>> {
    static void reserve(std::vector<V,A>& container, size_t n) {
      container.reserve(n);
    }
  };

  template<typename Container>
  void reserve(Container& container, size_t n) {
    ContainerManager<Container>::reserve(container, n);
  }


  template<typename T, typename Func, typename Allocator = std::allocator<T>>
  class vector : public std::vector<T, Allocator> {
    T* old_addr_;
    Func func_;

    void notify() {
      T* addr = this->data();
      if(addr != old_addr_) {
	func_(*this, old_addr_);
	old_addr_ = addr;
      }
    }
    
  public:
    using typename std::vector<T, Allocator>::reference;

    vector(Func func) : std::vector<T, Allocator>{}, func_(func) {
      static_assert(std::is_invocable_r_v<void, Func, std::vector<T, Allocator>&, T*>, "Callback function with wrong prototype");
      old_addr_ = this->data();
    }
    vector(const vector&) = default;
    vector(vector&&) = default;
    vector& operator=(const vector& other) = default;
    vector& operator=(vector&& other) = default;
    
    vector& operator=(const std::vector<T, Allocator>& other) {
      std::vector<T, Allocator>::operator=(other);
      return *this;
    }
    vector& operator=(std::vector<T, Allocator>&& other) {
      std::vector<T, Allocator>::operator=(std::move(other));
      return *this;
    }
    
    void push_back(const T& value) {
      std::vector<T, Allocator>::push_back(value);
      notify();
    }
    
    void push_back(T&& value) {
      std::vector<T, Allocator>::push_back(std::move(value));
      notify();
    }
    
    template<class... Args>
    reference emplace_back( Args&&... args ) {
      reference res = std::vector<T, Allocator>::emplace_back(args...);
      notify();
      return res;
    }
    
  };
}
