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
#include <string>
#include <vector>
#include <deque>
#include <tuple>

#include <gimlet/timer.hpp>
#include <gimlet/statistics.hpp>
#include <gimlet/topk_queue.hpp>
#include <gimlet/bin_parser.hpp>
#include <gimlet/data_iterator.hpp>

#include "list.hpp"

namespace gimlet {
  namespace itemsets {
    using namespace std::string_literals;

    template<typename Columns, typename Scorer>
    struct VerticalMiner {
      using columns_t = Columns;
      using column_t = typename columns_t::column_t;
      using field_t = typename columns_t::field_t;      
      using varset_type = std::vector<field_t>;
      using score_t = typename Scorer::value_t;
      using field_iterator_t = typename gimlet::NodeList<field_t>::iterator;
      
      struct Statistics : cool::Statistics {
    	double totalTime_;
    	unsigned int patternNumber_;
    	Statistics() : cool::Statistics() {
    	  addDouble("total time", totalTime_, "s");
    	  addInteger("pattern number", patternNumber_);
    	}
      };

      using output_format = tuple<list<field_t>, score_t>;
      using parser_t = BIN_typed_flow_t<output_format>;
      using stream_t = output_stream_t<parser_t>;

    protected:
      columns_t columns_;      
      Scorer scorer_;
      gimlet::NodeList<field_t> variables_;
      stream_t outputDataStream_;
      output_stream_iterator_t<stream_t> outputIt_;
      Statistics stats_;
      bool debug_ = false;

      void setOutputStream(std::ostream& output) {
	outputDataStream_ = stream_t{output, parser_t{}};
	outputIt_ = {outputDataStream_};
      }

      void output(const varset_type& pattern, score_t score) {
	++stats_.patternNumber_;
	*outputIt_++ = std::pair{pattern, score};
      }

      void selectVariables() {
	for(auto col = columns_.begin(), end = columns_.end(); col != end; ++col) {
	  variables_.push_back(col.index());
	}
    	variables_.build();
      }
      
      VerticalMiner(std::string inputFileName, const Scorer& scorer) : columns_(), scorer_(scorer), variables_(), stats_() {
	std::istream* is = &std::cin;
	std::ifstream inputFile;
	if(! inputFileName.empty()) {
	  inputFile.open(inputFileName);
	  is = &inputFile;
	}
	columns_.load(*is);
	
	if(debug_) std::clog << columns_ << std::endl;
      }

      
    public:      
      Statistics& statistics() { return stats_; }
    };

    template<typename Columns, typename Scorer>
    struct MonotonicMiner : public VerticalMiner<Columns, Scorer> {
      using column_t = typename VerticalMiner<Columns, Scorer>::column_t;
      using field_t = typename VerticalMiner<Columns, Scorer>::field_t;
      using field_iterator_t = typename VerticalMiner<Columns, Scorer>::field_iterator_t;
      using score_t = typename VerticalMiner<Columns, Scorer>::score_t;
    private:
      using VerticalMiner<Columns, Scorer>::variables_;
      using VerticalMiner<Columns, Scorer>::columns_;
      using VerticalMiner<Columns, Scorer>::scorer_;
      using VerticalMiner<Columns, Scorer>::stats_;

      struct Extension {
	column_t col_;
	score_t score_;
	
	Extension(column_t col) : col_(std::move(col)) {}
	Extension(Extension&&) = default;
      };
      
      Extension intersect(const column_t& current, const column_t& other) const {
	Extension inter{current};
	inter.score_ = inter.col_.intersect(other, scorer_);
	return inter;
      }
      
      using extension_t = Extension;
      using extension_set_t = std::vector<Extension>;
      
      varset_type pattern_;      
      double threshold_;
      double ratio_;
      score_t min_, max_;
      
      score_t absoluteScore(double ratio) const { return min_ + (max_ - min_) * ratio; }
      double relativeScore(score_t score) const { return (score - min_) / (max_ - min_); }

