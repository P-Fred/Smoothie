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

#include "gimlet/statistics.hpp"

namespace cool {

  Statistics::Entry::Entry(const char* name, const char* unit) : name_(name), unit_(unit) {}
  void Statistics::Entry::fullWrite(std::ostream& os) {
    os << name_ << " = ";
    write(os);
    if(unit_) os << ' ' << unit_;
  }
  
  Statistics::Entry::~Entry() {}

  Statistics::IntegerEntry::IntegerEntry(const char* name, unsigned int& variable, const char* unit) : Entry(name, unit), value_(&variable) {
    variable = 0;
  }
  void Statistics::IntegerEntry::write(std::ostream& os) {
    os << *value_;
  }

  Statistics::DoubleEntry::DoubleEntry(const char* name, double& variable, const char* unit) : Entry(name, unit), value_(&variable) {
    variable = 0.;
  }
  
  void Statistics::DoubleEntry::write(std::ostream& os) {
    os << *value_;
  }

  Statistics::StringEntry::StringEntry(const char* name, std::string& variable, const char* unit) : Entry(name, unit), value_(&variable) {
  }
  void Statistics::StringEntry::write(std::ostream& os) {
    os << *value_;
  }
  
  Statistics::Statistics(const std::string& testId) : fileName_(), comment_(), variables_(), fullPrinting_(true), enabled_(false), testId_(testId) {} 

  void Statistics::addInteger(const char* name, unsigned int& variable, const char* unit) {
    variables_.push_back(std::make_shared<IntegerEntry>(name, variable, unit));
  }

  void Statistics::addDouble(const char* name, double& variable, const char* unit) {
    variables_.push_back(std::make_shared<DoubleEntry>(name, variable, unit));
  }

  void Statistics::setTestId(const std::string& testId) { testId_ = testId; }

  void Statistics::open(std::string statisticsFileName, const char* comment) {
    if(statisticsFileName == "-") {
      statisticsFileName.clear();
      fullPrinting_ = true;
    } else
      fullPrinting_ = false;
      
    fileName_ = statisticsFileName;
    comment_ = comment;
    enabled_ = true;
  }

  void Statistics::write() const {
    if(enabled_) {
      std::ostringstream header;
      bool writeHeader = false;
      if(! (fullPrinting_ || comment_.empty())) {
	writeHeader = true;
	header << comment_;
	if(! testId_.empty()) header << " id,";
	bool first = true;
	for(auto& entry : variables_) {
	  if(first) first = false; else header << ',';
	  header << ' ' << entry->name_;
	}
	header << std::endl;
	
	if(! fileName_.empty()) {
	  std::ifstream ifs(fileName_);
	  if(ifs.good()) {
	    std::string line;
	    std::getline(ifs,line);
	    if(ifs.good()) {
	      if(line != header.str()) throw std::runtime_error("Statistics file header mismatch");
	      else writeHeader = false;
	    }
	  }
	}
      }
      
      std::ostream* os = &std::clog;
      std::ofstream file;
      if(! fileName_.empty()) {
	file.open(fileName_, std::ios::out | std::ios::app);
	if(! file.good()) throw std::runtime_error(std::string("Cannot open statistics file \"") + fileName_ + "\"");
	else os = &file;
      }
      
      if(writeHeader) *os << header.str();
      if(! testId_.empty()) *os << '\r' <<  testId_ << ' ';    
      for(auto& entry : variables_)
	if(fullPrinting_) {
	  *os << '\r'; entry->fullWrite(*os); *os << std::endl;
	} else {
	  entry->write(*os); *os << ' ';
	}
      if(! fullPrinting_) *os << std::endl;
    }
  }
}
