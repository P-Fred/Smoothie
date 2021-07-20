#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <tuple>

#include <gimlet/timer.hpp>

#include "data_processors.hpp"
#include "list.hpp"

namespace gimlet {
  namespace itemsets {
    using namespace std::string_literals;
   
    template<typename Columns, typename Processor>
    struct VerticalMiner {
      using columns_t = Columns;
      using processor_t = Processor;
      using statistics_t = Statistics;
      using column_t = typename columns_t::column_t;
      using field_t = typename columns_t::field_t;
      using varset_type = std::vector<field_t>;
      using state_t = typename processor_t::state_t;
      using field_iterator_t = typename gimlet::NodeList<field_t>::iterator;
      
    protected:
      columns_t columns_;
      Processor& processor_;
      gimlet::NodeList<field_t> variables_;
      statistics_t& stats_;

      void selectVariables() {
	for(auto col = columns_.begin(), end = columns_.end(); col != end; ++col) {
	  variables_.push_back(col.index());
	}
    	variables_.build();
      }
      
      VerticalMiner(std::string inputFileName, Processor& processor) : columns_(), processor_(processor), variables_(), stats_(processor.statistics()) {
	std::istream* is = &std::cin;
	std::ifstream inputFile;
	if(! inputFileName.empty()) {
	  inputFile.open(inputFileName);
	  is = &inputFile;
	}
	columns_.load(*is);
	processor.preprocess(columns_);
	selectVariables();
	
#ifdef DEBUG
	std::clog << columns_ << std::endl;
#endif
      }
    }; 
   
    template<typename Columns, typename Processor>
    struct MonotonicMiner : public VerticalMiner<Columns, Processor> {
      using VerticalMiner<Columns, Processor>::processor_t;
      using column_t = VerticalMiner<Columns, Processor>::column_t;
      using field_iterator_t = VerticalMiner<Columns, Processor>::field_iterator_t;
      using varset_type = Processor::varset_type;
      using state_t = Processor::state_t;
      
    private:
      using VerticalMiner<Columns, Processor>::variables_;
      using VerticalMiner<Columns, Processor>::columns_;
      using VerticalMiner<Columns, Processor>::processor_;
      using VerticalMiner<Columns, Processor>::stats_;
      
    public:
      struct Extension {
	column_t col_;
	field_iterator_t field_;
	state_t state_;
	
	Extension(column_t col) : col_(std::move(col)), field_() {}	
	Extension(column_t col, field_iterator_t field) : col_(std::move(col)), field_(field) {}
	Extension(Extension&&) = default;
      };
      
      Extension extension(const column_t& current) const {
	Extension ext{current};
	return ext;
      }
      
      Extension intersect(const column_t& current, const field_iterator_t& field) const {
	Extension inter{current, field};
	inter.col_.intersect(columns_[*field]);
	return inter;
      }
      
      using extension_t = Extension;      

    private:
      varset_type pattern_;
      
      void mine(const Extension& current) {
	const field_iterator_t& field = current.field_;
	if(! field.empty())
	  pattern_.push_back(*field);
	processor_.push(pattern_, current.state_);
	
 	++stats_.patternNumber_;
	
	std::vector<field_iterator_t> removed;	
	bool accept;
	
	for(field_iterator_t field = variables_.begin(), end = variables_.end(); field != end; ++field) {
	  extension_t ext = intersect(current.col_, field);
	  std::tie(ext.state_, accept) = processor_.compute_state(ext.col_);
	  removed.push_back(field.remove());	  
	  if(accept) mine(ext);
	}
	
	while(! removed.empty()) {
	  removed.back().insert();
	  removed.pop_back();
	}
	
	if(! field.empty())
	  pattern_.pop_back();	
      }
      
    public:

      void mine() {
	cool::Timer timer;
	bool accept;
	
	timer.start();
	
	extension_t ext = extension(columns_.top());	
	std::tie(ext.state_, accept) = processor_.compute_state(ext.col_);
	if(accept) mine(ext);
	
	// columns_[target] = std::move(targetCol);
	stats_.totalTime_ = timer.stop();
	stats_.write();
      }

      MonotonicMiner(std::string inputFileName, Processor& processor) : VerticalMiner<Columns, Processor>(inputFileName, processor) {}      
    };

    
    template<typename Columns, typename Processor>
    struct BranchAndBoundMiner : public VerticalMiner<Columns, Processor> {
      using VerticalMiner<Columns, Processor>::processor_t;
      using column_t = VerticalMiner<Columns, Processor>::column_t;
      using field_iterator_t = VerticalMiner<Columns, Processor>::field_iterator_t;
      using varset_type = Processor::varset_type;
      using state_t = Processor::state_t;
      
