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

#include <cassert>
#include <iterator>
#include <gimlet/data_stream.hpp>

namespace gimlet {

  template<class T, class InputDataStream, class Distance = std::ptrdiff_t> class istream_iterator: public std::iterator<
    std::input_iterator_tag, T, Distance, const T*, const T&> {
    InputDataStream* stream_;
    T value_;
    void read() {
      assert(stream_ != nullptr);
      *stream_ >> value_;
      if(! *stream_)
	stream_ = nullptr;
    }

  public:
    istream_iterator() = default;
    
    istream_iterator(InputDataStream& stream) :
      stream_(&stream) {
      read();
    }
    
    istream_iterator(const istream_iterator& other) = default;

    const T& operator*() const {
      return value_;
    }
    const T* operator->() const {
      return &value_;
    }
    istream_iterator<T, InputDataStream, Distance>& operator++() {
      read();
      return *this;
    }
    istream_iterator<T, InputDataStream, Distance> operator++(int) {
      istream_iterator<T, InputDataStream, Distance> tmp = *this;
      ++*this;
      return tmp;
    }

    bool operator!=(const istream_iterator& other) const {
      if (stream_ && other.stream_)
	return (*stream_ != *other.stream_);
      else
	return (stream_ || other.stream_);
    }
    bool operator==(const istream_iterator& other) const {
      return !(*this != other);
    }
  };

  
  template<typename T,
	   typename OutputDataStream,
	   typename Distance = std::ptrdiff_t>
  class ostream_iterator: public std::iterator<std::output_iterator_tag, T, Distance, const T*, const T&> {
    OutputDataStream* stream_;
    T value_;
    bool changed_;

  public:
    ostream_iterator() :
      stream_(nullptr) {
    }
    ostream_iterator(OutputDataStream& stream) :
      stream_(&stream), changed_(false) {
    }
    ostream_iterator(const ostream_iterator& other) = default;
    ~ostream_iterator() {
      ++(*this);
    }

    T& operator*() {
      changed_ = true;
      return value_;
    }

    ostream_iterator<T, OutputDataStream, Distance>& operator++() {
      if(changed_) {
	changed_ = false;	 
	*stream_ << value_;
      }
      return *this;
    }
    
    ostream_iterator<T, OutputDataStream, Distance> operator++(int) {
      ++(*this);
      return *this;
    }

    bool operator!=(const ostream_iterator& other) const {
      if (stream_ && other.stream_)
	return (*stream_ != *other.stream_);
      else
	return (stream_ || other.stream_);
    }

    bool operator==(const ostream_iterator& other) const {
      return !(*this != other);
    }
  };

  template<typename InputDataStream, typename Value = typename InputDataStream::default_value_type>
  istream_iterator<Value, InputDataStream> make_input_data_begin(InputDataStream& inputDataStream) {
    return istream_iterator<Value, InputDataStream>(inputDataStream);
  }

  template<typename InputDataStream, typename Value = typename InputDataStream::default_value_type>
  istream_iterator<Value, InputDataStream> make_input_data_end(InputDataStream&) {
    return istream_iterator<Value, InputDataStream>();
  }

  template<typename OutputDataStream, typename Value = typename OutputDataStream::default_value_type>
  using output_stream_iterator_t = ostream_iterator<Value, OutputDataStream>;
  
  template<typename OutputDataStream, typename Value = typename OutputDataStream::default_value_type>
  output_stream_iterator_t<OutputDataStream, Value> make_output_iterator(OutputDataStream& outputDataStream) {
    return output_stream_iterator_t<OutputDataStream, Value>(outputDataStream);
  }

  
}


