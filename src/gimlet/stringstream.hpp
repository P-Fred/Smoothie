#pragma once

#include <cmath>
#include <iostream>

namespace cool {	

  struct skip {
    const char * text;
    skip(const char * text) : text(text) {
    }
  };

  std::istream & operator>> (std::istream & stream, const skip & x);

  class double_istream {
    std::istream &in_;

    double_istream & parse_on_fail (double &x, bool neg);
    
  public:
    double_istream(std::istream &in);
    double_istream & operator >> (double &x);
  };

  struct double_imanip {
    mutable std::istream *in;
    const double_imanip & operator >> (double &x) const;
    std::istream & operator >> (const double_imanip &) const;
  };

  const double_imanip&  operator>>(std::istream &in, const double_imanip &dm);
  
  template <typename Char, typename Traits = std::char_traits<Char>>
    class basic_stringbuf : public std::basic_streambuf<Char, Traits> {
      typedef std::basic_streambuf<Char, Traits> Base;
    
    public:
      typedef typename Base::char_type char_type;
      typedef typename Base::int_type int_type;
      typedef typename Base::traits_type traits_type;
      typedef typename std::basic_string<char_type> string_type;

      const string_type& str() const { return m_str; }

      basic_stringbuf() : Base(), m_str() {
	updatePtrs(true);
      }

      basic_stringbuf(string_type&& str) : m_str{std::move(str)} {
	updatePtrs(true);
      }

    protected:
      string_type m_str;
    
      virtual int_type underflow();
      virtual std::streamsize xsputn(const char_type* s, std::streamsize n);
      virtual int_type overflow(int_type);
    
      void updatePtrs(bool newBuffer);
    
    protected:
      bool is_eof(int_type ch) { return ch == traits_type::eof(); }
    };
  
  template <typename Char, typename Traits>
  void basic_stringbuf<Char, Traits>::updatePtrs(bool newBuffer) {
    char_type* begin = const_cast<char_type*>(m_str.data());
    char_type* end = begin + m_str.size();

    if(newBuffer) 
      this->setg(begin, begin, end);
    else {
      std::streamsize nread = this->gptr() - this->eback();
      this->setg(begin, begin + nread, end);
    }
    this->setp(end, end);
  }
    
  template <typename Char, typename Traits>
  typename basic_stringbuf<Char, Traits>::int_type
  basic_stringbuf<Char, Traits>::underflow() {
    auto current = this->gptr();
    if(current == this->egptr())
      return traits_type::eof();
    else
      return traits_type::to_int_type(*current);
  }
  
  template <typename Char, typename Traits>
  std::streamsize basic_stringbuf<Char, Traits>::xsputn(const char_type* s, std::streamsize n) {
    m_str.append(s, n);
    updatePtrs(false);
    return n;
  }

  template <typename Char, typename Traits>
  typename basic_stringbuf<Char, Traits>::int_type
  basic_stringbuf<Char, Traits>::overflow(int_type ch) {
    if(is_eof(ch)) return traits_type::eof();
    else {
      char_type c = traits_type::to_char_type(ch);
      m_str.push_back(c);
      updatePtrs(false);
      return 1;
    }
  }

  template <typename Char, typename Traits = std::char_traits<Char> >
  class basic_stringstream : public std::basic_iostream<Char, Traits> {
    basic_stringbuf<Char, Traits> m_buf;
  public:
    using string_type = typename basic_stringbuf<Char, Traits>::string_type;

    basic_stringstream() : std::basic_iostream<Char, Traits>(&m_buf) {}
    basic_stringstream(std::string&& str) : std::basic_iostream<Char, Traits>(&m_buf), m_buf(std::move(str)) {}
    const string_type& str() const { return m_buf.str(); }
  };

  using stringstream = basic_stringstream<char>;
}
