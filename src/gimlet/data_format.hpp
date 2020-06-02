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
#include <list>
#include <map>
#include <unordered_map>
#include <utility>
#include <array>
#include <tuple>
#include <variant>
#include <functional>
#include <memory>
#include <type_traits>

#include <gimlet/type_traits.hpp>

namespace gimlet {

  /**
     List of available data formats
  */
  namespace format {
  }
  template<typename F> struct basic_string {}; //! String of characters
  using string = gimlet::basic_string<char>;
  template<typename F> struct list {}; //! List of homogeneous elements
  template<typename KF, typename VF> struct map {}; //! Associative map of keys to values
  template<typename F> struct set {}; //! Sets of homogeneous elements
  template<typename... Args> struct tuple {}; //! Tuple of heterogeneous elements
  template<typename... Args> struct variant {}; //! Variant of heterogeneous elements
  template<typename F> struct buffer {}; //! Intermediate buffer
  template<typename... Args> struct sequence {}; //! Sequence of heterogeneous elements
  template<typename F> struct flow {}; //! Flow of homogeneous elements

  struct end_of_flow {};
  
  /**
     Equivalence relations between data formats and real data types
  */
  template<typename D1, typename D2>
  struct is_equiv : std::integral_constant<bool, false> {};

  template<typename D>
  struct is_equiv<D,D> : std::integral_constant<bool, true> {};
  
  template<typename D1, typename D2>
  struct is_equivalent : std::integral_constant<bool, is_equiv<D1,D2>::value || is_equiv<D2,D1>::value> {};

  /**
     Subsumption relations between data formats and real data types
  */

  /**
     Numeric types
  */
  template<int v, bool s> struct resolution_constant {
    static constexpr int size = v;
    static constexpr bool sign = s;
  };
  template<typename T> struct resolution : resolution_constant<sizeof(T), std::is_signed<T>::value> {};
  template<> struct resolution<bool> : resolution_constant<0, false> {};

  
  template<typename D1, typename D2, typename T = void>
  struct is_more_specific : std::integral_constant<bool, is_equivalent<D1,D2>::value> {};

  template<typename Data, typename Format>
  struct is_read_compatible : std::integral_constant<bool, is_more_specific<Data, Format>::value> {};

  template<typename Data, typename Format>
  constexpr bool is_read_compatible_v = is_read_compatible<Data, Format>::value;
  
  template<typename Data, typename Format>
  struct is_write_compatible : std::integral_constant<bool, is_more_specific<Format, Data>::value> {};

  template<typename Data, typename Format>
  constexpr bool is_write_compatible_v = is_write_compatible<Data, Format>::value;
  
  template<typename T1, typename T2>
  struct is_more_specific<T1, T2, std::enable_if_t<std::is_integral<T1>::value && std::is_integral<T2>::value>> : std::integral_constant<bool, (resolution<T1>::size > resolution<T2>::size && (resolution<T1>::sign || !resolution<T2>::sign)) || (resolution<T1>::size == resolution<T2>::size && (resolution<T1>::sign == resolution<T2>::sign))> {};
  
  template<>
  struct is_more_specific<double, float> : std::integral_constant<bool, true> {};

  /**
     Strings
  */
 
  template<typename C1, typename C2>
  struct is_more_specific<std::basic_string<C1>, basic_string<C2>> : std::integral_constant<bool, is_more_specific<C1, C2>::value> {};

  template<typename C1, typename C2>
  struct is_more_specific<gimlet::basic_string<C1>, std::basic_string<C2>> : std::integral_constant<bool, is_more_specific<C1, C2>::value> {};  

  /**
     Buffers
  */

  template<typename D1, typename D2>
  struct is_more_specific<buffer<D1>, D2> : std::integral_constant<bool, is_more_specific<D1, D2>::value> {};

  template<typename D1, typename D2>
  struct is_more_specific<D1, buffer<D2>> : std::integral_constant<bool, is_more_specific<D1, D2>::value> {};
  
  template<typename D>
  struct is_more_specific<std::string, buffer<D>> : std::integral_constant<bool, true> {};

  template<typename D>
  struct is_more_specific<buffer<D>, std::string> : std::integral_constant<bool, true> {};
  
  /**
     Lists
  */

