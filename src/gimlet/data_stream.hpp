#pragma once
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
#include <stdexcept>

//#include <gimlet/data_format_parser.hpp>

namespace gimlet {

  void initDataStreams();

  template<typename Parser> class InputDataStream : public Parser {
    std::istream* is_;

    void initInputStream() {
      initDataStreams();
      is_->exceptions(std::istream::badbit);
      this->readBegin(*is_);
    }
  public:
    
    InputDataStream() = default;
    InputDataStream(std::istream& is, const Parser& parser = Parser()) :
      Parser(parser), is_(&is) {
      initInputStream();
    }
    InputDataStream(const InputDataStream& other) = delete;
    InputDataStream(InputDataStream&& other) : Parser(std::move(other)), is_(other.is_) {
      other.is_ = nullptr;
    }
    InputDataStream& operator=(InputDataStream&& other) {
      ((Parser&) *this) = ((Parser&) other);
      is_ = other.is_;
      other.is_ = nullptr;
      return *this;
    }
    ~InputDataStream() {
        close();
    }
    
    void close() {
        if(is_)
	  //        if(is_ && ! std::uncaught_exception())
          this->readEnd(*is_);
    }

    InputDataStream& operator>>(std::ios_base& (*pf)(std::ios_base&)) {
      *is_ >> pf;
      return *this;
    }

    InputDataStream& operator>>(std::ios& (*pf)(std::ios&)) {
      *is_ >> pf;
      return *this;
    }

    InputDataStream& operator>>(std::istream& (*pf)(std::istream&)) {
      *is_ >> pf;
      return *this;
    }

    template<typename Value>
    InputDataStream& operator>>(Value& data) {
      Parser::read(*is_, data);
      return *this;
    }

    bool operator!=(const InputDataStream& other) const {
      return is_ != other.is_;
    }

    bool operator!() const {
      return (! is_->good()) || this->finished();
    }

    operator bool() const {
      return is_->good() && (! this->finished());
    }

    bool good() const {
      return is_->good();
    }

    int peek() {
      return is_->peek();
    }
  };

  template<typename Parser> class OutputDataStream : public Parser {

    std::ostream* os_;

    void initOutputStream() {
      initDataStreams();
      this->writeBegin(*os_);
    }

  public:
    OutputDataStream() = default;
    OutputDataStream(std::ostream& os, const Parser& parser = Parser()) :
      Parser(parser), os_(&os) {
      initOutputStream();
    }
    OutputDataStream(const OutputDataStream& other) = delete;
    OutputDataStream(OutputDataStream&& other) : Parser(std::move(other)), os_(other.os_) {
      other.os_ = nullptr;
    }
    OutputDataStream& operator=(OutputDataStream&& other) {
      ((Parser&) *this) = ((Parser&) other);
      os_ = other.os_;
      other.os_ = nullptr;
      return *this;
    }
    
    ~OutputDataStream() {
        close();
    }
    
    void close() {
      if(os_)
	this->writeEnd(*os_);
    }


    OutputDataStream& operator<<(std::ios& (*pf)(std::ios&)) {
      *os_ << pf;
      return *this;
    }

    OutputDataStream& operator<<(std::ios_base& (*pf)(std::ios_base&)) {
      *os_ << pf;
      return *this;
    }

    OutputDataStream& operator<<(std::ostream& (*pf)(std::ostream&)) {
      *os_ << pf;
      return *this;
    }

    template<typename Value>
    OutputDataStream& operator<<(const Value& data) {
      Parser::write(*os_, data);
      return *this;
    }

    bool operator!=(const OutputDataStream& other) const {
      return os_ != other.os_;
    }
  };

  template<typename Parser>
  using input_stream_t = InputDataStream<Parser>;

  template<typename Parser>
  input_stream_t<Parser> make_input_data_stream(std::istream& is, const Parser& parser = Parser()) {
    return InputDataStream<Parser>(is, parser);
  }
 
  template<typename Parser>
  using output_stream_t = OutputDataStream<Parser>;

  template<typename Parser>
  output_stream_t<Parser> make_output_data_stream(std::ostream& os, const Parser& parser = Parser()) {
    return OutputDataStream<Parser>(os, parser);
  }

  template<typename... StreamTypes>
  struct StreamTypeIdentifier {
    static int getIndex(int index, const std::string& streamTypeString) {
      return -1;
    }
  };

  // template<typename StreamType, typename... StreamTypes>
  // struct StreamTypeIdentifier<StreamType, StreamTypes...> {
  //   static int getIndex(int index, const std::string& streamTypeString) {
  //     if(gimlet::iscompatible<StreamType>(streamTypeString))
  // 	return index;
  //     else
  // 	return StreamTypeIdentifier<StreamTypes...>::getIndex(++index, streamTypeString);
  //   }
  // };

  // template<template<typename S> class Parser, typename... StreamTypes> int identifyStreamType(std::istream& is) {
  //   Parser<std::string> stringParser;
  //   std::string streamTypeString;
  //   stringParser.read(is, streamTypeString);
  //   return StreamTypeIdentifier<StreamTypes...>::getIndex(0, streamTypeString);
  // }
}