      void mine(const Extension& current) {
	std::vector<field_iterator_t> removed;

	double score = current.score_ / ratio_;
	this->output(pattern_, score);
#ifdef DEBUG
	std::clog << itemset(pattern_) << " = " << score << std::endl;
#endif	
	
	std::vector<extension_t> extensions;
	for(field_iterator_t field = variables_.begin(), end = variables_.end(); field != end; ++field) {
	  extension_t ext = intersect(current.col_, columns_[*field]);

	  if(this->scorer_.accept(ext.score_, threshold_)) {
#ifdef DEBUG
	  std::clog << "Keeping " << *field << " -> score = " << ext.score_ << std::endl;
#endif	
	    extensions.push_back(std::move(ext));
	  } else {
#ifdef DEBUG
	  std::clog << "Pruning " << *field << " -> score = " << ext.score_ << std::endl;
#endif	
	    removed.push_back(field.remove());
	  }
	}
	
	auto extIt = extensions.begin();
	for(auto field = variables_.begin(), end = variables_.end(); field != end; ++field, ++extIt) {
	  pattern_.push_back(*field);
	  removed.push_back(field.remove());
	  mine(*extIt);
	  pattern_.pop_back();
	}
	while(! removed.empty()) {
	  removed.back().insert();
	  removed.pop_back();
	}
      }

    public:
      void mine(double threshold, std::ostream& output, bool relativeThreshold, bool relativeOutput = false) {	
	this->setOutputStream(output);
	stats_.addDouble("threshold", threshold_);

	this->selectVariables();

	{
	  column_t col = columns_.top();	  
	  auto topScore = scorer_(col);
	  for(field_iterator_t field = variables_.begin(), end = variables_.end(); field != end; ++field)
	    col.intersect(columns_[*field], scorer_);
	  auto bottomScore = scorer_(col);
	  min_ = topScore.min();
	  max_ = bottomScore.max();
	}
	
	if(relativeThreshold) {
	  if(threshold < 0 || threshold > 1) throw std::runtime_error("Threshold "s + std::to_string(threshold) + " must be in [0;1]");
	  threshold_ = this->absoluteScore(threshold);
	} else {
	  threshold_ = threshold;
	}
#ifdef DEBUG
	std::clog << "Absolute threshold = " << threshold_ << std::endl;
#endif	

	if(relativeOutput) {
    	  ratio_ = this->absoluteScore(1.);
    	} else {
    	  ratio_ = 1.;
    	}


	
	cool::Timer timer;
	timer.start();

	Extension top{columns_.top()};
	if(this->scorer_.accept(top.score_, threshold_)) mine(top);
	
	stats_.totalTime_ = timer.stop();
	stats_.write();
      }
           
      MonotonicMiner(std::string inputFileName, const Scorer& scorer = Scorer{}) : VerticalMiner<Columns, Scorer>(inputFileName, scorer), pattern_(), ratio_(1.) {}
    };
    

    template<typename Columns, typename Scorer>
    struct TopKMiner : public VerticalMiner<Columns, Scorer> {
      using column_t = typename VerticalMiner<Columns, Scorer>::column_t;
      using field_t = typename VerticalMiner<Columns, Scorer>::field_t;
      using field_iterator_t = typename VerticalMiner<Columns, Scorer>::field_iterator_t;
      using score_t = typename VerticalMiner<Columns, Scorer>::score_t;
    private:
      using VerticalMiner<Columns, Scorer>::variables_;
      using VerticalMiner<Columns, Scorer>::columns_;
      using VerticalMiner<Columns, Scorer>::scorer_;
      using VerticalMiner<Columns, Scorer>::stats_;
      using VerticalMiner<Columns, Scorer>::outputIt_;

      struct Extension {
	column_t col_;
	score_t score_;
	score_t bound_;
	field_iterator_t field_;
	
	Extension(column_t col) : col_(std::move(col)) {}	
	Extension(column_t col, field_iterator_t field) : col_(std::move(col)), score_(), bound_(), field_(field) {}
	Extension(Extension&&) = default;
      };
      
