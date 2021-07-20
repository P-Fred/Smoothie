#include <boost/program_options.hpp>
#include <iostream>
#include <iomanip>

#include <gimlet/mining/search_algorithms.hpp>
#include <gimlet/mining/data_processors.hpp>
#include <gimlet/mining/scoring_functions.hpp>
#include <gimlet/mining/data_partition.hpp>

#if defined(BOUND1) && (BOUND1 == 0)
#define BOOL_BOUND1 false
#else
#define BOOL_BOUND1 true
#endif

#if defined(BOUND2) && (BOUND2 == 0)
#define BOOL_BOUND2 false
#else
#define BOOL_BOUND2 true
#endif


namespace gimlet {
  namespace itemsets {

    template<typename Scorer>
    void mine(Scorer& scorer, std::string inputFileName, std::string outputFileName, std::string statsFileName, bool opus, size_t K, int target) {
      using scorer_t = Scorer;
      using processor_t = TopKProcessor<scorer_t, Partitions>;
      using miner_t = BranchAndBoundMiner<Partitions, processor_t>;

      auto outputStream = std::ref(std::cout);
      std::ofstream outputFile;
      if(! outputFileName.empty()) {
	outputFile.open(outputFileName, std::ios::out | std::ios::binary);
	outputStream = outputFile;
      }
      
      processor_t processor{K, target, outputStream, scorer};
      miner_t miner{inputFileName, processor, opus};
      if(! statsFileName.empty()) processor.statistics().open(statsFileName);

      miner.mine();
      //   scorer.displayRatio();
      
    }    
  }
}

int main(int argc, char *argv[]) {
  using namespace gimlet;
  using namespace gimlet::itemsets;
  try {
    std::string inputFileName, outputFileName, statsFileName;
    size_t K; bool opus;
    int target;
    {
      namespace po = boost::program_options;
      po::options_description desc("Allowed options");
      desc.add_options()
	("help", "help message")
	("target",  po::value<int>(&target)->required(), "target feature")
	("K",  po::value<size_t>(&K)->default_value(1), "number of top-K patterns")
	("rfi",  "reliable fraction of information")
	("smi",  po::value<double>()->implicit_value(1.), "smoothed mutual information (with alpha coefficent)")
	("opus", po::bool_switch(&opus)->default_value(false), "opus optimization")
	("input", po::value<std::string>(&inputFileName), "input filename")
	("output", po::value<std::string>(&outputFileName), "output filename")
	("stats", po::value<std::string>(&statsFileName), "statistics filename");
      po::positional_options_description extraOptions;
      extraOptions.add("command", 1);

      po::variables_map vm;
      po::store(po::command_line_parser(argc, argv).options(desc).positional(extraOptions).run(), vm);
      po::notify(vm);
      
      if(argc == 1 || vm.count("help")) {
	std::cout << desc << "\n";
	return EXIT_FAILURE;
      }
      
      if(vm.count("rfi")) {
	using scorer_t = ReliableFractionOfInformation<Partition>;
	scorer_t scorer{};
	mine(scorer, inputFileName, outputFileName, statsFileName, opus, K,  target);	
      } else if(vm.count("smi")) {
	using scorer_t = SmoothedInformation<Partition, BOOL_BOUND1, BOOL_BOUND2>;
	double alpha = vm["smi"].as<double>();
	scorer_t scorer{alpha};
	mine(scorer, inputFileName, outputFileName, statsFileName, opus, K,  target);
      } else
	throw std::invalid_argument("No scoring function provided among { rfi, smi }");
    }

  } catch(const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
