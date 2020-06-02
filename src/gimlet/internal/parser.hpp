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

#include <gimlet/stringstream.hpp>
#include <gimlet/internal/parsing_tools.hpp>
#include <gimlet/data_format.hpp>

namespace gimlet {
  struct ParserTemplate {
    bool finished() const { return true; }
    void readBegin(std::istream&) const {}
    void writeBegin(std::ostream&) const {}
    void readEnd(std::istream&) const {}
    void writeEnd(std::ostream&) const {}
  };

  template<typename DataFormat>
  struct ParserBase : public ParserTemplate {
    using data_format = DataFormat;  
    using default_value_type = typename data_format_traits<data_format>::default_value_type;
  };
  
  using gimlet::internal::FormatException;

  template<typename Format, typename Value>
  [[noreturn]] void throwTypeMismatchError() {
    std::string formatName = gimlet::data_format_traits<Format>::name();
    std::string valueName = cool::type_traits<Value>::name();
    throw internal::formatError(std::string("parser type mismatch: expect a value compatible with \"") + formatName + "\", get a \"" + valueName + "\"");
  }
}
