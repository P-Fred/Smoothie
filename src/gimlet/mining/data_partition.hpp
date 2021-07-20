#pragma once

#include <stdexcept>
#include <vector>
#include <iostream>
#include <cassert>
#include <unordered_map>

#include <gimlet/vector.hpp>
#include <gimlet/mining/scoring_functions.hpp>


namespace gimlet {
  namespace itemsets {

    using value_field_t = unsigned char;

    class Partition {
    public:
      using size_type = unsigned int;
    private:
      struct Part;
      
      struct Cell {
	Cell* next_;
	Part* part_;
#ifdef _DEBUG
	Cell* base_;
#endif
	Cell();
      };

      struct Part {
	Cell *first_, *last_;
	mutable Part *newPart_;
	size_type n_;

	Part() = default;
	void add(Cell* cell);
	bool empty();
      };

      static Part* addPart(std::vector<Part>& parts);
      
      size_type getIndex(const Cell* cell) const;      
      Cell* getPtr(size_type index) const;
      Cell* translatePtr(const Partition& other, const Cell* cell) const;

      class Rebuilder {
	Partition* partition_;

	template <typename T>
	static T* translate(const T* old_elt, const T* old_base, T* new_base) {
	  if(old_elt)
	    return new_base + (old_elt - old_base);
	  else
	    return nullptr;
	}
      public:
	void rebuildCells(const Cell* oldCellAddr);
	void rebuildParts(const Part* oldPartAddr);
	void rebuildCellsAndParts(const Cell* oldCellAddr, const Part* oldPartAddr);
	
	Rebuilder(Partition& partition);
	Rebuilder(const Rebuilder&) = default;
	Rebuilder& operator=(const Rebuilder&) = default;
	
	void operator()(std::vector<Cell>& cells, Cell* oldAddr);
	void operator()(std::vector<Part>& parts, Part* oldAddr);
      };

      Rebuilder rebuilder_;
      Cell *base_, *end_;
      cool::vector<Cell, Rebuilder> cells_;
      cool::vector<Part, Rebuilder> parts_;
      size_type nEmptyParts_;
      
    public:
      Partition();
      Partition(Partition&& other);
      Partition(const Partition& other);
      Partition& operator=(const Partition&) = default;
      Partition& operator=(Partition&&) = default;

      bool empty() const;
      size_t size() const;
      size_t nParts() const;
      size_t nNonEmptyParts() const;
      size_t nEmptyParts() const;

      template<typename Score = NoScore<Partition>>
      Score intersect(const Partition& other, Score score = Score()) const;

      template<typename Score = NoScore<Partition>>
      Score intersect(const Partition& other, Score score = Score());
      template<typename Score>
      Score score(Score score = Score()) const;

      template<typename Function>
      void apply(Function func) const;

      double entropy() const;

      void printArray(std::ostream& os) const;
      void printList(std::ostream& os) const;
      void printPartSize(std::ostream& os) const;
      friend std::ostream& operator<<(std::ostream& os, const Partition& column);

      struct Mapper : std::unordered_map<value_field_t, Part*> {
	Partition* partition_;

	Mapper(Partition& partition);

	void setPartition(Partition& partition);	
	void addCell(value_field_t value);
      };      
    };

    template<typename Score = NoScore<Partition>>
    Score Partition::intersect(const Partition& other, Score score) const {
      Partition copy = *this;
      return copy.intersect(other, score);
    }    

    template<typename Score = NoScore<Partition>>
    Score Partition::intersect(const Partition& other, Score score) {
      assert(size() == other.size());

      score.begin(this->nParts(), other.nParts());
      
      size_type nParts = this->nParts() * other.nParts();
      std::vector<Part> parts;
      parts.reserve(std::min(size(), parts_.size() * other.parts_.size()));

      for(Part& part : parts_) {
	score.subbegin();
	Cell* cell = part.first_;
	while(cell) {
	  Cell* next = cell->next_;
	  Part* otherPart = other.translatePtr(*this, cell)->part_;
	  Part* newPart = otherPart->newPart_;
	  if(newPart == nullptr) {
	    newPart = addPart(parts);
	    otherPart->newPart_ = newPart;
	  }
	  newPart->add(cell);
	  cell = next;
	}
	  
	for(const Part& part : other.parts_) {
	  if(part.newPart_) {
	    score.update(part.newPart_->n_);
	    part.newPart_ = nullptr;
	  }
	}
	score.subend();
      }
	
      nEmptyParts_ = nParts - parts.size();
      parts_ = std::move(parts);
      score.end();
      return score;
    }

    template<typename Score>
    Score Partition::score(Score score) const {
      score.begin(this->nParts());
      for(const Part& part : parts_)
	score.update(part.n_);
      score.end();
      return score;
    }

    template<typename Function>
    void Partition::apply(Function func) const {
      for(const Part& part : parts_) func(part.n_);
    }


    
    
    // Model of Column Concept
    
    struct Partitions {
      using size_type = Partition::size_type;
      using field_t = unsigned short;
      using column_t = Partition;
      
    private:
      using mapper_t = Partition::Mapper;
      
      struct Rebuilder {
	Partitions* partitions_;
	Rebuilder(Partitions& partitions);
	Rebuilder(const Rebuilder&) = default;
	Rebuilder& operator=(const Rebuilder&) = default;
	
	void operator()(std::vector<Partition>& partitions, Partition* oldAddr);
      };

      Rebuilder rebuilder_;
      Partition top_;
      cool::vector<Partition, Rebuilder> columns_;
      std::vector<mapper_t> mappers_;
      mapper_t topMapper_;
      size_t size_;

      class Iterator : public std::vector<Partition>::iterator {
	using parent_iterator = std::vector<Partition>::iterator;
	parent_iterator end_;
	field_t index_;
	size_type n_;

	void update();
      public:
	Iterator() = default;
	Iterator(const parent_iterator& it, const parent_iterator& end, field_t index);
	Iterator(const Iterator& other);

	Iterator& operator++();
	field_t index();
      };

      template<typename _Pattern> void add(const _Pattern& pattern);
      
    public:
      using iterator = Iterator;
      
      iterator begin();
      iterator end();

      mapper_t& getMapper(field_t field);

      Partition& operator[] (field_t field);
      const Partition& operator[] (field_t field) const;
      Partitions();

      void load(std::istream& is);
      const column_t& top() const;
      size_t size();

      friend std::ostream& operator<<(std::ostream& os, const Partitions& columns);
    };

    template<typename _Pattern>
    void Partitions::add(const _Pattern& pattern) {
      for(const auto& pair : pattern) {
	mapper_t& mapper = getMapper(pair.first);
	mapper.addCell(pair.second);
      }
      topMapper_.addCell(1);
    }
    
  }
}
