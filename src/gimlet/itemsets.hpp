#pragma once
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

#include <iostream>
#include <stdexcept>
#include <vector>
#include <list>
#include <iterator>
#include <random>
#include <algorithm>

#include <gimlet/vector.hpp>

namespace gimlet {

  namespace itemsets {
    using item_type = unsigned short;
    using itemset_type = std::vector<item_type>;

    using attribute_type = unsigned short;
    using attribute_value_type = unsigned char;
    using valued_attribute_type = std::tuple<attribute_type, attribute_value_type>;
    using varset_type = std::vector<attribute_type>;
    using valued_varset_type = std::vector<valued_attribute_type>;
    
    using index_type = unsigned int;

    template<typename Pattern>
    typename Pattern::value_type first_free_item(const Pattern& pattern, typename Pattern::value_type item = 0) {
      for(auto it = pattern.end(); it != pattern.begin();) {
	--it;
	if(*it > item) break;
	else if(*it == item)
	  ++item;
      }
      return item;
    }

    template<typename Pattern> class itemset;
    template<typename Pattern>
    std::ostream& operator<<(std::ostream&, const itemset<Pattern>&);
    
    template<typename Pattern>
    class itemset {
      const Pattern& pattern_;
      friend std::ostream& operator<< <Pattern>(std::ostream&, const itemset<Pattern>&);
    public:
      using const_iterator = typename Pattern::const_iterator;
      itemset(const Pattern& pattern) : pattern_(pattern) {}
      const_iterator begin() const { return pattern_.begin(); }
      const_iterator end() const { return pattern_.end(); }
    };
    
    template<typename Pattern>
    std::ostream& operator<<(std::ostream& os, const itemset<Pattern>& pattern) {
      bool first = true;
      for(const auto& item : pattern) {
	if(first)
	  first = false;
	else
	  os << ' ';
	os << int(item);
      }
      return os;
    }

    template<typename Pattern> class valued_varset;
    template<typename Pattern>
    std::ostream& operator<<(std::ostream&, const valued_varset<Pattern>&);

    template<typename Pattern>
    class valued_varset {
      const Pattern& pattern_;
      friend std::ostream& operator<< <Pattern>(std::ostream&, const valued_varset<Pattern>&);
    public:
      using const_iterator = typename Pattern::const_iterator;
      valued_varset(const Pattern& pattern) : pattern_(pattern) {}
      const_iterator begin() const { return pattern_.begin(); }
      const_iterator end() const { return pattern_.end(); }
    };
    
    template<typename Pattern>
    std::ostream& operator<<(std::ostream& os, const valued_varset<Pattern>& pattern) {
      bool first = true;
      for(const auto& pair : pattern) {
	if(first)
	  first = false;
	else
	  os << ' ';
	os << '[' << int(std::get<0>(pair)) << ", " << int(std::get<1>(pair)) << ']';
      }
      return os;
    }
    
    template<typename Itemset>
    class Predecessors {
      const Itemset& itemset_;
    public:
      using size_type = typename Itemset::size_type;
      using value_type = typename Itemset::value_type;
      using itemset_iterator = typename Itemset::iterator;
      using itemset_const_iterator = typename Itemset::const_iterator;
    
      struct Predecessor {
	using value_type = typename Itemset::value_type;
      private:
	const Itemset& itemset_;
	value_type attr_;

      public:	
	struct iterator {
	  using value_type = typename Itemset::value_type;
	  using iterator_category = std::output_iterator_tag;
	  
	private:
	  mutable itemset_const_iterator it_;
	  value_type attr_;
	  mutable bool skipped_;

	  inline void next() {
	    if(! skipped_) if(*it_ == attr_) { skipped_ = true; ++it_; }
	  }
	public:
	  
	  inline iterator(const itemset_const_iterator& it, value_type attr) : it_(it), attr_(attr), skipped_(false) { next(); }
	  inline iterator(const itemset_const_iterator& it) : it_(it) {}
	  inline iterator(const iterator&) = default;

	  inline iterator& operator++() {
	    ++it_;
	    next();
	    return *this;
	  }
	  inline const value_type& operator*() const {
	    return *it_;
	  }
	  inline bool operator==(const iterator& other) const {
	    return it_ == other.it_;
	  }
	  inline bool operator!=(const iterator& other) const {
	    return it_ != other.it_;
	  }
	};
	using const_iterator = iterator;
	
	inline Predecessor(const Itemset& itemset, value_type attr) : itemset_(itemset), attr_(attr) {}	
	Predecessor(const Predecessor&) = default;
      
	inline iterator begin() const { return iterator(itemset_.begin(), attr_); }
	inline iterator end() const { return iterator(itemset_.end()); }	
      };
    
      class iterator {
	const Itemset& itemset_;
	itemset_const_iterator it_;
      public:
	using value_type = Predecessor;
	
