#include <gimlet/mining/data_partition.hpp>
#include <gimlet/json_parser.hpp>
#include <gimlet/data_iterator.hpp>

namespace gimlet {
  namespace itemsets {
    
    Partition::Cell::Cell() : next_{}, part_{} {}

    void Partition::Part::add(Cell* cell) {
      if(first_) last_->next_ = cell; else first_ = cell;
      last_ = cell;
      cell->part_ = this;
      cell->next_ = nullptr;
      ++n_;
    }

    bool Partition::Part::empty() { return first_ == nullptr; }

    Partition::Part* Partition::addPart(std::vector<Part>& parts) {
      Part& part = parts.emplace_back();
      return &part;
    }
      
    Partition::size_type Partition::getIndex(const Cell* cell) const { 
      return static_cast<size_type>(cell - base_); 
    }
      
    Partition::Cell* Partition::getPtr(size_type index) const { return base_ + index; } 
    Partition::Cell* Partition::translatePtr(const Partition& other, const Cell* cell) const {
      return getPtr(other.getIndex(cell));
    }
      
    void Partition::Rebuilder::rebuildCells(const Cell* oldCellAddr) {
      std::vector<Cell>& cells = partition_->cells_;
      std::vector<Part>& parts = partition_->parts_;	  
      Cell* newCellAddr = cells.data();
      partition_->base_ = newCellAddr;
      partition_->end_ = partition_->base_ + cells.size();
	  
      for(Cell& cell : cells) {
	cell.next_ = translate(cell.next_, oldCellAddr, newCellAddr);
#ifdef _DEBUG
	cell.base_ = newCellAddr;
#endif
      }

      for(Part& part : parts) {
	part.first_ = translate(part.first_, oldCellAddr, newCellAddr);
	part.last_ = translate(part.last_, oldCellAddr, newCellAddr);
      }
    }
	
    void Partition::Rebuilder::rebuildParts(const Part* oldPartAddr) {
      std::vector<Cell>& cells = partition_->cells_;
      std::vector<Part>& parts = partition_->parts_;	  
      Part* newPartAddr = parts.data();
	  
      for(Cell& cell : cells)
	cell.part_ = translate(cell.part_, oldPartAddr, newPartAddr);
    }

    void Partition::Rebuilder::rebuildCellsAndParts(const Cell* oldCellAddr, const Part* oldPartAddr) {
      std::vector<Cell>& cells = partition_->cells_;
      std::vector<Part>& parts = partition_->parts_;	  
      Cell* newCellAddr = cells.data();
      Part* newPartAddr = parts.data();

      partition_->base_ = newCellAddr;
      partition_->end_ = partition_->base_ + cells.size();
	  
      for(Cell& cell : cells) {
	cell.next_ = translate(cell.next_, oldCellAddr, newCellAddr);
	cell.part_ = translate(cell.part_, oldPartAddr, newPartAddr);
#ifdef _DEBUG
	cell.base_ = newCellAddr;
#endif
      }
      for(Part& part : parts) {
	part.first_ = translate(part.first_, oldCellAddr, newCellAddr);
	part.last_ = translate(part.last_, oldCellAddr, newCellAddr);
      }
    }
	
    Partition::Rebuilder::Rebuilder(Partition& partition) : partition_(&partition) {}
	
    void Partition::Rebuilder::operator()(std::vector<Cell>& cells, Cell* oldAddr) { rebuildCells(oldAddr); }
    void Partition::Rebuilder::operator()(std::vector<Part>& parts, Part* oldAddr) { rebuildParts(oldAddr); }

    Partition::Partition() : rebuilder_(*this), base_(), end_(), cells_(rebuilder_), parts_(rebuilder_), nEmptyParts_(0) {
      size_t defaultCapacity = 128;
      parts_.reserve(defaultCapacity);
      cells_.reserve(defaultCapacity);
      base_ = end_ = cells_.data();
    }
      
    Partition::Partition(Partition&& other) : rebuilder_(*this), base_(other.base_), end_(other.end_), cells_(std::move(other.cells_)), parts_(std::move(other.parts_)), nEmptyParts_(other.nEmptyParts_) {
      other.nEmptyParts_ = 0;
      other.base_ = other.end_ = nullptr;
    }
      
    Partition::Partition(const Partition& other) : rebuilder_(*this), cells_(other.cells_), parts_(other.parts_), nEmptyParts_(other.nEmptyParts_) {
      rebuilder_.rebuildCellsAndParts(other.cells_.data(), other.parts_.data());
    }

    bool Partition::empty() const { return cells_.empty(); }
    size_t Partition::size() const { return cells_.size(); }
    size_t Partition::nParts() const { return parts_.size() + nEmptyParts_; }
    size_t Partition::nNonEmptyParts() const { return parts_.size(); }
    size_t Partition::nEmptyParts() const { return nEmptyParts_; }


    double Partition::entropy() const {
      Entropy res = score(Entropy<Partition>()); 
      return res;
    }
      
    void Partition::printArray(std::ostream& os) const {
      const Cell* cells = cells_.data();
      const Part* parts = parts_.data();
      for(const Cell* cell = cells, *end = cells + cells_.size(); cell != end; ++cell) {
	os << (cell - cells) << ") ";
	const Cell* next = cell->next_;
	if(next) os << (next - cells);
	const Part* part = cell->part_;
	os << '/';
	if(part) os << (part - parts);
	os << std::endl;
      }
    }

