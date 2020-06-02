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

#pragma once

#include <stdexcept>
#include <iostream>
#include <cmath>
#include <cassert>
#include <algorithm>

#include <gimlet/itemsets.hpp>
#include <gimlet/statistics.hpp>

namespace gimlet {

  namespace stats {
    double chiTestCriticalValue(double p, int df);
  }
  
  namespace itemsets {
   
    // Model of concept score
    template<typename Partition, typename Value>
    struct PartitionScore {
      using size_type = typename Partition::size_type;
      using value_t = Value;
      
      void begin(size_type) {}      
      void begin(size_type, size_type) {}      
      void subbegin() {}
      void update(size_type) {}
      void subend() {}
      void end() {}

      value_t min();
      value_t max();
    };

    template<typename Partition>
    struct NoScore : public PartitionScore<Partition, void> {
      void min() {}
      void max() {}
    };

    template<typename Partition>
    class CountCollecter : public PartitionScore<Partition, std::vector<double>> {
      std::vector<double> counts_;
    public:
      using size_type = typename Partition::size_type;
      using value_t = std::vector<double>;
      
      CountCollecter() = default;
      CountCollecter(const CountCollecter&) = default;
      
      void begin(size_type) {
	counts_.clear();
      }
      
      void begin(size_type, size_type) {
	counts_.clear();
      }
      
      void update(long count) {
	counts_.push_back(count);
      }

      void end() {
      }

      operator value_t () const {
	return counts_;
      }
	
      std::vector<double> operator()(const Partition& partition) const {
	return partition.score(*this);
      }
    };

    
    static constexpr double log2 = std::log(2.);
    
    inline double xlogx(double c) {
      if(c > 0.)
	return c * std::log2(c);
      else
	return 0.;
    }
    
    template<typename Partition>
    class Entropy : public PartitionScore<Partition, double> {
      using size_type = typename Partition::size_type;

      double H_;
      size_type n_;
      size_type nParts_;
      
    public:
      using value_t = double;
      
      bool accept(const value_t& entropy, const value_t& threshold) const {
	return entropy <= threshold;
      }
      
      Entropy() = default;
      Entropy(const Entropy&) = default;

      void begin(size_type nParts) {
	H_ = 0.;
	n_ = 0;
	nParts_ = nParts;
      }
      
      // void begin(size_type nPartsX, size_type nPartsY) {
      // 	H_ = 0.;
      // 	n_ = 0;
      // 	nParts_ = nPartsX * nPartsY;
      // }
      
      void update(long count) {
	H_ += xlogx(count);
	n_ += count;
      }

      void end() {
	H_ = (std::log2(n_) - H_/n_);
	if(H_ < 0) H_ = 0.;
      }
	
      Entropy operator()(const Partition& partition) const {
	return partition.score(*this);
      }

      operator value_t () const {
	return H_;
      }
      double count() { return n_; }
      
      value_t min() { return 0.; }
      value_t max() { return std::log2(nParts_); }
    };
    
    class SEntropy {      
      double alpha_;
      mutable double counts_, sumxlogx_;
      mutable long nParts_, nNonEmptyParts_;

      void update() const {
	long nEmptyParts = nParts_ - nNonEmptyParts_;
	if(nEmptyParts != 0) {
	  sumxlogx_ += nEmptyParts * xlogx(alpha_);
	  nNonEmptyParts_ = nParts_;
	}
      }
      
    public:
      SEntropy() = default;
      SEntropy(double alpha, long nParts) : alpha_(alpha), counts_(0.), sumxlogx_(0.), nParts_(nParts), nNonEmptyParts_(0) {}
      SEntropy(const SEntropy&) = default;
      
      void setNParts(long nParts) { nParts_ = nParts; }
      
      SEntropy& operator+=(double count) {
	counts_ += count;
	count += alpha_;
	sumxlogx_ += xlogx(count);
	++nNonEmptyParts_;
	return *this;
      }
      
      // SEntropy& operator+=(const SEntropy& other) {
      // 	assert(alpha_ == other.alpha_);
      // 	counts_ += other.counts_;
      // 	sumxlogx_ += other.sumxlogx_;
      // 	nParts_ += other.nParts_;
      // 	nNonEmptyParts_ += other.nNonEmptyParts_;
      // 	return *this;
      // }
            
      double alpha() const { return alpha_; }
      long nParts() const { return nParts_; }
      long counts() const { return counts_; }
      double pseudoCounts() const { return counts_ + nParts_ * alpha_; }

      double sumxlogx() const {
	update();
	return sumxlogx_;
      }

