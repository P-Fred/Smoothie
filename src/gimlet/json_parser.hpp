#pragma once

/*
 *   Copyright (C) 2017,  CentraleSupelec
 *
 *   Authors : Frédéric Pennerath
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

#include<iostream>
#include<string>
#include<vector>
#include<tuple>
#include<stdexcept>
#include<type_traits>

#include <gimlet/stringstream.hpp>
#include <gimlet/internal/parser.hpp>

namespace gimlet {

  using namespace format;

  template<typename Format> struct JSONParserBase : public ParserBase<Format> {
    static const size_t TAB_WIDTH = 2;
    size_t tabs_;

    JSONParserBase(size_t tabs = 0) : tabs_(tabs) {}
  };
  
  template<typename Format, typename Enable = void>
  struct JSONParser {
  }; 
  
  struct Tabs {
    size_t n_;
    char c_;
    Tabs(size_t n) : n_(n), c_(' ') {}
    std::ostream& operator()(std::ostream& os) const {
      if(n_ != 0) os << std::string(n_, c_);
      return os;
    }
  };
  inline Tabs tabs(size_t n) { return Tabs{n}; }
  
  inline std::ostream& operator<<(std::ostream& os, const Tabs& tabs) {
    return tabs(os);
  }
  
  template<typename Format>
  struct JSONParser<Format, std::enable_if_t<std::is_integral_v<Format>>> : public JSONParserBase<Format> {
    JSONParser(size_t tabs = 0) : JSONParserBase<Format>(tabs) {}
    
    void read(std::istream& is, Format& val) const {
      try {
	if constexpr(std::is_same_v<Format,char> || std::is_same_v<Format, unsigned char>) {
	    int i;
	    is >> i;
	    val = Format(i);
	  } else {
	  is >> val;
	}
      } catch (const std::istream::failure& e) {
	std::string message("parsing of type \"");
	(message += typeid(Format).name()) += "\" failed";
	throw internal::formatError(is, message);
      }
    }

    template<typename Value>
      std::enable_if_t<is_read_compatible_v<Value, Format>>
      read(std::istream& is, Value& val) const {
      try {
	if constexpr(std::is_same_v<Format,char> || std::is_same_v<Format, unsigned char>) {
	    int i;
	    is >> i;
	    val = Value(i);
	  } else {
	  Format t;
	  is >> t;
	  val = Value(t);
	}
      } catch (const std::istream::failure& e) {
	std::string message("parsing of type \"");
	(message += typeid(Format).name()) += "\" failed";
	throw internal::formatError(is, message);
      }
    }    
    
    void write(std::ostream& os, const Format& val) const {
      if constexpr(std::is_same_v<Format,char> || std::is_same_v<Format, unsigned char>) {
	  os << static_cast<int>(val);
	  } else
	  os << val;
    }

    template<typename Value>
      std::enable_if_t<is_write_compatible_v<Value,Format>>
      write(std::ostream& os, const Value& val) const {
      if constexpr(std::is_same_v<Format,char> || std::is_same_v<Format, unsigned char>) {
	  os << static_cast<int>(val);
	  } else
      os << static_cast<Format>(val);
    }
  };

  template<typename Format>
  struct JSONParser<Format, std::enable_if_t<std::is_floating_point_v<Format>>> : public JSONParserBase<Format> {
    JSONParser(size_t tabs = 0) : JSONParserBase<Format>(tabs) {}
    
    template<typename Value>
      std::enable_if_t<is_read_compatible_v<Value, Format>>
      read(std::istream& is, Value& number) const {
      try {
	is >> cool::double_imanip() >> number;
      } catch (const std::istream::failure& e) {
	std::string message("parsing of type \"");
	(message += typeid(Value).name()) += "\" failed";
	throw std::runtime_error(message);
      }
    }

    template<typename Value>
      std::enable_if_t<is_write_compatible_v<Value, Format>>
      write(std::ostream& os, const Value& number) const {
      os << number;
    }
  };
  
  template<typename Format>
  struct JSONParser<list<Format>> : JSONParserBase<list<Format>> {
    using typename ParserBase<list<Format>>::data_format;

    JSONParser<Format> elementParser_;
    
  public:
    JSONParser(size_t tabs = 0) : JSONParserBase<list<Format>>(tabs) {}

    template<typename Value>
      std::enable_if_t<is_read_compatible_v<Value, data_format>>
      read(std::istream& is, Value& seq) const {
      seq.clear();
      char c = internal::expectOneOf(is, "[",
				     "a list" " starts with" " a left square bracket");

      is >> std::skipws >> c;
      if (c != ']') {
	is.putback(c);

	std::back_insert_iterator<Value> ii(seq);
	typename Value::value_type d;
	while (is.good()) {
	  elementParser_.read(is, d);
	  *ii++ = std::move(d);
	  c = internal::expectOneOf(is, ",]",
				    "list" " elements are separated by" " coma");
	  if (c == ']')
	    return;
	}
	throw internal::formatError(is,
				    "a list" " ends with" " a right square bracket");
      }
    }

    template<typename Value>
      std::enable_if_t<is_write_compatible_v<Value, data_format>>
      write(std::ostream& os, const Value& seq) const {
      os << '[';
      auto it = seq.begin();
      if (it != seq.end()) {
	elementParser_.write(os, *it);
	for (++it; it != seq.end(); ++it) {
	  os << ", ";
	  elementParser_.write(os, *it);
	}
      }
      os << ']';
    }
  };
  
  template<typename Tuple, int i, int c>
  struct JSONTupleComponentParser {
    using head_type = typename std::tuple_element<i, Tuple>::type;
    using head_parser = JSONParser<head_type>;
    using tail_parser = JSONTupleComponentParser<Tuple, i + 1, c - 1>;
    
    head_parser headParser_;
    tail_parser tailParser_;

    template<typename Value>
    void read(std::istream& is, Value& t) const {      
      headParser_.read(is, std::get < i > (t));
      internal::expectOneOf(is, ",", "tuple"" elements are separated with"" coma");
      tailParser_.read(is, t);
    }

    template<typename Value>
    void write(std::ostream& os, const Value& t) const {
      headParser_.write(os, std::get < i > (t));
      os << ", ";
      tailParser_.write(os, t);
    }
  };

  template<typename Tuple, int i>
  struct JSONTupleComponentParser<Tuple, i, 0> {

    template<typename Value>
    void read(std::istream&, Value&) const {}

    template<typename Value>
    void write(std::ostream&, const Value&) const {}
  };

  template<typename Tuple, int i>
  class JSONTupleComponentParser<Tuple, i, 1> {
    using head_type = typename std::tuple_element<i, Tuple>::type;

    JSONParser<head_type> headParser_;
    
  public:
    template<typename Value>
    void read(std::istream& is, Value& t) const {      
      headParser_.read(is, std::get < i > (t));
    }

    template<typename Value>
    void write(std::ostream& os, const Value& t) const {
      headParser_.write(os, std::get < i > (t));
    }
  };

  template<typename ... Args> class JSONParser<tuple<Args...>> : public JSONParserBase<gimlet::tuple<Args...>> {
    using typename ParserBase<tuple<Args...>>::data_format;
    using tuple_type = std::tuple<Args...>;

    JSONTupleComponentParser<tuple_type, 0, std::tuple_size<tuple_type>::value> tupleParser_;
  public:    
    JSONParser(size_t tabs = 0) : JSONParserBase<gimlet::tuple<Args...>>(tabs) {}

    template<typename Value>
      std::enable_if_t<is_read_compatible_v<Value, data_format>>
      read(std::istream& is, Value& t) const {
      internal::expectOneOf(is, "[", "a tuple"" starts with"" a left"" square bracket");
      tupleParser_.read(is, t);
      internal::expectOneOf(is, "]", "a tuple"" ends with"" a right"" square bracket");
    }

    template<typename Value>
      std::enable_if_t<is_write_compatible_v<Value, data_format>>
      write(std::ostream& os, const Value& t) const {
      os << '[';
      tupleParser_.write(os, t);
      os << ']';
    }
  };

  template<typename Format> class JSONParser<flow<Format>> : public JSONParserBase<flow<Format>> {
    using typename ParserBase<gimlet::flow<Format>>::data_format;
    using element_format = data_format_t<Format>;
    JSONParser<element_format> elementParser_;
    mutable bool first_ = true;
    mutable bool finished_ = false;
    
  public:
    JSONParser(size_t tabs = 0) : JSONParserBase<flow<Format>>(tabs), elementParser_(tabs + this->TAB_WIDTH) {}
    
    bool finished() const { return finished_; }
    
    template<typename Value>
      std::enable_if_t<is_read_compatible_v<Value,data_format>>
      read(std::istream& is, Value& val) const {
      char c;
      is >> std::skipws >> c;
      if(c == ']') {
	finished_ = true;
      } else {
	if(first_) {
	  is.putback(c);
	  first_ = false;
	} else {
	  if(c != ',')
	    throw internal::formatError(is, "flow"" elements are separated with"" coma");
	}
	elementParser_.read(is, val);
      }
    }
    
    template<typename Value>
      std::enable_if_t<is_write_compatible_v<Value,data_format>>
      write(std::ostream& os, const Value& val) const {
      if(first_) {
	first_ = false;
      } else
	os << ",\n";
      os << tabs(this->tabs_ + this->TAB_WIDTH);
      elementParser_.write(os, val);
    }
    
    void readBegin(std::istream& is) const {
      internal::expectOneOf(is, "[", "a flow"" starts with"" a left"" square bracket");
    }
    
    void readEnd(std::istream& is) const {
      if(! finished_) {
	finished_ = true;
	internal::expectOneOf(is, "]", "a flow"" ends with"" a right"" square bracket");
      }
    }

    void writeBegin(std::ostream& os) const {
      os << tabs(this->tabs_) << "[\n";
    }
    
    void writeEnd(std::ostream& os) const {
      if(! finished_) {
	finished_ = true;
	os << '\n' << tabs(this->tabs_) << "]\n";
      }
    }
  }; 


  template<typename Data>
  JSONParser<data_format_t<Data>> make_JSON_parser() {
    return {};
  }
}