  template<typename D1, typename D2>
  struct is_more_specific<std::vector<D1>, gimlet::list<D2>> : std::integral_constant<bool, is_more_specific<D1, D2>::value> {};

  template<typename D1, typename D2>
  struct is_more_specific<gimlet::list<D1>, std::vector<D2>> : std::integral_constant<bool, is_more_specific<D1, D2>::value> {};

  template<typename D1, typename D2>
  struct is_more_specific<std::list<D1>, gimlet::list<D2>> : std::integral_constant<bool, is_more_specific<D1, D2>::value> {};

  template<typename D1, typename D2>
  struct is_more_specific<gimlet::list<D1>, std::list<D2>> : std::integral_constant<bool, is_more_specific<D1, D2>::value> {};

  /**
     Maps
  */

  template<typename K1, typename V1, typename K2, typename V2>
  struct is_more_specific<std::map<K1,V1>, gimlet::map<K2,V2>> : std::integral_constant<bool, is_more_specific<K1, K2>::value && is_more_specific<V1, V2>::value> {};

  template<typename K1, typename V1, typename K2, typename V2>
  struct is_more_specific<gimlet::map<K1,V1>, std::map<K2,V2>> : std::integral_constant<bool, is_more_specific<K1, K2>::value && is_more_specific<V1, V2>::value> {};

  template<typename K1, typename V1, typename K2, typename V2>
  struct is_more_specific<std::unordered_map<K1,V1>, gimlet::map<K2,V2>> : std::integral_constant<bool, is_more_specific<K1, K2>::value && is_more_specific<V1, V2>::value> {};

  template<typename K1, typename V1, typename K2, typename V2>
  struct is_more_specific<gimlet::map<K1,V1>, std::unordered_map<K2,V2>> : std::integral_constant<bool, is_more_specific<K1, K2>::value && is_more_specific<V1, V2>::value> {};

  
  /**
     Tuples
  */
  template<>
  struct is_equiv<std::tuple<>, gimlet::tuple<>> : std::integral_constant<bool, true> {};
  
  template<typename D1, typename... Args1, typename D2, typename... Args2>
  struct is_more_specific<std::tuple<D1, Args1...>, std::tuple<D2, Args2...>> : std::integral_constant<bool, is_more_specific<D1, D2>::value && is_more_specific<std::tuple<Args1...>, std::tuple<Args2...>>::value > {};

  template<typename... Args, typename D>
  struct is_more_specific<gimlet::tuple<Args...>, D> : std::integral_constant<bool, is_more_specific<std::tuple<Args...>, D>::value > {};

  template<typename... Args, typename D>
  struct is_more_specific<D, gimlet::tuple<Args...>> : std::integral_constant<bool, is_more_specific<D, std::tuple<Args...>>::value > {};
  
  template<typename A1, typename B1, typename A2, typename B2>
  struct is_more_specific<std::pair<A1, B1>, std::tuple<A2, B2>> : std::integral_constant<bool, is_more_specific<A1, A2>::value && is_more_specific<B1,B2>::value> {};

  template<typename A1, typename B1, typename A2, typename B2>
  struct is_more_specific<std::tuple<A1, B1>, std::pair<A2, B2>> : std::integral_constant<bool, is_more_specific<A1, A2>::value && is_more_specific<B1,B2>::value> {};
  
  /**
     Variants
  */
  
  using variant_index_type = unsigned char;
  
  template<typename D1, typename... Args1, typename D2, typename... Args2>
  struct is_more_specific<std::variant<D1, Args1...>, std::variant<D2, Args2...>> : std::integral_constant<bool, is_more_specific<D1, D2>::value && is_more_specific<std::variant<Args1...>, gimlet::variant<Args2...>>::value > {};

  template<typename... Args, typename D>
  struct is_more_specific<gimlet::variant<Args...>, D> : std::integral_constant<bool, is_more_specific<std::variant<Args...>, D>::value > {};

  template<typename... Args, typename D>
  struct is_more_specific<D, gimlet::variant<Args...>> : std::integral_constant<bool, is_more_specific<D, std::variant<Args...>>::value > {};  

   /**
     Variables
  */