      // double info() const {
      // 	update();
      // 	double counts = counts_ + nParts_ * alpha_;
      // 	double H = xlogx(counts) - sumxlogx_;
      // 	if(H < 0.) H = 0.;
      // 	return H;
      // }
      
      operator double() const {
	update();

	double counts = counts_ + nParts_ * alpha_;
	double H = std::log2(counts) - sumxlogx_ / counts;
	if(H < 0.) H = 0.;
	return H;
      }
    };

    class SCondEntropy {
      double alpha_;
      double counts_, sumxlogx_, sumxylogxy_;
      long nXParts_, nYParts_, nNonEmptyXParts_;

      void update() {
	long nEmptyXParts = nXParts_ - nNonEmptyXParts_;
	if(nEmptyXParts != 0) {
	  SEntropy emptyXPart{alpha_, nYParts_};
	  sumxlogx_ += nEmptyXParts * xlogx(nYParts_ * alpha_);
	  sumxylogxy_ += nEmptyXParts * emptyXPart.sumxlogx();
	  nNonEmptyXParts_ = nXParts_;
	}
      }
    public:
      SCondEntropy() = default;
      SCondEntropy(double alpha, long nXParts, long nYParts) : alpha_(alpha), counts_(0.), sumxlogx_(0.), sumxylogxy_(0.), nXParts_(nXParts), nYParts_(nYParts), nNonEmptyXParts_(0) {}
      SCondEntropy(const SCondEntropy&) = default;

      SCondEntropy& operator+=(const SEntropy& other) {
	assert(alpha_ == other.alpha());
	assert(nYParts_ == other.nParts());
	double count = other.counts();
	counts_ += count;
	sumxlogx_ += xlogx(count + nYParts_ * alpha_);
	sumxylogxy_ += other.sumxlogx();
	++nNonEmptyXParts_;
	return *this;
      }
 
      long nXParts() const { return nXParts_; }
      long nYParts() const { return nYParts_; }
      long counts() const { return counts_; }

      double sumxylogxy()  {
	update();
	return sumxylogxy_;
      }
      
      double sumxlogx() {
	update();
	return sumxlogx_;
      }
      operator double() {
	return (sumxlogx() - sumxylogxy()) / (counts_ + nXParts_ * nYParts_ * alpha_);
      }
    };

    
    template<typename Partition>
    class SmoothedEntropy : public PartitionScore<Partition, double> {
      
      double alpha_;
      SEntropy H_;
      
    public:
      using size_type = typename Partition::size_type;
      using value_t = double;

      SmoothedEntropy(double alpha = 1) : alpha_(alpha), H_() {}
      SmoothedEntropy(const SmoothedEntropy&) = default;

      bool accept(value_t entropy, value_t threshold) const {
	return entropy <= threshold;
      }

      void begin(size_type nParts) {
	H_ = SEntropy(alpha_, nParts);
      }

      void begin(size_type nPartsX, size_type nPartsY) {
	H_ = SEntropy(alpha_, nPartsX * nPartsY);
      }
      
      void update(long count) {
	H_ += count;
      }
      
      operator value_t() {
	return H_;
      }
      
      SmoothedEntropy operator()(const Partition& partition) const {
	return partition.score(*this);
      }
      
      value_t min() { return 0.; }
      value_t max() { return std::log2(H_.nParts()); }
    };


    
    template<typename Partition, bool active_bound1, bool active_bound2>
    class SmoothedInformation : public PartitionScore<Partition, double> {
      using size_type = typename Partition::size_type;

      double alpha_, aloga_, n_, IXY_, bound_;
      const Partition *partitionY_;

      SEntropy HXa_;
      SEntropy HYx_;
      SCondEntropy HYgX_;
      size_type NX_, NY_;
      std::vector<double> nys_;
      
      inline static size_type bestBound1_ = 0, bestBound2_ = 0;
      
    public:
      using value_t = double;
      
      static void displayRatio() {
	double ratio = (1. * bestBound2_) / (bestBound1_ + bestBound2_);
	std::clog << "Ratio of best bound 2 = " << ratio << std::endl;
      }
      
      static bool comparator(value_t v1, value_t v2) {
	return v1 < v2;
      }

      SmoothedInformation(double alpha = 1) : alpha_(alpha), aloga_(xlogx(alpha)), partitionY_(), HXa_(), HYx_(), HYgX_() {}
      SmoothedInformation(const SmoothedInformation&) = default;

      void setTarget(const Partition& target) {
	partitionY_ = &target;
	NY_ = target.nParts();
	nys_ = CountCollecter<Partition>{}(*partitionY_);
      }

