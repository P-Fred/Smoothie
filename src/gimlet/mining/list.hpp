#pragma once

#include <vector>

namespace gimlet {
  template <typename Value> class NodeList {
    struct Node {
      Node *prev_, *next_;
      Value v_;
      void insert() {
	next_->prev_ = this;
	prev_->next_ = this;
      }
      Node() : v_() {}
      Node(const Value& v) : v_(v) {}
    };
    Node list_;
    std::vector<Node> nodes_;
    size_t size_;
    
    struct Iterator {
      Node* n_;
      NodeList* l_;
      Iterator() = default;
      Iterator(Node* n, NodeList* l) : n_(n), l_(l) {}
      Iterator(const Iterator&) = default;
      Iterator& operator++() { 
	n_ = n_->next_; 
	return *this;
      }
      bool empty() const { return n_ == nullptr; }
      Value& operator*() { return n_->v_; }
      const Value& operator*() const { return n_->v_; }
      Value* operator->() { return &(n_->v_); }
      bool operator!=(const Iterator& other) const { return n_ != other.n_; }
      Iterator& remove() {
	n_->next_->prev_ = n_->prev_;
	n_->prev_->next_ = n_->next_;
	--l_->size_;
	return *this;
      }
      void insert() {
	n_->insert();
	++l_->size_;
      }
    };

  public:
    using value_type = Value;
    using iterator = Iterator;

    NodeList(unsigned int n = 0) : nodes_(), size_(0) { 
      nodes_.reserve(n);
      list_.next_ = list_.prev_ = &list_; 
    }
    void push_back(const value_type& v) {
      nodes_.push_back(Node(v));
      ++size_;
    }
    void build() {
      auto it = nodes_.begin(), end = nodes_.end();
      if(it != end) {
	Node* prev = &list_;
	Node* current = nullptr;
	for(; it != end; ++it) {
	  current = &(*it);
	  prev->next_ = current;
	  current->prev_ = prev;
	  prev = current;
	}
	current->next_ = &list_;
	list_.prev_ = current;
      }
    }

    size_t size() const { return size_; }
    Iterator begin() { return Iterator(list_.next_, this); }
    Iterator end() { return Iterator(&list_, this); }
  };
}