	inline iterator(const Itemset& itemset, const itemset_const_iterator& it) : itemset_(itemset), it_(it) {}
	iterator(const iterator&) = default;

	inline iterator& operator++() { ++it_; return *this; }
	Predecessor operator*() const {
	  return Predecessor(itemset_, *it_);
	}
	inline bool operator==(const iterator& other) const {
	  return it_ == other.it_;
	}
	inline bool operator!=(const iterator& other) const {
	  return it_ != other.it_;
	}
      };
    
      inline Predecessors(const Itemset& itemset) : itemset_(itemset) {}

      inline iterator begin() {
	return iterator(itemset_, itemset_.begin());
      }
    
      inline iterator end() {
	return iterator(itemset_, itemset_.end());
      }
    };

    template<typename Pattern>
    Predecessors<Pattern> make_predecessors(const Pattern& pattern) {
      return {pattern};
    }

    

    template<typename Itemset>
    class Ancestors {
      const Itemset& itemset_;
      size_t depth_;
      
    public:
      using size_type = typename Itemset::size_type;
      using item_type = typename Itemset::value_type;
      using value_type = item_type;
      using itemset_iterator = typename Itemset::iterator;
      using itemset_const_iterator = typename Itemset::const_iterator;
    
      struct Ancestor {
	using value_type = typename Itemset::value_type;
      private:
	const Itemset& itemset_;
	const std::vector<item_type>& attrs_;
	using attr_iterator = typename std::vector<item_type>::const_iterator;
	
      public:	
	struct iterator {
	  using value_type = typename Itemset::value_type;
	  using iterator_category = std::output_iterator_tag;
	  
	private:
	  mutable itemset_const_iterator it_;
	  item_type current_;
	  attr_iterator attrIt_, attrEnd_;

	  inline void next() {
	    while(attrIt_ != attrEnd_ && current_ == *attrIt_) {
	      ++it_; ++current_; ++attrIt_;
	    }
	  }
	public:
	  
	  inline iterator(const itemset_const_iterator& it,
			  const  attr_iterator& attrIt,
			  const  attr_iterator& attrEnd
			  ) : it_(it), current_(0),
			      attrIt_(attrIt), attrEnd_(attrEnd) {
	    next();
	  }
	  inline iterator(const itemset_const_iterator& it) : it_(it) {}
	  inline iterator(const iterator&) = default;

	  inline iterator& operator++() {
	    ++it_, ++current_;
	    next();
	    return *this;
	  }
	  inline const value_type& operator*() const {
	    return *it_;
	  }
	  inline bool operator==(const iterator& other) const {
	    return it_ == other.it_;
	  }
	  inline bool operator!=(const iterator& other) const {
	    return it_ != other.it_;
	  }
	};
	using const_iterator = iterator;

	inline Ancestor(const Itemset& itemset, const std::vector<item_type>& attrs) : itemset_(itemset), attrs_(attrs) {}
	
	Ancestor(const Ancestor&) = default;
      
	inline iterator begin() const { return iterator(itemset_.begin(), attrs_.begin(), attrs_.end()); }
	inline iterator end() const { return iterator(itemset_.end()); }	
      };
    
      class iterator {
	const Itemset& itemset_;
	std::vector<item_type> attrs_;
	Ancestor ancestor_;
	size_t depth_;
	
	void next() {
	  if(! attrs_.empty()) {
	    if(attrs_.size() < depth_) {
	      item_type newAttr = attrs_.back() + 1;
	      if(newAttr != itemset_.size()) {
		attrs_.push_back(newAttr);
		return;
	      }
	    }
	    while(true) {
	      item_type& last = attrs_.back();
	      ++last;
	      if(last != itemset_.size()) {
		return;
	      }
	      attrs_.pop_back();
	      if(attrs_.empty()) return;
	    }
	  }
	}
	
      public:
	using value_type = Ancestor;
	
	inline iterator(const Itemset& itemset, size_t depth) : itemset_(itemset), attrs_(), ancestor_(itemset, attrs_), depth_(depth) {
	  if(! itemset.empty()) attrs_.push_back(0);
	}
	inline iterator(const Itemset& itemset) : itemset_(itemset), attrs_(), ancestor_(itemset, attrs_) {}
	  
	iterator(const iterator&) = default;

	inline iterator& operator++() { next(); return *this; }
	
	const Ancestor& operator*() const {
	  return ancestor_;
	}
	inline bool operator==(const iterator& other) const {
	  return attrs_ == other.attrs_;
	}
	inline bool operator!=(const iterator& other) const {
	  return attrs_ != other.attrs_;
	}
      };
    
      inline Ancestors(const Itemset& itemset, size_t depth) : itemset_(itemset), depth_(std::min(itemset.size(), depth)) {}

      inline iterator begin() {
	return iterator(itemset_, depth_);
      }
    
      inline iterator end() {
	return iterator(itemset_);
      }
    };