    private:
      using VerticalMiner<Columns, Processor>::variables_;
      using VerticalMiner<Columns, Processor>::columns_;
      using VerticalMiner<Columns, Processor>::processor_;
      using VerticalMiner<Columns, Processor>::stats_;
      
    public:
      struct Extension {
	column_t col_;
	field_iterator_t field_;
	state_t state_;
	
	Extension(column_t col) : col_(std::move(col)), field_() {}	
	Extension(column_t col, field_iterator_t field) : col_(std::move(col)), field_(field) {}
	Extension(Extension&&) = default;
      };
      
      Extension extension(const column_t& current) const {
	Extension ext{current};
	return ext;
      }
      
      Extension intersect(const column_t& current, const field_iterator_t& field) const {
	Extension inter{current, field};
	inter.col_.intersect(columns_[*field]);
	return inter;
      }
      
      using extension_t = Extension;      

    private:
      using extension_set_t = std::vector<Extension>;
      varset_type pattern_;
      bool opus_;
      
      void mine(const Extension& current) {
	const field_iterator_t& field = current.field_;
	if(! field.empty())
	  pattern_.push_back(*field);
	processor_.push(pattern_, current.state_);
#ifdef DEBUG
	std::clog << std::setprecision(3) << "Processing (" << itemset(pattern_.fields()) << ") = " << current << std::endl;
#endif	
	
 	++stats_.patternNumber_;
	
	std::vector<field_iterator_t> removed;	
	std::vector<extension_t> extensions;
	bool accept;
	
	for(field_iterator_t field = variables_.begin(), end = variables_.end(); field != end; ++field) {
	  extension_t ext = intersect(current.col_, field);
#ifdef DEBUG
	  std::clog << "Variable " << ext.field_ << " ";
#endif
	  std::tie(ext.state_, accept) = processor_.compute_state(ext.col_);
	  
	  if(accept)
	    extensions.push_back(std::move(ext));
	  else
	    removed.push_back(field.remove());
	}

	std::vector<extension_t*> extensionPtrs;
	for(extension_t& ext : extensions)
	  extensionPtrs.push_back(&ext);
	  
	std::sort(extensionPtrs.begin(), extensionPtrs.end(), [this] (const Extension* e1, const Extension* e2) -> bool { return processor_.worse(e2->state_, e1->state_); });

	if(opus_) {
	  for(auto it = extensionPtrs.rbegin(), end = extensionPtrs.rend(); it != end; ++it)
	    (*it)->field_.remove();
	  
	  for(extension_t* ext : extensionPtrs) {
	    field_iterator_t& field = ext->field_;
	    if(processor_.accept(ext->state_)) mine(*ext);
	    field.insert();
	  }
	  
	} else {
	  for(extension_t* ext : extensionPtrs) {
	    field_iterator_t& field = ext->field_;
	    removed.push_back(field.remove());
	    if(processor_.accept(ext->state_)) mine(*ext);
	  }
	}
	
	while(! removed.empty()) {
	  removed.back().insert();
	  removed.pop_back();
	}
	
	if(! field.empty())
	  pattern_.pop_back();	
      }
      
    public:

      void mine() {
	cool::Timer timer;
	bool accept;
	
	timer.start();
	extension_t ext = extension(columns_.top());	
	std::tie(ext.state_, accept) = processor_.compute_state(ext.col_);
	if(accept) mine(ext);	
	stats_.totalTime_ = timer.stop();
	stats_.write();
      }

      BranchAndBoundMiner(std::string inputFileName, Processor& processor, bool opus=false) : VerticalMiner<Columns, Processor>(inputFileName, processor), opus_(opus) {}
    };


    template<typename Columns, typename Processor>
    struct BranchTopMiner : public VerticalMiner<Columns, Processor> {
      using VerticalMiner<Columns, Processor>::processor_t;
      using column_t = VerticalMiner<Columns, Processor>::column_t;
      using field_iterator_t = VerticalMiner<Columns, Processor>::field_iterator_t;
      using varset_type = Processor::varset_type;
      using state_t = Processor::state_t;
      
    private:
      using VerticalMiner<Columns, Processor>::variables_;
      using VerticalMiner<Columns, Processor>::columns_;
      using VerticalMiner<Columns, Processor>::processor_;
      using VerticalMiner<Columns, Processor>::stats_;
      
