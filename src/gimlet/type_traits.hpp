#pragma once

#include <vector>
#include <list>
#include <tuple>
#include <variant>
#include <type_traits>
#include <iterator>

namespace cool {

  template <typename R, typename ... Types> constexpr size_t nbr_arguments(R(*f)(Types ...)) {
    return sizeof...(Types);
  }
  
  template<typename T1, typename T2, typename C = void>
  struct Copier {
    static void set(T1& v1, const T2& v2) {
      v1 = v2;
    }
  };
  
  template<typename T1, typename T2>
  T1& copy(T1& v1, const T2& v2) {
    Copier<T1,T2>::set(v1, v2);
    return v1;
  }
  
  template<typename T>
  struct is_sequence : std::integral_constant<bool, false> {};
  
  template<typename T, typename A>
  struct is_sequence<std::vector<T,A>> : std::integral_constant<bool, true> {};

  template<typename T, typename A>
  struct is_sequence<std::list<T,A>> : std::integral_constant<bool, true> {};

  template<typename T>
  constexpr bool is_sequence_v = is_sequence<T>::value;

  template<typename T>
  struct is_tuple : std::integral_constant<bool, false> {};
  
  template<typename... Args>
  struct is_tuple<std::tuple<Args...>> : std::integral_constant<bool, true> {};

  template<typename T1, typename T2>
  struct is_tuple<std::pair<T1,T2>> : std::integral_constant<bool, true> {};

  template<typename T>
  constexpr bool is_tuple_v = is_tuple<T>::value;
  
  template<typename T1, typename T2>
  struct Copier<T1, T2, std::enable_if_t<is_sequence_v<T1> && is_sequence_v<T2> && (! std::is_same_v<T1,T2>)>>{
    static void set(T1& seq1, const T2& seq2) {
      seq1.clear();
      auto it = std::back_inserter(seq1);
      for(const auto& elt : seq2) {
	auto& v = *it;
	copy(v,elt);
	++it;
      }
    }
  };

  template<typename T1, typename T2, size_t n>
  struct ComponentCopier {
    static void copyComponent(T1& tuple1, const T2& tuple2) {
      copy(std::get<n-1>(tuple1), std::get<n-1>(tuple2));
      ComponentCopier<T1, T2, n-1>::copyComponent(tuple1, tuple2);
    }
  };
    
  template<typename T1, typename T2>
  struct ComponentCopier<T1, T2, 0> {
    static void copyComponent(T1& tuple1, const T2& tuple2) {}
  };
  
  template<typename T1, typename T2>
  struct Copier<T1, T2, std::enable_if_t<is_tuple_v<T1> && is_tuple_v<T2> && (! std::is_same_v<T1,T2>) && (std::tuple_size<T1>::value == std::tuple_size<T2>::value)>> {
    
    static void set(T1& tuple1, const T2& tuple2) {
      ComponentCopier<T1, T2, std::tuple_size<T1>::value>::copyComponent(tuple1, tuple2);
    }
  };


  
  template<typename T1, typename T2>
  struct type_cat {};
  
  template<typename T, typename... Args> struct type_cat<T, std::tuple<Args...>> {
    using type = std::tuple<T, Args...>;
  };
 
  template<typename T, typename... Args> struct type_cat<T, std::variant<Args...>> {
    using type = std::variant<T, Args...>;
  };

  template<typename Value>
  struct has_reserve {
  };
  
  template<typename V, typename A>
  struct has_reserve<std::vector<V,A>> {
    static constexpr bool value = true;
  };

  /*
    Type traits
  */
  
  template<typename T> struct type_traits {
    static const std::string name() { return typeid(T).name(); }
  };

#define ELEMENTARY_TYPE_TRAITS(TYPE)			\
  template<> struct type_traits<TYPE> {			\
    static const std::string name() { return #TYPE; }	\
  }
  
  ELEMENTARY_TYPE_TRAITS(void);
  ELEMENTARY_TYPE_TRAITS(bool);
  ELEMENTARY_TYPE_TRAITS(char);
  ELEMENTARY_TYPE_TRAITS(unsigned char);
  ELEMENTARY_TYPE_TRAITS(short);
  ELEMENTARY_TYPE_TRAITS(unsigned short);
  ELEMENTARY_TYPE_TRAITS(long);
  ELEMENTARY_TYPE_TRAITS(unsigned long);
  ELEMENTARY_TYPE_TRAITS(long long);
  ELEMENTARY_TYPE_TRAITS(unsigned long long);
  ELEMENTARY_TYPE_TRAITS(float);
  ELEMENTARY_TYPE_TRAITS(double);
  
  template<typename T> struct type_traits<std::vector<T>> {
    static const std::string name() {
      return std::string("std::vector<") + type_traits<T>::name() + ">";
    }
  };

  template<typename T> struct type_traits<std::list<T>> {
    static const std::string name() {
      return std::string("std::list<") + type_traits<T>::name() + ">";
    }
  };

  template<typename... Args> struct type_list {
  };
  template<typename T1, typename T2, typename... Args> struct type_list<T1, T2, Args...> {
    static const std::string name() {
      return type_traits<T1>::name() + ", " + type_list<T2, Args...>::name();
    }
  };
  template<typename T> struct type_list<T> {
    static const std::string name() {
      return type_traits<T>::name();
    }
  };
  template<> struct type_list<> {
    static const std::string name() {
      return "";
    }
  };  
    
  template<typename... Args> struct type_traits<std::tuple<Args...>> {
    static const std::string name() {
      return std::string("std::tuple<") + type_list<Args...>::name() + ">";
    }
  };

  template<typename... Args> struct type_traits<std::variant<Args...>> {
    static const std::string name() {
      return std::string("std::variant<") + type_list<Args...>::name() + ">";
    }
  };
}