    template<typename Pattern>
    Ancestors<Pattern> make_ancestors(const Pattern& pattern, size_t depth) {
      return {pattern, depth};
    }
    
  
    template<typename Item>
    class incremental_generator {
      Item n_items_;
      
    public:      
      class iterator {	
	using itemset_t = std::list<Item>;
	using item_type = Item;      
	itemset_t itemset_;
      public:
	using value_type = itemset_t;
	
	iterator(const std::initializer_list<Item>& init) : itemset_(init) {}
	iterator(const iterator&) = default;
	
	inline iterator& operator++() {
	  auto it = itemset_.end();
	  item_type i = 0;
	  
	  while(it != itemset_.begin()) {
	    --it;
	    auto& v = *it;
	    if(i < v) {
	      itemset_.insert(++it, i);
	      return *this;
	    } else if(i == v) {
	      it = itemset_.erase(it);
	      ++i;
	    }
	  }
	  itemset_.push_front(i);
	  return *this;
	}
	
	inline iterator& backtrack(const iterator& end) {
	  auto it = itemset_.end();
	  if(it == itemset_.begin())
	    *this = end;
	  else {
	    --it;
	    item_type i = *it;
	    it = itemset_.erase(it);
	    ++i;
	    while(it != itemset_.begin()) {
	      --it;
	      auto& v = *it;
	      if(i < v) {
		itemset_.insert(++it, i);
		return *this;
	      }
	      it = itemset_.erase(it);
	      ++i;
	    }
	    itemset_.push_front(i);
	  }
	  return *this;
	}
	
	inline const itemset_t& operator*() const {
	  return itemset_;
	}
	
	inline const itemset_t* operator->() const {
	  return &itemset_;
	}
	
	inline bool operator==(const iterator& other) const {
	  return itemset_ == other.itemset_;
	}
      
	inline bool operator!=(const iterator& other) const {
	  return itemset_ != other.itemset_;
	}
      };

      iterator begin() const { return iterator({}); }
      iterator end() const  {
	return iterator({n_items_});
      }
      
      inline incremental_generator(Item n_items) : n_items_(n_items) {}      
    };


    
    template<typename Iterator, typename Condition>
    class BacktrackedRange {

      Iterator begin_, end_;
      Condition cond_;
      
    public:      
      inline BacktrackedRange(const Iterator& begin, const Iterator& end, const Condition& cond) : begin_(begin), end_(end), cond_(cond) {}
      inline BacktrackedRange(const BacktrackedRange&) = default;
      
      class iterator {
	Iterator it_, end_;
	Condition cond_;

	friend class BacktrackedRange;
	inline iterator(const Iterator& it, const Iterator& end, const Condition& cond) : it_(it), end_(end), cond_(cond) { next(); }

	inline void next() {
	  while(it_ != end_) {
	    if(cond_(*it_)) return;
	    it_.backtrack(end_);
	  }
	}
      public:
	using value_type = typename Iterator::value_type;
	
	inline iterator(const iterator&) = default;

	inline iterator& operator++() {
	  ++it_;
	  next();
	  return *this;
	}
	inline const value_type& operator*() const { return *it_; }
	inline const value_type* operator->() const { return &*it_; }
	inline bool operator==(const iterator& other) const { return it_ == other.it_; }
	inline bool operator!=(const iterator& other) const { return it_ != other.it_; }
      };

      iterator begin() const { return iterator(begin_, end_, cond_); }
      iterator end() const { return iterator(end_, end_, cond_); }
    };

    template<typename Iterator, typename Condition>
    BacktrackedRange<Iterator, Condition> backtracked_range(const Iterator& begin, const Iterator& end, const Condition& cond) { return BacktrackedRange<Iterator, Condition>(begin, end, cond); }


    
    template<typename Itemset>
    class random_generator {
      using size_type = typename Itemset::size_type;
      using value_type = typename Itemset::value_type;
      size_type size_;
      value_type n_items_;
      std::mt19937 gen_{std::random_device{}()};
      std::exponential_distribution<> dist_;
      std::vector<double> tmp_;
      Itemset pattern_;
    
    public:
      random_generator(size_type size, value_type n_items) : size_(size + 1), n_items_(n_items), dist_(1.) {
	tmp_.reserve(size_);
	cool::reserve(pattern_, size_);      
      }
    
      random_generator(const random_generator&) = default;
    
      Itemset operator()() {
	tmp_.clear();
	pattern_.clear();
      
	double sum = 0.;
	std::generate_n(std::back_inserter(tmp_),
			size_,
			[this, &sum]() -> double {
			  sum += dist_(gen_);
			  return sum;
			});
	tmp_.pop_back();
      
	std::transform(tmp_.begin(), tmp_.end(),
		       std::back_inserter(pattern_),
		       [this, &sum](double v) -> value_type {
			 return v / sum * n_items_;
		       });
	return pattern_;
      }
    };
  }  
}
  


