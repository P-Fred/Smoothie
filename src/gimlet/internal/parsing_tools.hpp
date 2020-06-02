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
#include <vector>

namespace gimlet {
  namespace internal {
    class FormatException: public std::runtime_error {
    public:
      template<typename String>
      FormatException(const String& msg) :
	std::runtime_error(msg) {
      }
    };
  
    FormatException formatError(std::string msg);
    FormatException formatError(std::istream& is, std::string msg);

    char expectOneOf(std::istream& is, const char* chars,
		     const char* msg = nullptr);
    
    int expect(std::istream& is, const std::vector<const char*>& words);
  }
}