      void begin(size_type NX, size_type NY) {
	NX_ = NX;	
	HXa_ = SEntropy(alpha_, NX_);
	HYgX_ = SCondEntropy(alpha_, NX_, NY_);
      }
      
      void subbegin() {
	HYx_ = SEntropy(alpha_, NY_);
      }
      
      void update(double count) {	
	HYx_ += count;
      }

      void subend() {
	HXa_ += HYx_.counts();
	HYgX_ += HYx_;
      }

      void end() {	
	n_ = HYgX_.counts();
	double HY = computeSmoothedEntropyOfY(alpha_ *  NX_);
	IXY_ = HY - HYgX_;
	if constexpr(active_bound1) {
	    if constexpr(active_bound2) {
		double boundValue1 = bound1();
		double boundValue2 = bound2();
		bound_ = std::min(boundValue1, boundValue2);
		if(boundValue2 < boundValue1) {
		  ++bestBound2_;
		} else
		  ++bestBound1_;		  
	      } else {
	      bound_ = bound1();
	    }
	  } else if constexpr(active_bound2) {
	    bound_ = bound2();
	  } else {
	  bound_ = naiveBound();
	}
      }      
      
      double computeSmoothedEntropyOfY(double alpha) {
	SEntropy H(alpha, NY_);
	for(auto ny : nys_) {
	  H += ny;
	}
	return H;
      }
      
      template<typename Func> double newtonRaphson(Func func, double minNZ, double maxNZ) {
	constexpr double eps = 0.1;
	double NZ = minNZ, dNZ;
	do {
	  auto p = func(NZ);
	  dNZ = - p.first / p.second;
	  if(NZ + dNZ < minNZ) dNZ = minNZ - NZ;
	  else if(NZ + dNZ > maxNZ) dNZ = maxNZ - NZ;
	  if(p.first * dNZ <= 0) break;
	  NZ += dNZ;
	} while(std::abs(dNZ) > eps);
	return NZ;
      }
      
      double naiveBound() {
	double S = HYgX_.sumxlogx() - HXa_.sumxlogx();
	return (std::log2(NY_) - (S - NX_ * (NY_- 1) * aloga_) / (n_ + NX_ * NY_ * alpha_));
      }

      std::pair<double,double> bound1Derivate(double NZ, double c0) const {
	double S1 = 0., S2 = 0.;
	for(auto ny : nys_) {
	  double c1 = ny + NZ * alpha_;
	  double c2 = ny - n_ / NY_;
	  S1 += c2 * std::log2(c1);
	  S2 += c2 / c1;
	}
	double f = (S1 - n_ * std::log2(NY_)) - c0;
	double fprime = alpha_ * S2 / log2;
	return { f, fprime };
      }

      double bound1() {
	double c0;
	{
	  double S = HXa_.sumxlogx() - HYgX_.sumxlogx();
	  c0 = NX_ * alpha_ * xlogx(NY_) + NX_ * (NY_ - 1)* aloga_ + S;
	}

	double NZ = newtonRaphson([c0, this] (double NZ) { return bound1Derivate(NZ, c0); }, NX_, NX_ * NY_);

	double S = 0.;
	for(auto ny : nys_)
	    S += xlogx(ny + NZ * alpha_);
	double n = HYgX_.counts();
	double c = n + NZ * NY_ * alpha_;
	return (std::log2(c) + (c0 - S - NZ * NY_ * alpha_ * std::log2(NY_)) / c);
      }

      std::pair<double,double> bound2Derivate(double NZ, double c0) const {
	double S1 = 0., S2 = 0.;

	for(auto ny : nys_) {
	  double c1 = ny + NZ * alpha_;
	  double c2 = ny - n_ / NY_;
	  S1 += c2 * std::log2(c1);
	  S2 += c2 / c1;
	}
	double c = n_ + NZ * NY_ * alpha_;	
	double f = S1 - c0 + (n_ + NX_ * NY_) * aloga_ + n_ * c / (NZ * NY_ * alpha_) / log2;
	double fprime = alpha_ * S2 - n_ * n_ / (NZ * NZ * NY_ * alpha_) / log2 ;
	return { f, fprime };
      }

      double bound2() {
	double c0 = HYgX_.sumxylogxy();
	double NZ = newtonRaphson([c0, this] (double NZ) { return bound2Derivate(NZ, c0); }, NX_, NX_ * NY_);

	double S = 0.;
	for(auto ny : nys_) {
	  S += xlogx(ny + NZ * alpha_);
	}
	double bound = std::log2(NZ) + (c0 - S + (NZ - NX_) * NY_ * aloga_) / (n_ + NZ * NY_ * alpha_);
	return bound;
      }      
      
