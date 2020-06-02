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

#include <gimlet/internal/parsing_tools.hpp>

namespace gimlet {
  namespace internal {
    FormatException formatError(std::string msg) {
      return FormatException(msg);
    }

    FormatException formatError(std::istream& is, std::string msg) {
      char location[20];
      if (is.good()) {
	is.getline(location, sizeof(location));
	(msg += " (at >>>") += location;
	if (is.gcount() >= 19)
	  msg += " ...";
	msg += "<<<)";
      }
      return FormatException(msg);
    }
    
    char expectOneOf(std::istream& is, const char* chars,
		     const char* msg) {
      char c;
      is >> std::skipws >> c;
      for (const char* ptr = chars; *ptr != 0; ++ptr) {
	if (*ptr == -1) {
	  if (is.eof())
	    return -1;
	} else if (*ptr == c)
	  return c;
      }
      
      std::string fullMessage;
      if (msg == nullptr) {
	fullMessage = "char in \"";
	(fullMessage += chars) += "\" expected";
      } else
	fullMessage = msg;
      throw formatError(is, fullMessage);
    }

    int expect(std::istream& is, const std::vector<const char*>& words) {
      std::vector<const char*> ptrs = words;

      char c;
      is >> std::skipws >> c;
      size_t count = ptrs.size();
      is >> std::noskipws;
      while (is.good() && count > 0) {
	for (size_t i = 0; i != ptrs.size(); ++i) {
	  const char*& ptr = ptrs[i];
	  if (ptr != nullptr) {
	    if (c != *ptr) {
	      ptr = nullptr;
	      --count;
	    } else {
	      ++ptr;
	      if (*ptr == 0)
		return static_cast<int>(i);
	    }
	  }
	}
	is >> c;
      }
      return -1;
    }
  }
}