      Extension extension(const column_t& current) const {
	Extension ext{current};
	std::tie(ext.score_, ext.bound_) = scorer_(ext.col_);
	return ext;
      }
      
      Extension intersect(const column_t& current, const field_iterator_t& field) const {
	Extension inter{current, field};
	//	auto score = inter.col_.intersect(columns_[*field], scorer_);
	inter.col_.intersect(columns_[*field]);
	std::tie(inter.score_, inter.bound_) = scorer_(inter.col_);
	return inter;
      }
      
      using extension_t = Extension;
      using extension_set_t = std::vector<Extension>;

      struct Entry : std::pair<varset_type, score_t> {
	void setScore(const score_t& score) { this->second = score; }
	varset_type& fields() { return this->first; }
	const score_t& score() const { return this->second; }
	bool operator<(const Entry& other) const { return Scorer::comparator(this->score(), other.score()); }
      };

      struct SortQueuePattern {
	Entry operator()(Entry entry) {
      	  varset_type& pattern = entry.first;
	  std::sort(pattern.begin(), pattern.end());
	  return entry;
	}
      };
      
      cool::topk_queue<Entry> queue_;      
      Entry pattern_;

      void mine(const Extension& current) {
	pattern_.setScore(current.score_);
	queue_.push(pattern_);
 	++stats_.patternNumber_;
#ifdef DEBUG
	std::clog << std::setprecision(3) << "Processing (" << itemset(pattern_.fields()) << ") = " << pattern_.score() << std::endl;
#endif	
	std::vector<field_iterator_t> removed;	
	std::vector<extension_t> extensions;

	for(field_iterator_t field = variables_.begin(), end = variables_.end(); field != end; ++field) {
	  extension_t ext = intersect(current.col_, field);
	  if((!queue_.full()) || Scorer::comparator(queue_.last().score(), ext.bound_)) {
#ifdef DEBUG
	    std::clog << "  Keeping " << *field << " -> score = " << ext.score_ << " bound " << ext.bound_ << " better than top-k " << queue_.last().score() << std::endl;
#endif
	    extensions.push_back(std::move(ext));
	  } else {
#ifdef DEBUG
	    std::clog << "  Pruning " << *field << " -> score = " << ext.score_ << " bound " << ext.bound_ << " worse than top-k " << queue_.last().score() << std::endl;
#endif
	    removed.push_back(field.remove());
	  }
	}

	std::vector<extension_t*> extensionPtrs;
	for(extension_t& ext : extensions)
	  extensionPtrs.push_back(&ext);
	  
	std::sort(extensionPtrs.begin(), extensionPtrs.end(), [] (const Extension* e1, const Extension* e2) -> bool { return Scorer::comparator(e2->score_, e1->score_); });

	for(extension_t* ext : extensionPtrs) {
	  field_iterator_t& field = ext->field_;
	  removed.push_back(field.remove());
	  if((!queue_.full()) || Scorer::comparator(queue_.last().score(), ext->bound_)) {
	    pattern_.fields().push_back(*field);
	    mine(*ext);
	    pattern_.fields().pop_back();
	  }
	}
	
	while(! removed.empty()) {
	  removed.back().insert();
	  removed.pop_back();
	}
      }
      
    public:
      
      void mine(size_t K, int target, std::ostream& output) {
	queue_.set_maxsize(K);
	this->setOutputStream(output);
	//	stats_.addDouble("K", &K);

       	if(target < 0) target = columns_.size() - 1;
	column_t targetCol{std::move(columns_[target])};
	scorer_.setTarget(targetCol);
	
	this->selectVariables();

	cool::Timer timer;
	timer.start();

	mine(extension(columns_.top()));
	queue_.purge(outputIt_, SortQueuePattern{});

	// columns_[target] = std::move(targetCol);
	stats_.totalTime_ = timer.stop();
	stats_.write();
      }

      TopKMiner(std::string inputFileName, const Scorer& scorer = Scorer{}) : VerticalMiner<Columns, Scorer>(inputFileName, scorer), queue_(0) {}
      
    };
  }
}