      SmoothedInformation operator()(const Partition& partitionX) const {
	auto s = partitionX.intersect(*partitionY_, *this);
	return s;
      }

      operator std::pair<value_t, value_t>() const {
	return { IXY_, bound_ };
      }
    };


    template<typename Partition>
    class ReliableFractionOfInformation : public PartitionScore<Partition, double> {
      using size_type = typename Partition::size_type;

      double HX_, HY_, HXY_, bias_, RFI_;
      unsigned long nx_, n_, n2_;
      unsigned long NX2_, NX_, NY_;
      double boundBias_, bound_;
      std::vector<double> nys_;
      const Partition *partitionY_;

      double hyperGeometricProbLog(unsigned long k, unsigned long a, unsigned long b, unsigned long n) {
	if(a > n || b > n || (k+n < a+b) || k > a || k > b)
	  return 0.;
	if(a < b) std::swap(a,b);

	double res = 0.;
	unsigned long p1 = a;
	unsigned long p2 = b;
	unsigned long p3 = n;
	unsigned long p4 = k;
	unsigned long p5 = n-a;

	for(unsigned long i = 0; i != k; ++i, --p1, --p2, --p3, --p4) 
	  res += std::log2(double(p1) / p3 * p2 / p4);
      
	for(unsigned long i = 0; i != b-k; ++i, --p3, --p5)
	  res += std::log2(double(p5) / p3);
      
	if(res > 1.) {
	  std::cerr << "res = " << res << std::endl;
	  throw std::runtime_error("Bad res");
	}
	return res;
      }
      
      void updateBias(double& bias, unsigned long ai, unsigned long bj) {
	unsigned long n = n_;
	// std::clog << "ai = " << ai << " bj = " << bj << " n = " << n << std::endl;
	double m = ai+bj <= n+1 ? 1 : ai+bj-n;
	double M = std::min(ai, bj);
	double logh = hyperGeometricProbLog(m, ai, bj, n);

	double total = 0.;
	for(unsigned long k = m; k <= M; ++k) {
	  double h = std::exp2(logh);
	  double term = h * k * std::log2(k);
	  total += term;
	  double c = double(ai-k) / (k+1) * (bj -k) / (n - ai - bj + k + 1);
	  if(c != 0.)
	    logh += std::log2(c);
	}
	// std::clog << "total1 = " << total << std::endl;
	double p = double(ai) / n * bj / n;
	total -= xlogx(p*n);
	bias += total / n;
	//	std::clog << "bias = " << bias << std::endl;
      }
            
    public:
      using value_t = double;

      static bool comparator(value_t v1, value_t v2) {
	return v1 < v2;
      }

      ReliableFractionOfInformation() = default;
      ReliableFractionOfInformation(const ReliableFractionOfInformation&) = default;
      
      void setTarget(const Partition& target) {
	partitionY_ = &target;
	Entropy H = Entropy<Partition>{}(*partitionY_);
	HY_ = H;
	n_ = H.count();
	nys_ = CountCollecter<Partition>{}(*partitionY_);
      }

      void begin(size_type NX, size_type NY) {
	NX_ = NX; NY_ = NY; NX2_ = 0;
	HX_ = HXY_ = bias_ = boundBias_ = 0.;
	n2_ = 0;
      }
      
      void subbegin() {
	nx_ = 0.;
      }
      
      void update(double nxy) {
	HXY_ += xlogx(nxy);
	nx_ += nxy;
	for(double ny : nys_)
	  updateBias(boundBias_, nxy, ny);
      }

      void subend() {
	HX_ += xlogx(nx_);
	++NX2_;
	for(double ny : nys_)
	  updateBias(bias_, nx_, ny);
	n2_ += nx_;
      }
      
      void end() {
	double logn = std::log2(n_);
	HX_ = logn - HX_ / n_;
	HXY_ = logn - HXY_ / n_;
	bias_ /= HY_;
	boundBias_ /= HY_;
	RFI_ = (HY_ + HX_ - HXY_) / HY_ - bias_;
	bound_ = 1. - boundBias_;

#ifdef DEBUG_COUNTS
	std::cerr << "\nNX   = " << NX_ << " versus " << NX2_;
	std::cerr << "IXY  = " << (HY_ + HX_ - HXY_) << std::endl;
	std::cerr << "bias = " << bias_ << std::endl;
	std::cerr << "bbias= " << boundBias_ << std::endl;
#endif		
      }
      
      ReliableFractionOfInformation operator()(const Partition& partitionX) const {
	return partitionX.intersect(*partitionY_, *this);	
      }

      operator std::pair<value_t, value_t>() const {
      	return { RFI_, bound_ };
      }      
    };


