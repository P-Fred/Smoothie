/*
 *   Copyright (C) 2018,  CentraleSupelec
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

#pragma once

#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include <utility>
#include <map>
#include <type_traits>
#include <boost/pool/object_pool.hpp>
#include "gimlet/thread_pool.hpp"
#include <gimlet/topk_queue.hpp>

#include <gimlet/itemsets.hpp>
#include <gimlet/mining/data_partition_scores.hpp>


namespace gimlet {	
  namespace itemsets {
    
    // template<typename T1, typename T2>
    // std::ostream& operator<<(std::ostream& os, const std::pair<T1,T2>& pair) {
    //   os << '(' << pair.first << ',' << pair.second << ')';
    //   return os;
    // }

    template<typename Item>
    std::enable_if_t<std::is_arithmetic_v<Item>, std::string>
    attr_to_string(const Item& attr) {
      return std::to_string(attr);
    }
    
    template<typename Variable, typename Value>
    std::string attr_to_string(const std::pair<Variable, Value>& attr) {
      return std::string("(") + attr_to_string(attr.first) + "," + attr_to_string(attr.second) + ")";
    }
    
    /*
     * A Fast Pruning Tree as used by the FP-growth algorithm
     */
    class FPTree {
           
      /*
       * Type used to store counts in nodes of an FP-tree
       */
      using token_type = unsigned short;

    public:
      
      /*
       * Type used to store total count on an horizontal node of an FP-Tree
       */
      using count_type = unsigned long;

      using pair_type = std::pair<attribute_type, attribute_value_type>;
      //    private:
      
      using pattern_type = std::vector<pair_type>;
      
      /*
       * A single linked list element
       */
      struct Link {
	Link *next_;

	Link() : next_() {}
	Link(const Link&) = default;	
      };
      
      struct Level;

      /*
       * A node of an FP-tree
       */

      struct Part;
      
      struct Node : Link {
	Node* parent_;
	Node* ancestor_;
	token_type count_;
	Part* part_;
	Level* level_;
	
	
	Node(Node* parent, token_type count);
	Node(const Node&) = default;
	
	bool isLast() const;

	void setCount(token_type count);
      };

      struct Part {
	Level* level_;
	Part* next_;
	Part* heir_;
	count_type count_;

	Part() = default;
	Part(Level* level, Part* next) : level_(level), next_(next), heir_(), count_(0) {}
	Part(const Part&) = default;
      };
      
      /*
       * The entrance point to access "horizontal lists" of nodes in a FP-tree
       */

      struct Group;
      
      struct Level : Link {
	pair_type attr_;
	count_type count_;
	count_type size_;
	Part* part_;
	Group* group_;
	
	Level();
	Level(pair_type attr);
	Level(const Level&) = default;

	template<typename LINK, typename NODE>
	class Iterator {
	  LINK* link_;
	public:
	  Iterator(LINK* link) : link_(link) {}
	  Iterator(const Iterator&) = default;

	  void operator++() {
	    link_ = link_->next_;
	  }

	  bool operator!=(const Iterator& other) const {
	    return link_ != other.link_;
	  }
	  
	  bool operator==(const Iterator& other) const {
	    return link_ == other.link_;
	  }

	  NODE* operator*() {
	    return static_cast<NODE*>(link_);
	  }
	  
	  NODE* operator->() {
	    return static_cast<NODE*>(link_);
	  }

	};
            
	using iterator = Iterator<Link, Node>;
	using const_iterator = Iterator<const Link, const Node>;
	
	iterator begin();
	iterator end();
	
	const_iterator begin() const;
	const_iterator end() const;

	void push_back(Node* n);
	bool empty() const;
      };

      struct Group : std::vector<Level*> {
	std::vector<Part> parts_;
	attribute_type var_;
	double H_;
	long index_;
	count_type size_;
	size_t nParts_;

	Group(Node* root, Level* rootLevel);
	Group(attribute_type var);
	size_t nParts() const { return nParts_; }
	
	void buildParts() {
	  nParts_ = size();
	  parts_.resize(nParts_);
	  size_t i = 0;
	  for(auto& level : *this) {
	    Part* part = &parts_[i++];
	    level->part_ = part;
	    level->group_ = this;
	    part->level_ = level;
	  }
	}
	
	void displayHeirs(std::ostream& out) {
	  out << "H(" << index_ << ") |";
	  for(Part& part : parts_) {
	    Part* ptr = part.heir_;
	    while(ptr != nullptr) {
	      // if(ptr->level_->group_->index_ != index_)
	      // 	throw std::runtime_error("Corrupted memory");
	      out << " " << ptr->count_;
	      ptr = ptr->next_;
	    }
	    out << " |";
	  }
	  out << std::endl;
	}
	
	void displayParts(std::ostream& out) {
	  out << "G(" << index_ << ") |";
	  for(Part& part : parts_) {
	    out << " " << part.count_;
	    out << " |";
	  }
	  out << std::endl;
	}

	void reserveMaxPartNumber() {
	  size_ = 0;
	  for(auto& level : *this) {
	    size_ += level->size_;
	  }
	  parts_.reserve(size_);
	}	

	void computeEntropyFromLevels();

	template<typename Score>
	Score score(Score score) const {
	  score.begin(nParts());
	  for(const Part& part : parts_)
	    score.update(part.count_);
	  score.end();
	  return score;
	}

	
	Part* getPartFromAncestorGroup(Node* node, Group* ancestorGroup) {
	  Node* ancestor = node->ancestor_;
	  if(ancestor->level_->group_->index_+1 < ancestorGroup->index_+1)
	    ancestor = node;
	  while(ancestor->level_->group_->index_ != ancestorGroup->index_) {
	    ancestor = ancestor->parent_;
	  }
	  node->ancestor_ = ancestor;
	  return ancestor->part_;
	}

	template<typename Score = NoScore<Group>>
	Score intersect(const Group& constAncestor, const Score& score = Score()) const {
	  Group& group = const_cast<Group&>(*this);
	  return group.intersect(constAncestor, score);
	}
	
	template<typename Score = NoScore<Group>>
	Score intersect(const Group& constAncestor, Score score = Score()) {
	  Group& ancestor = const_cast<Group&>(constAncestor);
	  if(index_+1 < ancestor.index_+1)
	    return ancestor.intersect(*this, score);

	  nParts_ = size() * ancestor.nParts();
	  parts_.clear();
	  for(Level* level : *this) {
	    auto it = level->begin(), end = level->end();	  
	    for(; it != end; ++it) {
	      Node* node = *it;
	      Part* ancestorPart = getPartFromAncestorGroup(node->parent_, &ancestor);
	      Part* part = ancestorPart->heir_;
	      if(part == nullptr || part->level_ != level) {
		parts_.emplace_back(level, part);
		Part& newPart = parts_.back();
		part = &newPart;
		ancestorPart->heir_ = part;		
	      }
	      part->count_ += node->count_;
	      node->part_ = part;
	    }
	  }

	  score.begin(ancestor.nParts(), this->size());
#ifdef DEBUG_COUNTS
	  std::cerr << "[" << std::endl;
	  bool firstRow = true;
#endif	      
	  for(Part& ancestorPart : ancestor.parts_) {
	    if(ancestorPart.heir_ != nullptr) {
	      score.subbegin();
#ifdef DEBUG_COUNTS
	      if(firstRow) firstRow = false; else std::cerr << "," << std::endl;
	      std::vector<int> nxys;
	      nxys.resize(this->size());
#endif	      
	      Part* part = ancestorPart.heir_;
	      ancestorPart.heir_ = nullptr;
	      while(part != nullptr) {
		score.update(part->count_);
#ifdef DEBUG_COUNTS
		auto val = part->level_->attr_.second;
		if(nxys[val] != 0)
		  std::cerr << " PROBLEM_ALREADY_FILL";
		nxys[val] = part->count_;
#endif		
		part = part->next_;
	      }
#ifdef DEBUG_COUNTS
	      std::cerr << "  [ ";
	      bool first = true;
	      for(auto c : nxys) {
		if(first) first = false; else std::cerr << ", ";
		std::cerr << c;
	      }
	      std::cerr << " ]";
#endif		
	      score.subend();
	    }
	  }
	  score.end();
#ifdef DEBUG_COUNTS
	  std::cerr << "\n]" << std::endl;
#endif		

	  return score;
	}
	
	template<typename Function>
	void apply(Function func) const {
	  for(const Part& part : parts_) func(part.count_);
	}
	
      };

      void skip(Group&);
      static double hyperGeometricProbLog(count_type k, count_type a, count_type b, count_type n);
      double computeInfoBias(const Group& currentGroup) const;
      
      mutable cool::ThreadPool threads_;
      std::map<pair_type, Level> levels_;
      std::map<attribute_type, Group> groups_;
      std::vector<Group*> sortedGroups_;
      
      std::unique_ptr<boost::object_pool<Node>> pool_;
      size_t size_, nbrNodes_;
      Node root_;
      Level rootLevel_;
      Group rootGroup_;
      double targetEntropy_;
      Group* targetGroup_;
      int target_;
      
      Group& group(attribute_type attr);
      Level& level(const pair_type& attr);
      Node* addNode(const pair_type& attr, Node* parent);

      template<typename Processor, typename Scorer>
      class PatternGenerator;
      
      class Iterator;
            
      template<typename Iterator>
      attribute_type record(const Iterator& begin, const Iterator& end) {
	attribute_type maxAttr = 0;
	for(Iterator it = begin; it != end; ++it) {
	  const pair_type& attr = *it;
	  if(maxAttr < attr.first) maxAttr = attr.first;
	  Level& lvl = level(attr);
	  ++lvl.count_;
	}
	return maxAttr;
      }

      void build(std::vector<pattern_type>& data);

    public:
      FPTree(int target, size_t nThreads);
      FPTree(const FPTree&) = delete;
      FPTree(FPTree&&) = default;

      template<typename DataIterator>
      void build(DataIterator begin, DataIterator end) {
	std::vector<pattern_type> data;
	for(auto it = begin; it != end; ++it) data.push_back(*it);
	build(data);
      }
      
      void build(std::istream&);
      size_t size() const;
      size_t nbrNodes() const;
      size_t nVars() const;
      double targetEntropy() const;
      void internalState(std::ostream& os);

      using const_iterator = Iterator;
      const_iterator begin() const;
      const_iterator end() const;

      friend std::ostream& operator<<(std::ostream&, const FPTree::Level&);
      friend std::ostream& operator<<(std::ostream&, const FPTree::Group&);
      friend std::ostream& operator<<(std::ostream&, const FPTree&);     

      /*
       * Generates patterns and their frequencies verifying some anti-monotonic predicate
       * and pass them to a processor
       */
      template<typename Processor, typename Scorer>
      void generate(Processor& processor, const Scorer& scorer);
    };



    
    /*
     * Generator of pattern
     * See the function template generate()
     */
    template<typename Processor, typename Scorer=NoScore<FPTree::Group>>
    class FPTree::PatternGenerator {
      FPTree& tree_;
      Processor& processor_;
      Scorer scorer_;
      const Group* targetGroup_;      
      count_type n_;
      
      void develop(Group* parentGroup, size_t varIndex, const Scorer& previousScorer) {
	Group& group = *tree_.sortedGroups_[varIndex];
	//tree_.internalState(std::clog);

	if(++varIndex != tree_.nVars()) {
	  develop(parentGroup, varIndex, previousScorer);
	  
#ifdef DEBUG_COUNTS
	  std::cerr << "INTERSECT " << itemset(processor_.pattern()) << " " << group.var_ << std::endl;
#endif
	  group.intersect(*parentGroup);

	  processor_.push(group.var_);

#ifdef DEBUG_COUNTS
	  std::cerr << "SCORE " <<  itemset(processor_.pattern()) << std::endl;
#endif	  
	  Scorer newScorer = previousScorer(group);
	  double score, bound;
	  std::tie(score, bound) = static_cast<std::pair<double,double>>(newScorer);
	  processor_.emit(score);
#ifdef DEBUG_COUNTS
	  std::cerr << std::setprecision(3) << "RESULT " << score << " " << bound;
	  if(score > bound) 
	    std::cerr << " PROBLEM" << std::endl;
	  else
	    std::cerr << std::endl;
#endif	  

#ifdef DEBUG
	  std::clog << std::setprecision(3) << "Processing (" << itemset(processor_.pattern()) << ") = (" << score << ", " << bound << ")" << std::endl;
#endif
	  if(processor_.toDevelop(bound))
	    develop(&group, varIndex, newScorer);
	  processor_.pop();
	}
      }

    public:
      PatternGenerator(FPTree& tree, Processor& processor, const Scorer& scorer) :
	tree_(tree),
	processor_(processor),
	scorer_(scorer),
	targetGroup_(tree.targetGroup_), n_(tree_.size()) {
      }
      void generate() {
	scorer_.setTarget(*targetGroup_);
	Group& rootGroup = tree_.rootGroup_;

	Scorer newScorer = scorer_(rootGroup);
	double score, bound;
	std::tie(score, bound) = static_cast<std::pair<double,double>>(newScorer);
	
	processor_.emit(score);
	if(processor_.toDevelop(bound))
	  develop(&rootGroup, 0, newScorer);
      }
    };

    template<typename Processor,typename Scorer>
    void FPTree::generate(Processor& processor, const Scorer& scorer) {
      PatternGenerator<Processor, Scorer> generator{*this, processor, scorer};
      generator.generate();
    }    
  }
}