  /* See data_format_parser.hpp */

 
  /**
     Sequences
  */
  template<typename D1, typename... D2, typename D3>
  struct is_more_specific<sequence<D1, D2...>, D3> : std::integral_constant<bool, is_more_specific<D1, D3>::value || is_more_specific<sequence<D2...>, D3>::value> {};
  
  template<typename D3>
  struct is_more_specific<sequence<>, D3> : std::integral_constant<bool, false> {};

  template<typename D1, typename... D2, typename D3>
  struct is_more_specific<D1, sequence<D3, D2...>> : std::integral_constant<bool, is_more_specific<D1, D3>::value || is_more_specific<D1, sequence<D2...>>::value> {};
  
  template<typename D1>
  struct is_more_specific<D1, sequence<>> : std::integral_constant<bool, false> {};
  
  /**
     Streams
  */
  template<typename D1, typename D2>
  struct is_more_specific<flow<D1>, D2> : std::integral_constant<bool, is_more_specific<D1, D2>::value> {};
  
  template<typename D1, typename D2>
  struct is_more_specific<D1, flow<D2>> : std::integral_constant<bool, is_more_specific<D1, D2>::value> {};



   /**
    Extraction of data format from data type
  */
  template<typename Value> struct data_format {
    using type = Value;
  };

  template<typename Value> using data_format_t = typename data_format<Value>::type;
  
  // template<typename Value, typename... Values>
  // using data_formats_t = typename cool::type_cat<data_format_t<Value>, data_formats_t<Values>>::type;
  // template<> using data_formats_t = std::tuple<>;

  
  template<typename V> struct data_format<std::basic_string<V>> {
    using type = gimlet::basic_string<data_format_t<V>>;
  };
  
  template<typename V> struct data_format<gimlet::list<V>> {
    using type = gimlet::list<data_format_t<V>>;
  };
  template<typename V, typename A> struct data_format<std::vector<V,A>> {
    using type = gimlet::list<data_format_t<V>>;
  };
  template<typename V, typename A> struct data_format<std::list<V,A>> {
    using type = gimlet::list<data_format_t<V>>;
  };
  
  template<typename K, typename V> struct data_format<gimlet::map<K, V>> {
    using type = gimlet::map<data_format_t<K>, data_format_t<V>>;
  };
  template<typename K, typename V, typename C, typename A> struct data_format<std::map<K, V, C, A>> {
    using type = gimlet::map<data_format_t<K>, data_format_t<V>>;
  };
  template<typename K, typename V, typename H, typename C, typename A> struct data_format<std::unordered_map<K, V, H, C, A>> {
    using type = gimlet::map<data_format_t<K>, data_format_t<V>>;
  };
  
  template<typename V> struct data_format<gimlet::flow<V>> {
    using type = gimlet::flow<data_format_t<V>>;
  };


  
  template<typename T> struct gimlet_tuple_from_tuple {};
  template<typename... Args> struct gimlet_tuple_from_tuple<std::tuple<Args...>> {
    using type = gimlet::tuple<Args...>;
  };
  template<typename T> struct data_format_from_tuple {};
  template<typename V, typename... Args> struct data_format_from_tuple<std::tuple<V, Args...>> {
    using type = typename cool::type_cat<data_format_t<V>, typename data_format_from_tuple<std::tuple<Args...>>::type>::type;
  };
  template<> struct data_format_from_tuple<std::tuple<>> {
    using type = std::tuple<>;
  };  
  
  template<typename... Args> struct data_format<std::tuple<Args...>> {
    using type = typename gimlet_tuple_from_tuple<typename data_format_from_tuple<std::tuple<Args...>>::type>::type;
  };
  
  template<typename... Args> struct data_format<gimlet::tuple<Args...>> {
    using type = typename gimlet_tuple_from_tuple<typename data_format_from_tuple<std::tuple<Args...>>::type>::type;
  };

  template<typename T1, typename T2> struct data_format<std::pair<T1, T2>> {
    using type = gimlet::tuple<data_format_t<T1>, data_format_t<T2>>;
  };  

  template<typename T> struct gimlet_variant_from_tuple {};
  template<typename... Args> struct gimlet_variant_from_tuple<std::tuple<Args...>> {
    using type = gimlet::variant<Args...>;
  };
  