    public:
      struct Extension {
	column_t col_;
	field_iterator_t field_;
	state_t state_;
	
	Extension(column_t col) : col_(std::move(col)), field_() {}	
	Extension(column_t col, field_iterator_t field) : col_(std::move(col)), field_(field) {}
	Extension(Extension&&) = default;
      };
      
      Extension extension(const column_t& current) const {
	Extension ext{current};
	return ext;
      }
      
      Extension intersect(const column_t& current, const field_iterator_t& field) const {
	Extension inter{current, field};
	inter.col_.intersect(columns_[*field]);
	return inter;
      }
      
      using extension_t = Extension;      

    private:
      using extension_set_t = std::vector<Extension>;
      varset_type pattern_;
      bool accept_score_decrease_, opus_;
      
      state_t mine(const Extension& current, state_t best_state_from_ancestors) {
	if(processor_.worse(best_state_from_ancestors, current.state_))
	  best_state_from_ancestors = current.state_;

	const field_iterator_t& field = current.field_;
	if(! field.empty())
	  pattern_.push_back(*field);
#ifdef DEBUG
	std::clog << std::setprecision(3) << "Processing (" << itemset(pattern_.fields()) << ") = " << current << std::endl;
#endif	
	
 	++stats_.patternNumber_;
	
	std::vector<field_iterator_t> removed;	
	std::vector<extension_t> extensions;
	bool accept;
	
	for(field_iterator_t field = variables_.begin(), end = variables_.end(); field != end; ++field) {
	  extension_t ext = intersect(current.col_, field);
#ifdef DEBUG
	  std::clog << "Variable " << ext.field_ << " ";
#endif
	  std::tie(ext.state_, accept) = processor_.compute_state(ext.col_);
	  
	  if(accept && (accept_score_decrease_ || processor_.worse_or_equal(current.state_, ext.state_)))
	    extensions.push_back(std::move(ext));
	  else
	    removed.push_back(field.remove());
	}

	std::vector<extension_t*> extensionPtrs;
	for(extension_t& ext : extensions)
	  extensionPtrs.push_back(&ext);
	  
	std::sort(extensionPtrs.begin(), extensionPtrs.end(), [this] (const Extension* e1, const Extension* e2) -> bool { return processor_.worse(e2->state_, e1->state_); });

	state_t best_state_from_offspring = current.state_;
	if(opus_) {
	  for(auto it = extensionPtrs.rbegin(), end = extensionPtrs.rend(); it != end; ++it)
	    (*it)->field_.remove();
	  
	  for(extension_t* ext : extensionPtrs) {
	    field_iterator_t& field = ext->field_;
	    if(processor_.accept(ext->state_)) {
	      state_t best_state = mine(*ext, best_state_from_ancestors);
	      if(processor_.worse(best_state_from_offspring, best_state))
		best_state_from_offspring = best_state;
	    }
	    field.insert();
	  }
	  
	} else {
	  for(extension_t* ext : extensionPtrs) {
	    field_iterator_t& field = ext->field_;
	    removed.push_back(field.remove());
	    if(processor_.accept(ext->state_)) {
	      state_t best_state = mine(*ext, best_state_from_ancestors);
	      if(processor_.worse(best_state_from_offspring, best_state))
		best_state_from_offspring = best_state;
	    }
	  }
	}

	if(processor_.worse_or_equal(best_state_from_ancestors, current.state_) && processor_.worse_or_equal(best_state_from_offspring, current.state_))
	  processor_.push(pattern_, current.state_);
	
	while(! removed.empty()) {
	  removed.back().insert();
	  removed.pop_back();
	}

	
	if(! field.empty())
	  pattern_.pop_back();

	return best_state_from_offspring;
      }
      
    public:

      void mine() {
	cool::Timer timer;
	bool accept;
	
	timer.start();
	extension_t ext = extension(columns_.top());	
	std::tie(ext.state_, accept) = processor_.compute_state(ext.col_);
	if(accept) mine(ext, ext.state_);	
	stats_.totalTime_ = timer.stop();
	stats_.write();
      }

      BranchTopMiner(std::string inputFileName, Processor& processor, bool reject_score_decrease=false, bool opus=false) : VerticalMiner<Columns, Processor>(inputFileName, processor), accept_score_decrease_(! reject_score_decrease), opus_(opus) {}
    };    
  }
}
