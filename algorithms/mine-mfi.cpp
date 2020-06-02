/*
 *   Copyright (C) 2020,  CentraleSupelec
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

#include <boost/program_options.hpp>
#include <iostream>
#include <thread>

#include <gimlet/mining/data_partition_scores.hpp>

#include "IFPGrowth.hpp"

namespace gimlet {
  namespace itemsets {

    class SuzukiInfoTopK {
      using scorer_type = SuzukiInfo<partition_tree_parition_t>;
      using miner_type = IFPGrowth<scorer_type>;
      scorer_type scorer_;
      miner_type miner_;
    public:
      SuzukiInfoTopK() : scorer_(), miner_() {}
  
      void operator()(int target,
		      size_t K,
		      size_t nThreads,
		      const std::string& inputFileName,
		      const std::string& outputFileName,
		      const std::string& statsFileName) {
	miner_(scorer_, target, K, nThreads, inputFileName, outputFileName, statsFileName);
      }
    };
  }
}
  
int main(int argc, char *argv[]) {
  using namespace gimlet::itemsets;
  try {
    
    std::string inputFileName, outputFileName, statsFileName;
    int target;
    size_t K;
    size_t nThreads = std::thread::hardware_concurrency();
    
    {
      namespace po = boost::program_options;
      po::options_description desc("Allowed options");
      desc.add_options()
	("help", "help message")
	("target", po::value<int>(&target)->required(), "target attribute (negative target starts from the end: -1 is the last attribute)")
	("K", po::value<size_t>(&K)->default_value(1), "number K of top-k patterns")
	("threads", po::value<size_t>(&nThreads), "number of threads")
	("input", po::value<std::string>(&inputFileName), "input filename")
	("output", po::value<std::string>(&outputFileName), "output filename")
	("stats", po::value<std::string>(&statsFileName), "statistics filename");

      po::variables_map vm;
      po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
      if(argc == 1 || vm.count("help")) {
	std::cout << desc << "\n";
	return EXIT_FAILURE;
      }
      po::notify(vm);
    }
    SuzukiInfoTopK topKminer;
    topKminer(target, K, nThreads, inputFileName, outputFileName, statsFileName);
    return EXIT_SUCCESS;
  } catch(const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << std::endl;
    return EXIT_FAILURE;
  }
}
