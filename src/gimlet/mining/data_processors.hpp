#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <tuple>

#include <gimlet/statistics.hpp>
#include <gimlet/topk_queue.hpp>
#include <gimlet/json_parser.hpp>
#include <gimlet/data_iterator.hpp>

#include "list.hpp"

namespace gimlet {
  namespace itemsets {
    using namespace std::string_literals;

    template<typename OutputFormat>
    struct PatternWriter {
      using output_format = OutputFormat;
      using parser_t = JSONParser<flow<output_format>>;
      using stream_t = output_stream_t<parser_t>;

      stream_t outputDataStream_;
      output_stream_iterator_t<stream_t> outputIt_;

      template<typename Pattern, typename Score>
      void output_sorted(Pattern pattern, const Score& score) {
        std::sort(pattern.begin(), pattern.end());
	*outputIt_++ = std::pair{pattern, score};
      }

      PatternWriter() = default;
      PatternWriter(std::ostream& os) : outputDataStream_(os, parser_t{}) {
	outputIt_ = {outputDataStream_};
      }
    };

    struct Statistics : cool::Statistics {
      double totalTime_;
      unsigned int patternNumber_;
      Statistics() : cool::Statistics() {
	addDouble("total time", totalTime_, "s");
	addInteger("pattern number", patternNumber_);
      }
    };   
    
    template<typename Scorer, typename Columns>
    struct ProcessorWithScorer {
      using scorer_t = Scorer;
      using score_t = typename scorer_t::value_t;
      using columns_t = Columns;
      using column_t = typename columns_t::column_t;
      using field_t = typename columns_t::field_t;      
      using varset_type = std::vector<field_t>;
      
      scorer_t scorer_;
      PatternWriter<tuple<list<field_t>, score_t>> writer_;      
      Statistics stats_;

      void preprocess(columns_t& columns) {}

      Statistics& statistics() { return stats_; }
      
      ProcessorWithScorer(const scorer_t& scorer, std::ostream& output) :
	scorer_(scorer), writer_(output), stats_{} {}
    };

    template<typename Scorer, typename Columns>
    struct ProcessorWithTarget : ProcessorWithScorer<Scorer, Columns> {
      using columns_t = ProcessorWithScorer<Scorer, Columns>::columns_t;
      using column_t  = ProcessorWithScorer<Scorer, Columns>::column_t;
      using scorer_t  = ProcessorWithScorer<Scorer, Columns>::scorer_t;
      using ProcessorWithScorer<Scorer, Columns>::scorer_;
      
      int target_;
      column_t target_column_;

      void preprocess(columns_t& columns) {
	if constexpr(scorer_t::has_target) {
	    if(target_ < 0) target_ = columns.size() + target_;
	    if(target_ < 0 || target_ >= static_cast<int>(columns.size()))
	      throw std::invalid_argument("Target index out of bounds");
	    
	    target_column_ = {std::move(columns[target_])};
	    scorer_.setTarget(target_column_);
	  }
      }
      
      ProcessorWithTarget(int target, const scorer_t& scorer, std::ostream& output) :
	ProcessorWithScorer<Scorer, Columns>(scorer, output), target_(target), target_column_() {}
    };
    
    template<typename Scorer, typename Columns>
    struct MonotonicProcessor : ProcessorWithScorer<Scorer, Columns> {
      using columns_t = ProcessorWithScorer<Scorer, Columns>::columns_t;
      using column_t  = ProcessorWithScorer<Scorer, Columns>::column_t;
      using scorer_t  = ProcessorWithScorer<Scorer, Columns>::scorer_t;
      using score_t   = ProcessorWithScorer<Scorer, Columns>::score_t;
      using varset_type  = ProcessorWithScorer<Scorer, Columns>::varset_type;
      using ProcessorWithScorer<Scorer, Columns>::scorer_;
      using ProcessorWithScorer<Scorer, Columns>::writer_;
      