  template<typename... Args> struct data_format<std::variant<Args...>> {
    using type = typename gimlet_variant_from_tuple<typename data_format_from_tuple<std::tuple<Args...>>::type>::type;
  };
  
  template<typename... Args> struct data_format<gimlet::variant<Args...>> {
    using type = typename gimlet_variant_from_tuple<typename data_format_from_tuple<std::tuple<Args...>>::type>::type;
  };

  template<typename T> struct gimlet_sequence_from_tuple {};
  template<typename... Args> struct gimlet_sequence_from_tuple<std::tuple<Args...>> {
    using type = gimlet::sequence<Args...>;
  };
  
  template<typename... Args> struct data_format<gimlet::sequence<Args...>> {
    using type = typename gimlet_sequence_from_tuple<typename data_format_from_tuple<std::tuple<Args...>>::type>::type;
  }; 
  
  /**
     Data format traits
  */

  template<typename DataFormat> struct data_format_traits {
    static_assert(true, "Unsupported format");
  };

  template<typename Value> const std::string data_format_name() {
    return data_format_traits<data_format_t<Value>>::name();
  }
  
#define CREATE_DATA_FORMAT_TRAITS(TYPE_NAME)			\
  template<> struct data_format_traits<TYPE_NAME> {		\
    using default_value_type = TYPE_NAME;			\
    static const std::string name() { return #TYPE_NAME; }	\
  };

  CREATE_DATA_FORMAT_TRAITS(char)
  CREATE_DATA_FORMAT_TRAITS(unsigned char)
  CREATE_DATA_FORMAT_TRAITS(short)
  CREATE_DATA_FORMAT_TRAITS(unsigned short)
  CREATE_DATA_FORMAT_TRAITS(int)
  CREATE_DATA_FORMAT_TRAITS(unsigned int)
  CREATE_DATA_FORMAT_TRAITS(long)
  CREATE_DATA_FORMAT_TRAITS(unsigned long)
  CREATE_DATA_FORMAT_TRAITS(bool)
  CREATE_DATA_FORMAT_TRAITS(float)
  CREATE_DATA_FORMAT_TRAITS(double)
  
  template<typename Char>
  class data_format_traits<gimlet::basic_string<Char>> {
    using char_value_type = typename data_format_traits<Char>::default_value_type;
  public:
    using default_value_type = std::basic_string<char_value_type>;
    static const std::string name() {
      return std::string("string<") + data_format_traits<Char>::name() + ">";
    }
  };
  
  template<typename Value> struct data_format_traits<gimlet::list<Value>> {
    using element_default_value_type = typename data_format_traits<Value>::default_value_type;
  public:
    using default_value_type = std::vector<element_default_value_type>;
    static const std::string name() {
      return std::string("list<") + data_format_traits<Value>::name() + ">";
    }
  };

  template<typename Key, typename Value> struct data_format_traits<gimlet::map<Key, Value>> {
    using key_default_value_type = typename data_format_traits<Key>::default_value_type;
    using value_default_value_type = typename data_format_traits<Value>::default_value_type;
  public:
    using default_value_type = std::unordered_map<key_default_value_type, value_default_value_type>;
    static const std::string name() {
      return std::string("map<") + data_format_traits<Key>::name() + "," + data_format_traits<Value>::name() + ">";
    }
  };
  
  template<typename T>
  struct data_format_traits<buffer<T>> {
    using default_value_type = typename data_format_traits<T>::default_value_type;
    static const std::string name() {
      return std::string("buffer<") + data_format_traits<T>::name() + ">";
    }
  };  

  template<typename Tuple, int i, int c>
  class data_format_traits_from_tuple {
    using tuple_type = Tuple;
    using head_type = typename std::tuple_element<i, tuple_type>::type;
    using head_default_value_type = typename data_format_traits<head_type>::default_value_type;
    using tail_default_value_type = typename data_format_traits_from_tuple<Tuple, i + 1, c - 1>::default_value_type;
  public:
    using default_value_type = typename cool::type_cat<head_default_value_type, tail_default_value_type>::type;
    static const std::string name() {
      if(c > 1)
	return data_format_traits<head_type>::name() + "," +  data_format_traits_from_tuple<Tuple, i + 1, c - 1>::name();
      else
	return data_format_traits<head_type>::name();	  
    }
  };

  template<typename Tuple, int i>
  struct data_format_traits_from_tuple<Tuple, i, 0> {
    using default_value_type = std::tuple<>;
    static const std::string name() {
      return "";
    }
  };
    
  template<typename ... Args> class data_format_traits<gimlet::tuple<Args...>> {
    using tuple_type = std::tuple<Args...>;
  public:
    using default_value_type = typename data_format_traits_from_tuple<tuple_type, 0, std::tuple_size<tuple_type>::value>::default_value_type;
    static const std::string name() {
      return std::string("tuple<") + data_format_traits_from_tuple<tuple_type, 0, std::tuple_size<tuple_type>::value>::name() + ">";
    }       
  };

  template<typename Variant, int i, int c>
  class data_format_traits_from_variant {
    using variant_type = Variant;
    using head_type = std::variant_alternative_t<i, variant_type>;
    using head_default_value_type = typename data_format_traits<head_type>::default_value_type;
    using tail_default_value_type = typename data_format_traits_from_variant<Variant, i + 1, c - 1>::default_value_type;
  public:
    using default_value_type = typename cool::type_cat<head_default_value_type, tail_default_value_type>::type;
    static const std::string name() {
      return data_format_traits<head_type>::name() + "," +  data_format_traits_from_variant<Variant, i + 1, c - 1>::name();
    }
  };

  template<typename Variant, int i>
  class data_format_traits_from_variant<Variant, i, 1> {
    using variant_type = Variant;
    using head_type = std::variant_alternative_t<i, variant_type>;
    using head_default_value_type = typename data_format_traits<head_type>::default_value_type;
  public:    
    using default_value_type = std::variant<head_default_value_type>;
    static const std::string name() {
      return data_format_traits<head_type>::name();	  
    }    
  };
    
  template<typename ... Args> class data_format_traits<gimlet::variant<Args...>> {
    using variant_type = std::variant<Args...>;
  public:
    using default_value_type = typename data_format_traits_from_variant<variant_type, 0, std::variant_size<variant_type>::value>::default_value_type;
    static const std::string name() {
      return std::string("variant<") + data_format_traits_from_variant<variant_type, 0, std::variant_size<variant_type>::value>::name() + ">";
    } 
  };


  template<typename T> struct is_a_flow_format {
    static constexpr bool value = false;
  };
  template<typename T> struct is_a_flow_format<flow<T>> {
    static constexpr bool value = true;
  };
  template<typename T> struct get_data_format_from_flow {
  };
  template<typename T> struct get_data_format_from_flow<flow<T>> {
    using type = typename data_format<T>::type;
  };
  
  template<typename Tuple, typename T = void> struct get_default_value_type {
  };
  
  template<typename Arg1, typename Arg2, typename... Args> struct get_default_value_type<std::tuple<Arg1, Arg2, Args...>, std::enable_if_t<is_a_flow_format<Arg1>::value>> {
    using type = typename data_format_traits<typename get_data_format_from_flow<Arg1>::type>::default_value_type;
  };
  
  template<typename Arg1, typename Arg2, typename... Args> struct get_default_value_type<std::tuple<Arg1, Arg2, Args...>, std::enable_if_t<! is_a_flow_format<Arg1>::value>> {
    using type = typename get_default_value_type<std::tuple<Arg2, Args...>>::type;
  };
  
  template<typename Arg> struct get_default_value_type<std::tuple<Arg>> {
    using type = typename data_format_traits<typename data_format<Arg>::type>::default_value_type;
  };
  
  template<typename ... Args> class data_format_traits<sequence<Args...>> {
    using tuple_type = std::tuple<Args...>;
  public:
    using default_value_type = typename get_default_value_type<tuple_type>::type;
    static const std::string name() {
      return std::string("sequence<") + data_format_traits_from_tuple<tuple_type, 0, std::tuple_size<tuple_type>::value>::name() + ">";
    }
  }; 

  template<typename Value> struct data_format_traits<flow<Value>> {
    using element_default_value_type = typename data_format_traits<Value>::default_value_type;
  public:
    using default_value_type = element_default_value_type;
    static const std::string name() {
      return std::string("flow<") + data_format_traits<Value>::name() + ">";
    }
  };
        

}
