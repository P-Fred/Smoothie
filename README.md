
# Smoothie

Project `Smoothie` is the companion code to the article

> Pennerath Frederic, Mandros Panagiotis et Jilles Vreeken. *Discovering Approximate Functional Dependencies
using Smoothed Mutual Information.* To appear in SIGKDD'20 proceedings

It contains several optimized and exact data-mining algorithms to extract from data the top-K (approximate) functional dependencies "X -> Y" between a set X of features and a given target variable Y.

To do so we propose to use a new scoring function for dependencies, called smoothed mutual information (SMI), parameterized by pseudocount number alpha (value 1 is a good default value).
For sake of comparison we also implemented other scoring functions of the state of the art.
Implementations are based on principles detailed in (Pennerath, 2018) and are directly derived from the associated C++ source code downloadable from `github.com/P-Fred/hfp-growth`

In summary this project proposes 4 different binaries, one for each considered scoring function:
- Binary `mine-smi` scores dependencies with smoothed mutual information (SMI), which is our score. See (Pennerath, 2020)
- Binary `mine-rfi` scores dependencies with reliable fraction of information (RFI). See (Mandros, 2017)
- Binary `mine-afi` scores dependencies with adjusted fraction of information (AFI). See (Nguyen, 2014)
- Binary `mine-mfi` scores dependencies with a MDL based fraction of information (MFI). See (Suzuki, 2019)

## License

All software are released under the GNU Public License V3.

## Requirements

The project supports GNU C++ compiler (g++) under Linux distribution (tested on Ubuntu 18.04).
Please note the following requirements prior to installation:
- g++7.0 or higher since some C++ source files use the lastest C++17 features (default on Ubuntu 18.04).
- Boost since some projects like filesystem or program_options are used.
- A recent version of CMake in order to generate the makefile

Below is a script to install the required packages on an Ubuntu system:

```
sudo apt update
sudo apt install -y g++
sudo apt install -y cmake
sudo apt install -y libboost-all-dev
```

## Installation 

After having installed the required packages, compile and install the software as follows.

```
tar xvfz smoothed-info.tar.gz
cd smoothed-info
mkdir build
cd build
cmake ..
make
sudo make install # Only if you want to install binaries on your system
cd algorithms
ls
```
You should find all binaries in the `algorithms` subdirectory.

## Usage

Every program provides help to choose the right command line flags. Some datasets ready-to-be-used are provided in the right JSON format in the `data` subdirectory.
Here are examples showing for each binary how to mine the top-10 dependencies where the target attribute is the last one:
```
cd build/algorithms

./mine-smi --K 10 --target -1 --alpha 1 < ../../data/lymphography.json  # alpha is the Laplace smoothing coefficient
./mine-rfi --K 10 --target -1 < ../../data/lymphography.json 
./mine-afi --K 10 --target -1 --alpha 0.95 < ../../data/lymphography.json # alpha is the significance level. See (Nguyen et al, 2014)
./mine-mfi --K 10 --target -1 < ../../data/lymphography.json 
```

Note that applications take as inputs categorical data formatted in JSON. Every data must be a list of pairs of integers. The first integer encodes the feature number and starts at 0. The second integer is the value of the feature. Outputs are displayed in the same JSON format. A single output is a pair of a pattern defined by a list of feature numbers and of a score.

## References

- Mandros Panagiotis, Mario Boley, et Jilles Vreeken. *Discovering Reliable Approximate Functional Dependencies*. In Proceedings of the 23rd ACM SIGKDD International Conference on  Knowledge Discovery and Data Mining, 355‑63. Halifax, NS, Canada: ACM, 2017.
- Nguyen, Xuan Vinh, Jeffrey Chan, et James Bailey. *Reconsidering Mutual Information Based Feature Selection: A Statistical Significance View.* In AAAI, 2092--2098, 2014.
- Pennerath Frederic. *An Efficient Algorithm for Computing Entropic Measures of Feature Subsets.* In Machine Learning and Knowledge Discovery in Databases, Michele Berlingerio, Francesco Bonchi, Thomas- Pennerath Frederic, Mandros Panagiotis et Jilles Vreeken. *Discovering Approximate Functional Dependencies using Smoothed Mutual Information.* To appear in SIGKDD'20 proceedings
 Gärtner, Neil Hurley, et Georgiana Ifrim, 483‑99. Lecture Notes in Computer Science. Springer International Publishing, 2018.
- Suzuki, Joe. Mutual *Information Estimation: Independence Detection and Consistency.* In 2019 IEEE International Symposium on Information Theory (ISIT), 2514‑18, 2019. 
