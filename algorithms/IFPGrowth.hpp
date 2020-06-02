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
#include <fstream>
#include <limits>
#include <algorithm>

#include "gimlet/timer.hpp"
#include "gimlet/statistics.hpp"

#include <gimlet/json_parser.hpp>
#include <gimlet/data_iterator.hpp>

#include "FPTree.hpp"

namespace gimlet {
  namespace itemsets {
    using partition_tree_parition_t = FPTree::Group;

    template<typename Scorer>
    class IFPGrowth {
      using scorer_t = Scorer;
      using score_t = typename Scorer::value_t;
      
      struct Stats : cool::Statistics {
	unsigned int target_;
	unsigned int nPatterns_;
	double totalTime_;

	Stats() : Statistics() {
	  addInteger("target", target_);
	  addDouble("total time", totalTime_, "s");
	  addInteger("patterns", nPatterns_);
	}
      };
      
      Stats stats_;
      
      class PatternProcessor {
	using pattern_type = std::vector<attribute_type>;
	using output_format = tuple<list<attribute_type>, double>;
	using parser_t = JSONParser<flow<output_format>>;
	using stream_t = output_stream_t<parser_t>;

	struct Entry : std::pair<pattern_type, score_t> {
	  void setScore(const score_t& score) { this->second = score; }
	  pattern_type& fields() { return this->first; }
	  const pattern_type& fields() const { return this->first; }
	  const score_t& score() const { return this->second; }
	  bool operator<(const Entry& other) const { return scorer_t::comparator(this->score(), other.score()); }
	};
	
	stream_t outputDataStream_;
	output_stream_iterator_t<stream_t> outputIt_;
	Entry pattern_;      
	Stats& stats_;	
	cool::topk_queue<Entry> queue_;      

	struct SortQueuePattern {
	  Entry operator()(Entry entry) {
	    varset_type& pattern = entry.first;
	    std::sort(pattern.begin(), pattern.end());
	    return entry;
	  }
	};
	
      public:
	PatternProcessor(size_t K, std::ostream& outputStream, Stats& stats);
	~PatternProcessor();

	bool toDevelop(score_t bound) {
	  return ((!queue_.full()) || scorer_t::comparator(queue_.last().score(), bound));
	}
	
	void emit(score_t score);
	void push(attribute_type var);
	void pop();

	const pattern_type& pattern() const { return pattern_.fields(); }
      };   

    public:
      
      void operator()(
		      scorer_t scorer,
		      int target,
		      size_t K,
		      size_t nThreads,
		      const std::string& inputFileName,
		      const std::string& outputFileName,
		      const std::string& statsFileName);

      IFPGrowth();
    };

    template<typename Scorer>
    IFPGrowth<Scorer>::PatternProcessor::PatternProcessor(size_t K, std::ostream& outputStream, Stats& stats) :
      outputDataStream_{outputStream, parser_t{}},	
      outputIt_{outputDataStream_},
      pattern_(),
      stats_(stats),
      queue_(K) {
      }
    
    template<typename Scorer>
    IFPGrowth<Scorer>::PatternProcessor::~PatternProcessor() {
      queue_.purge(outputIt_, SortQueuePattern{});
    }

    template<typename Scorer>
    void IFPGrowth<Scorer>::PatternProcessor::emit(score_t score) {
	pattern_.setScore(score);
	queue_.push(pattern_);      
	++stats_.nPatterns_;
    }

    template<typename Scorer>
    void IFPGrowth<Scorer>::PatternProcessor::push(attribute_type var) {
      pattern_.fields().push_back(var);
    }

    template<typename Scorer>
    void IFPGrowth<Scorer>::PatternProcessor::pop() {
      pattern_.fields().pop_back();
    }

    template<typename Scorer>
    IFPGrowth<Scorer>::IFPGrowth() : stats_{} {}
    
    template<typename Scorer>
    void IFPGrowth<Scorer>::operator()(
			       scorer_t scorer,
			       int target,
			       size_t K,
			       size_t nThreads,
			       const std::string& inputFileName,
			       const std::string& outputFileName,
			       const std::string& statsFileName
			       ) {
      auto inputStream = std::ref(std::cin);
      std::ifstream inputFile;
      if(! inputFileName.empty()) {
	inputFile.open(inputFileName, std::ios::in | std::ios::binary);
	inputStream = inputFile;
      }
      auto outputStream = std::ref(std::cout);
      std::ofstream outputFile;
      if(! outputFileName.empty()) {
	outputFile.open(outputFileName, std::ios::out | std::ios::binary);
	outputStream = outputFile;
      }
      
      if(! statsFileName.empty())
	stats_.open(statsFileName.c_str(), "%");
      
      cool::Timer timer;
      timer.start();

      FPTree tree{target, nThreads};
      tree.build(inputStream);
      
      //tree.internalState(std::clog);
      
      PatternProcessor processor{K, outputStream, stats_};	
	
      tree.generate(processor, scorer);
      
      stats_.totalTime_ = timer.stop();
      stats_.write();
    }
  }
}