    /* See: 
       "Reconsidering Mutual Information Based Feature Selection: Statistical Significance View", Vinh
    */
    template<typename Partition>
    class AdjustedDependency : public PartitionScore<Partition, double> {
      using size_type = typename Partition::size_type;

      double HX_, HY_, HXY_, alpha_;
      unsigned long nx_, n_;
      unsigned long dOfFreedom_;
      double adjustedDependency_, bound_;
      const Partition *partitionY_;
            
    public:
      using value_t = double;

      static bool comparator(value_t v1, value_t v2) {
	return v1 < v2;
      }

      AdjustedDependency(double alpha) : alpha_(alpha) {};
      AdjustedDependency(const AdjustedDependency&) = default;
      
      void setTarget(const Partition& target) {
	partitionY_ = &target;
	Entropy H = Entropy<Partition>{}(*partitionY_);
	HY_ = H;
	n_ = H.count();
	unsigned long NY = target.nParts();
	dOfFreedom_ = NY - 1;
	// std::cerr << "SET DOF = " << dOfFreedom_ << std::endl;	
      }

      void begin(size_type NX, size_type NY) {
	HX_ = HXY_ = 0;
	// std::cerr << "UPDATE DOF " << NX << " -> " << dOfFreedom_ << std::endl;
      }
      
      void subbegin() {
	nx_ = 0.;
      }
      
      void update(double nxy) {
	HXY_ += xlogx(nxy);
	nx_ += nxy;
      }

      void subend() {
	HX_ += xlogx(nx_);
      }
      
      void end() {
	double logn = std::log2(n_);
	HX_ = logn - HX_ / n_;
	HXY_ = logn - HXY_ / n_;
	double info = (HY_ + HX_ - HXY_);
	double bias = stats::chiTestCriticalValue(alpha_, dOfFreedom_)/ (2. * n_);
	adjustedDependency_ = (info - bias) / HY_;
	bound_ = 1. - bias;	
	// std::cerr << "info = " << info << std::endl;
	// std::cerr << "DOF = " << dOfFreedom_ << std::endl;
	// std::cerr << "bias = " << bias << std::endl;
      }
      
      AdjustedDependency operator()(const Partition& partitionX) const {
	AdjustedDependency scorer = *this;
	size_t NX = partitionX.size();
	if(NX > 1) scorer.dOfFreedom_ *= (NX - 1);
	return partitionX.intersect(*partitionY_, scorer);	
      }

      operator std::pair<value_t, value_t>() const {
      	return { adjustedDependency_, bound_ };
      }      
    };

    /* See: 
       "Mutual Information Estimation: Independence Detection and Consistency", Suzuki
    */
    template<typename Partition>
    class SuzukiInfo : public PartitionScore<Partition, double> {
      using size_type = typename Partition::size_type;

      double HX_, HY_, HXY_;
      unsigned long NX_, NY_, nx_, n_;
      double suzukiInfo_, bound_;
      const Partition *partitionY_;
            
    public:
      using value_t = double;

      static bool comparator(value_t v1, value_t v2) {
	return v1 < v2;
      }

      SuzukiInfo() = default;
      SuzukiInfo(const SuzukiInfo&) = default;
      
      void setTarget(const Partition& target) {
	partitionY_ = &target;
	Entropy H = Entropy<Partition>{}(*partitionY_);
	HY_ = H;
	n_ = H.count();
	NY_ = target.nParts();
      }

      void begin(size_type NX, size_type NY) {
	NX_ = NX;
	HX_ = HXY_ = 0;
      }
      
      void subbegin() {
	nx_ = 0.;
      }
      
      void update(double nxy) {
	HXY_ += xlogx(nxy);
	nx_ += nxy;
      }

      void subend() {
	HX_ += xlogx(nx_);
      }
      
      void end() {
	double logn = std::log2(n_);
	HX_ = logn - HX_ / n_;
	HXY_ = logn - HXY_ / n_;
	double info = (HY_ + HX_ - HXY_);
	double bias = (NX_ - 1) * (NY_ - 1)/ (2. * n_) * std::log2(n_);
	suzukiInfo_ = (info - bias) / HY_;
	bound_ = 1. - bias;	
	// std::cerr << "info = " << info << std::endl;
	// std::cerr << "bias = " << bias << " NX = " << NX_ << std::endl;
      }
      
      SuzukiInfo operator()(const Partition& partitionX) const {
	SuzukiInfo scorer = *this;
	return partitionX.intersect(*partitionY_, scorer);	
      }

      operator std::pair<value_t, value_t>() const {
      	return { suzukiInfo_, bound_ };
      }      
    };    
        
  }
}