      struct State {
	score_t score_;
      };
      using state_t = State;

      double threshold_;
      double ratio_;
      score_t min_, max_;
      bool relativeThreshold_, relativeOutput_;
      
      score_t absoluteScore(double ratio) const { return min_ + (max_ - min_) * ratio; }
      double relativeScore(score_t score) const { return (score - min_) / (max_ - min_); }
      
      void preprocess(columns_t& columns) {
	ProcessorWithScorer<Scorer, Columns>::preprocess(columns);
	
	{
	  column_t col = columns.top();	  
	  min_ = scorer_(col);

	  for(auto colit = columns.begin(), end = columns.end(); colit != end; ++colit) {
	    col.intersect(*colit);
	  }	  
	  max_ = scorer_(col);
	}
	
	if(relativeThreshold_) {
	  if(threshold_ < 0. || threshold_ > 1.) throw std::runtime_error("Threshold "s + std::to_string(threshold_) + " must be in [0;1]");
	  threshold_ = this->absoluteScore(threshold_);
	}
	
#ifdef DEBUG
	std::clog << "Absolute threshold = " << threshold_ << std::endl;
#endif	
	
	if(relativeOutput_) {
    	  ratio_ = this->absoluteScore(1.);
    	} else {
    	  ratio_ = 1.;
    	}
      }
     
      MonotonicProcessor(double threshold, bool relativeThreshold, bool relativeOutput,
			 std::ostream& output, const scorer_t& scorer) : ProcessorWithScorer<Scorer, Columns>(scorer, output),
	threshold_(threshold), relativeThreshold_(relativeThreshold), relativeOutput_(relativeOutput) {}
      
      bool accept(const state_t& state) const {
	return scorer_t::comparator(state.score_, threshold_);
      }
      
      std::pair<state_t, bool> compute_state(column_t& column) const {
	std::pair<state_t, bool> result;
	state_t& state = result.first;
	bool& accept = result.second;
	
      	state.score_ = scorer_(column);
	accept = this->accept(state);
#ifdef DEBUG
	if (accept) std::clog << " kept "; else std::clog << "  pruned ";
	std::clog << ext.field_ << " -> score: " << state.score_ << " bound: " << state.bound_;
#endif
	return result;
      }

      void push(const varset_type& pattern, const state_t& state) {
	writer_.output_sorted(pattern, state.score_);
      }
      void pop(const state_t&) {}
    };
    
    template<typename Scorer, typename Columns>
    struct TopKProcessor : ProcessorWithTarget<Scorer, Columns> {
      using columns_t = ProcessorWithScorer<Scorer, Columns>::columns_t;
      using column_t  = ProcessorWithScorer<Scorer, Columns>::column_t;
      using scorer_t  = ProcessorWithScorer<Scorer, Columns>::scorer_t;
      using score_t   = ProcessorWithScorer<Scorer, Columns>::score_t;
      using varset_type  = ProcessorWithScorer<Scorer, Columns>::varset_type;
      using ProcessorWithScorer<Scorer, Columns>::scorer_;
      using ProcessorWithScorer<Scorer, Columns>::writer_;
      
      struct State {
	score_t score_;
	score_t bound_;
      };
      using state_t = State;

      struct Entry : std::pair<varset_type, score_t> {
	varset_type& fields() { return this->first; }
	const score_t& score() const { return this->second; }
	bool operator<(const Entry& other) const { return scorer_t::comparator(this->score(), other.score()); }

	Entry(const varset_type& varset, const score_t& score) : std::pair<varset_type, score_t>(varset, score) {}
      };
            
      cool::topk_queue<Entry> queue_;
            
      bool worse(const state_t& s1, const state_t& s2) const {
	return scorer_t::comparator(s1.score_, s2.score_);
      }
      