    void Partition::printList(std::ostream& os) const {
      for(const Part& part : parts_) {
	bool first = true;
	os << '(';
	for(const Cell* cell = part.first_; cell != nullptr; cell = cell->next_) {
	  if(first) {
	    first = false;
	  } else os << ", ";
#ifdef _DEBUG
	  if(cell->base_) os << static_cast<size_type>(cell - cell->base_); else os << getIndex(cell);
#else
	  os << getIndex(cell);
#endif
	}
	os << ") ";

	if(nEmptyParts_ > 0)
	  os << "()x" << nEmptyParts_;
      }
    }

    void Partition::printPartSize(std::ostream& os) const {
      os << this->nParts() << " parts:";
      size_t total = 0;
      for(const Part& part : parts_) {
	os << " (" << part.n_ << ')';
	total += part.n_;
      }
      if(nEmptyParts_ > 0)
	os << " ()x" << nEmptyParts_;

      assert(total == cells_.size());
    }      

    std::ostream& operator<<(std::ostream& os, const Partition& column) {
      //column.printArray(os); os << std::endl;
      //column.printList(os);
      column.printPartSize(os);
      return os;
    }

    Partition::Mapper::Mapper(Partition& partition) : partition_(&partition) {}

    void Partition::Mapper::setPartition(Partition& partition) { partition_ = &partition; }
	
    void Partition::Mapper::addCell(value_field_t value) {
      auto& column = partition_->cells_;
	  
      Cell& cell = column.emplace_back();
#ifdef _DEBUG
      cell.base_ = column.data();
#endif
      auto it = this->find(value);
      Part* part;
      if(it == this->end()) {
	part = addPart(partition_->parts_);
	(*this)[value] = part;
      } else
	part = it->second;
      part->add(&cell);
    }

    
    Partitions::Rebuilder::Rebuilder(Partitions& partitions) : partitions_(&partitions) {}
	
    void Partitions::Rebuilder::operator()(std::vector<Partition>& partitions, Partition* oldAddr) {
      auto it = partitions.begin();
      for(mapper_t& mapper : partitions_->mappers_) {
	mapper.setPartition(*it);
	++it;
      }
    }

    void Partitions::Iterator::update() {
      while(*this != end_) {
	size_type n = (*this)->size();
	if(n != 0) {
	  if(n_ == 0) n_ = n;
	  else if(n != n_) throw std::runtime_error("Non empty columns should all have the same size");
	  break;
	}
	++(static_cast<parent_iterator &>(*this)); ++index_;
      }
    }
    Partitions::Iterator::Iterator(const parent_iterator& it, const parent_iterator& end, field_t index) : parent_iterator(it), end_(end), 
													   index_(index), n_(0) {
      update();
    }
    Partitions::Iterator::Iterator(const Iterator& other) :  parent_iterator(other), end_(other.end_), index_(other.index_), n_(other.n_) {}

    Partitions::Iterator& Partitions::Iterator::operator++() {
      ++(static_cast<parent_iterator &>(*this)); ++index_;
      update();
      return *this;
    }

    Partitions::field_t Partitions::Iterator::index() { return index_; }

    Partitions::iterator Partitions::begin() { return Iterator(columns_.begin(), columns_.end(), 0); }
    Partitions::iterator Partitions::end() { return Iterator(columns_.end(), columns_.end(), static_cast<field_t>(columns_.size())); }

    Partition::Mapper& Partitions::getMapper(field_t field) {
      while(field >= columns_.size()) {
	column_t& col = columns_.emplace_back();
	mappers_.emplace_back(col);
      }
      return mappers_[field];
    }

    Partition& Partitions::operator[] (field_t field) {
      getMapper(field);
      return columns_[field];
    }

    const Partition& Partitions::operator[] (field_t field) const {
      return columns_[field];
    }

    Partitions::Partitions() : rebuilder_(*this), top_(), columns_(rebuilder_), mappers_(), topMapper_(top_), size_(0) {}

    void Partitions::load(std::istream& is) {
      using pattern_type = std::vector<std::pair<field_t, value_field_t>>;
      auto JSON_parser = gimlet::make_JSON_parser<flow<pattern_type>>();
      auto input_stream = gimlet::make_input_data_stream(is, JSON_parser);
      auto data = gimlet::make_input_data_begin<decltype(input_stream), pattern_type>(input_stream);
      auto end = gimlet::make_input_data_end<decltype(input_stream), pattern_type>(input_stream);

      if(data != end) {
	columns_.reserve(data->size()+1);
	do {
	  add(*data); ++size_; ++data;
	} while(data != end);
      }
    }

    const Partitions::column_t& Partitions::top() const { return top_; }
    size_t Partitions::size() { return columns_.size(); }

    std::ostream& operator<<(std::ostream& os, const Partitions& columns) {
      Partitions::field_t field = 0;
      os << "T) " << columns.top_ << std::endl;
      for(const auto& partition : columns.columns_)
	os << field++ << ") " << partition << std::endl;
      return os;
    }    
  }
}