      TopKProcessor(size_t K, int target, std::ostream& output, const scorer_t& scorer) :
	ProcessorWithTarget<Scorer, Columns>(target, scorer, output), queue_{K} {}
      
      ~TopKProcessor() {
	auto sort_variables = [] (Entry entry) -> Entry {
				varset_type& pattern = entry.first;
				std::sort(pattern.begin(), pattern.end());
				return entry;
			      };
	queue_.purge(writer_.outputIt_, sort_variables);
      }

      bool accept(const state_t& state) const {
	score_t worst_score =	queue_.last().score();
	return (! queue_.full()) || scorer_t::comparator(worst_score, state.bound_);
      }
      
      std::pair<state_t, bool> compute_state(column_t& column) const {
	std::pair<state_t, bool> result;
	state_t& state = result.first;
	bool& accept = result.second;
	
      	std::tie(state.score_, state.bound_) = scorer_(column);

	accept = this->accept(state);
#ifdef DEBUG
	if (accept) std::clog << " kept "; else std::clog << "  pruned ";
	std::clog << ext.field_ << " -> score: " << state.score_ << " bound: " << state.bound_;
#endif
	return result;
      }

      void push(const varset_type& pattern, const state_t& state) {
	queue_.push(Entry{pattern, state.score_});
      }
      void pop(const state_t&) {}
    };

    template<typename Scorer, typename Columns>
    struct RhoProcessor : ProcessorWithTarget<Scorer, Columns> {
      using columns_t = ProcessorWithScorer<Scorer, Columns>::columns_t;
      using column_t  = ProcessorWithScorer<Scorer, Columns>::column_t;
      using scorer_t  = ProcessorWithScorer<Scorer, Columns>::scorer_t;
      using score_t   = ProcessorWithScorer<Scorer, Columns>::score_t;
      using varset_type  = ProcessorWithScorer<Scorer, Columns>::varset_type;
      using ProcessorWithScorer<Scorer, Columns>::scorer_;
      using ProcessorWithScorer<Scorer, Columns>::writer_;

      struct State {
	score_t score_;
	score_t bound_;
      };      
      using state_t = State;

      double rho_;
      mutable std::optional<score_t> score_lower_bound_;
      
      bool worse(const state_t& s1, const state_t& s2) const {
	return scorer_t::comparator(s1.score_, s2.score_);
      }

      bool worse_or_equal(const state_t& s1, const state_t& s2) const {
	return ! scorer_t::comparator(s2.score_, s1.score_);
      }

      RhoProcessor(double rho, int target, std::ostream& output, const scorer_t& scorer) :
	ProcessorWithTarget<Scorer, Columns>(target, scorer, output), rho_(rho), score_lower_bound_{} {}
      
      bool accept(const state_t& state) const {
	if(score_lower_bound_) {
	  score_t& lower_bound = *score_lower_bound_;
	  if(scorer_t::comparator(lower_bound, state.bound_)) {
	    score_t lower_bound_candidate = rho_ * state.score_;
	    if(scorer_t::comparator(lower_bound, lower_bound_candidate))
	      lower_bound = lower_bound_candidate;
	  } else return false;
	} else score_lower_bound_ = rho_ * state.score_;
	return true;
      }
      
      std::pair<state_t, bool> compute_state(column_t& column) const {
	std::pair<state_t, bool> result;
	state_t& state = result.first;
	bool& accept = result.second;
	
      	std::tie(state.score_, state.bound_) = scorer_(column);

	accept = this->accept(state);
#ifdef DEBUG
	if (accept) std::clog << " kept "; else std::clog << "  pruned ";
	std::clog << ext.field_ << " -> score: " << state.score_ << " bound: " << state.bound_;
#endif
	return result;
      }

      void push(const varset_type& pattern, const state_t& state) {
	if(scorer_t::comparator(*score_lower_bound_, state.score_))
	  writer_.output_sorted(pattern, state.score_);
      }
      void pop(const state_t&) {}
    };
  }
}
