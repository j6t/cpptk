//
// Copyright (C) 2004-2006, Maciej Sobczak
//
// Permission to copy, use, modify, sell and distribute this software
// is granted provided this copyright notice appears in all copies.
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//

#ifndef CPPTKBASE_H_INCLUDED
#define CPPTKBASE_H_INCLUDED

# if defined _MSC_VER
#   if (_MSC_VER >= 1300)
#     pragma warning (disable : 4127 4511 4512 4701)
#   endif
# endif

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <sstream>
#include <vector>
#include <boost/lexical_cast.hpp>
#include <iosfwd>

namespace Tk
{

// exception class used for reporting all Tk errors

class TkError : public std::runtime_error
{
public:
     explicit TkError(std::string const &msg)
          : std::runtime_error(msg)
     {
     }
};

// for functions returning point and box (or windows) coordinates
struct Point
{
     Point(int a, int b) : x(a), y(b) {}
     int x, y;
};

struct Box
{
     Box(int a, int b, int c, int d) : x1(a), y1(b), x2(c), y2(d) {}
     int x1, y1, x2, y2;
};

namespace details
{

// The Result class gathers everything on its road while
// it travels the Tk expression

// returns the length of the result list
int getResultLen();

// retrieves the result list element at the given index
template <typename T> T getResultElem(int indx);

// available specializations
template <> int         getResultElem<int>(int indx);
template <> double      getResultElem<double>(int indx);
template <> std::string getResultElem<std::string>(int indx);


// The Result object is a result of executing Tk expression.

class ResultBase
{
public:
     ResultBase() = default;
     explicit ResultBase(std::string const &cmd) : cmd_(cmd) {}

     std::string toString() const;
     int toInt() const;
     double toDouble() const;
     Tk::Point toPoint() const;
     Tk::Box toBox() const;

     template <typename T1, typename T2>
     std::pair<T1, T2> toPair() const
     {
          auto ret = toString();
          if (ret.empty())
          {
               return std::make_pair(T1(), T2());
          }

          int len = getResultLen();
          if (len < 2)
          {
               throw TkError("Cannot convert the result list into pair\n");
          }

          return std::make_pair(getResultElem<T1>(0), getResultElem<T2>(1));
     }

     template <typename T>
     std::vector<T> toVector() const
     {
          auto ret = toString();
          if (ret.empty())
          {
               return std::vector<T>();
          }

          int len = getResultLen();
          std::vector<T> v;
          v.reserve(len);
          for(int i = 0; i != len; ++i)
          {
               v.push_back(getResultElem<T>(i));
          }

          return v;
     }

protected:
     void invokeOnce();

     std::string cmd_;
};

class Expr;

template<class DERIVED, class EXPR = Expr>
class Result : public ResultBase
{
public:
     using ResultBase::ResultBase;

     operator std::string() && {
          static_cast<DERIVED&>(*this).invokeOnce();
          return toString();
     }
     operator int() && {
          static_cast<DERIVED&>(*this).invokeOnce();
          return toInt();
     }
     operator double() && {
          static_cast<DERIVED&>(*this).invokeOnce();
          return toDouble();
     }
     operator Tk::Point() && {
          static_cast<DERIVED&>(*this).invokeOnce();
          return toPoint();
     }
     operator Tk::Box() && {
          static_cast<DERIVED&>(*this).invokeOnce();
          return toBox();
     }
     template <typename T1, typename T2>
     operator std::pair<T1, T2>() && {
          static_cast<DERIVED&>(*this).invokeOnce();
          return toPair<T1, T2>();
     }
     template <typename T>
     operator std::vector<T>() && {
          static_cast<DERIVED&>(*this).invokeOnce();
          return toVector<T>();
     }

     using ResultBase::cmd_;    // needed so that friends can access it

     friend DERIVED&& operator-(DERIVED &&lhs, EXPR &&rhs)
     {
          lhs.cmd_ += std::exchange(rhs.cmd_, {});

          return std::move(lhs);
     }

     friend DERIVED&& operator<<(std::string const &w, DERIVED &&rhs)
     {
          rhs.cmd_ = w + " " + move(rhs.cmd_);

          return std::move(rhs);
     }
};

// The intent is that the Expr object is a temporary.
// There may be many Expr objects flying around and
// accumulating state in a single Command object.

class Expr : public Result<Expr>
{
public:
     explicit Expr(std::string const &str) : Result(str) {}
     Expr(Expr &&) = default;
     ~Expr() noexcept(false);
     Expr& operator=(Expr &&) = default;

private:
     std::string str_;
};

class ExprWithPostfix : public Result<ExprWithPostfix>
{
public:
     ExprWithPostfix(std::string const &str, std::string const &postfix);
     ExprWithPostfix(ExprWithPostfix &&) = default;
     ~ExprWithPostfix() noexcept(false);
     ExprWithPostfix& operator=(ExprWithPostfix &&) = default;

     void invokeOnce();

private:
     std::string postfix_;
};

// The Params is used to encapsulate the list of parameters
// that will be passed to the callback functions.
// It is needed to isolate this header from Tcl/Tk headers.

class Params
{
public:
     Params(int argc, void *objv) : argc_(argc), objv_(objv) {}
     
     template <typename T> T get(int argno) const;

private:
     int argc_;
     void *objv_;     
};

// available specializations for Params::get
template <> int Params::get<int>(int argno) const;
template <> std::string Params::get<std::string>(int argno) const;
template <> std::vector<std::string> Params::get<std::vector<std::string>>(int argno) const;

// The Completion enumeration can be used by callback functions
// to report a particular completion to the Tcl interpreter.

enum class Completion : int {
     Ok         = 0,    // TCL_OK
     Error      = 1,    // TCL_ERROR
     Return     = 2,    // TCL_RETURN
     Break      = 3,    // TCL_BREAK
     Continue   = 4,    // TCL_CONTINUE
};

// The CallbackBase is used to store callback handlers
// in the polymorphic map

class CallbackBase
{
public:
     virtual ~CallbackBase() {}
     virtual Completion invoke(Params const &) = 0;
};

std::string addCallback(std::shared_ptr<CallbackBase> cb);

// helpers for setting result in the interpreter
void setResult(bool b);
void setResult(long i);
void setResult(double d);
void setResult(std::string const &s);

// The Dispatch class is used to execute the callback functor
// and to capture their results.

template <typename R, class... T>
struct Dispatch
{
     static Completion doDispatch(std::function<R(T...)> &f, T const &... t)
     {
          R result = f(t...);
          setResult(result);
          return Completion::Ok;
     }
};

// partial specialization for functors that return nothing
template <class... T>
struct Dispatch<void, T...>
{
     static Completion doDispatch(std::function<void(T...)> &f, T const &... t)
     {
          f(t...);
          return Completion::Ok;
     }
};

// partial specialization for functors that return a particular completion code
template <class... T>
struct Dispatch<Completion, T...>
{
     static Completion doDispatch(std::function<Completion(T...)> &f, T const &... t)
     {
          return f(t...);
     }
};

// A helper that removes a functor overload if type Functor
// cannot be used as a functor with signature F
template <class Functor, class R, class... Args>
constexpr bool IsFunctor() {
     if constexpr (std::is_constructible_v<std::function<R(Args...)>, Functor>)
          return std::is_same_v<R, decltype(std::declval<Functor>()(std::declval<Args const&>()...))>;
     else
          return false;
}
template <class F, class Functor, class T>
struct RequireFunctor {};
template <class Functor, class T, class R, class... Args>
struct RequireFunctor<R(Args...), Functor, T> : std::enable_if<IsFunctor<Functor, R, Args...>(), T> {};

// The Callback class is used as an envelope for the actual
// callback object

template <class...> class Callback;	// too generic, remains undefined

template <class R, class... T>
class Callback<R(T...)> : public CallbackBase
{
public:
     template<class FUNC>
     Callback(FUNC &&f) : f_(std::forward<FUNC>(f)) {}

     virtual Completion invoke(Params const &p)
     {
          return doInvoke(p, std::index_sequence_for<T...>{});
     }

private:
     template<std::size_t... I>
     Completion doInvoke(Params const &p, std::index_sequence<I...>)
     {
          return Dispatch<R, T...>::doDispatch(f_, p.template get<T>(I+1)...);
     }

     std::function<R(T...)> f_;
};

std::string addLinkVar(int &i);
std::string addLinkVar(double &d);
std::string addLinkVar(std::string &s);
void deleteLinkVar(int &i);
void deleteLinkVar(double &d);
void deleteLinkVar(std::string &s);

// helper functions for later definitions

template <typename T>
inline std::string toString(T const &t)
{ return boost::lexical_cast<std::string>(t); }

inline std::string toString(std::string const &str) { return str; }
inline std::string toString(char const *str) { return str; }

// this function is used to quote quotation marks in string values'
// in later version, it will not be needed
std::string quote(std::string const &s);

class BasicToken
{
public:
     BasicToken(std::string const &n) : name_(n) {}
     operator std::string() const { return name_; }
     
protected:
     std::string name_;
};

std::ostream & operator<<(std::ostream &os, BasicToken const &token);

// this class allows to use the same option name when configuring and
// querying, for example:
// 1. button(".b") -text("Hello") -foreground("blue");
// 2. string color(".b" << cget(foreground));
class Option : public details::BasicToken
{
public:
     explicit Option(char const *name, bool quote = false)
          : BasicToken(name), quote_(quote) {}
     
     template <typename T>
     Expr operator()(T const &t) const
     {
          std::string str(" -");
          str += name_;  str += " ";
          str += (quote_ ? "\"" : "");
          str += toString(t);
          str += (quote_ ? "\"" : "");
          return Expr(str);
     }

     Expr operator()() const
     {
          std::string str(" -");
          str += name_;
          return Expr(str);
     }

private:
     bool quote_;
};

// these classes are used for substitution specification

template <typename T>
class SubstAttr
{
public:
     SubstAttr(std::string const &spec) : spec_(spec) {}
     
     std::string get() const { return spec_; }

private:
     std::string spec_;
};

template <typename T>
class EventAttr : public SubstAttr<T>
{
public:
     typedef T attrType;
     
     EventAttr(std::string const &spec) : SubstAttr<T>(spec) {}
};

template <typename T>
class ValidateAttr : public SubstAttr<T>
{
public:
     typedef T validType;
     
     ValidateAttr(std::string const &spec) : SubstAttr<T>(spec) {}
};

class VarProxy
{
public:
     VarProxy(std::string name);

     void operator=(int v);
     void operator=(double v);
     void operator=(std::string const& str);
     operator int() const;
     operator double() const;
     operator std::string() const;

private:
     std::string name_;
};

} // namespace details

// basic operations for Tk expressions

// operator- and operator<< are declared as friend functions of details::Expr

// for defining callbacks (only function pointers)
template <class R, class... T> std::string callback(R(*f)(T...))
{
     return details::addCallback(
          std::shared_ptr<details::CallbackBase>(
               new details::Callback<R(T...)>(f)));
}

// for deleting callbacks
void deleteCallback(std::string const &name);

// RAII handle for callback (calls deleteCallback in its destructor)
class CallbackHandle
{
public:
     explicit CallbackHandle(std::string const &name);
     ~CallbackHandle();
     
     std::string const & get() const { return name_; }
     
private:
     std::string name_;
};

// for linking variable
template <typename T> std::string linkVar(T &t)
{
     return details::addLinkVar(t);
}

// for unlinking variable
template <typename T> void unLinkVar(T &t) { details::deleteLinkVar(t); }

// RAII handle for linking variables (calls unLinkVar in its destructor)
template <typename T>
class LinkHandle
{
public:
     explicit LinkHandle(T &t) :t_(t) { var_ = details::addLinkVar(t); }
     ~LinkHandle() { details::deleteLinkVar(t_); }
     
     std::string const & get() const { return var_; }
     
private:
     T &t_;
     std::string var_;
};

// for brute-force evaluation of simple scripts
details::Expr eval(std::string const &str);

inline namespace literals {
     details::Expr operator"" _tcl(const char *str, std::size_t len);
     std::string operator"" _tcls(const char *str, std::size_t len);
     int operator"" _tcli(const char *str, std::size_t len);
     double operator"" _tcld(const char *str, std::size_t len);

     details::VarProxy operator"" _tclv(const char *str, std::size_t len);
     std::string operator"" _tclvs(const char *str, std::size_t len);
     int operator"" _tclvi(const char *str, std::size_t len);
     double operator"" _tclvd(const char *str, std::size_t len);
} // namespace literals

constexpr details::Completion Return = details::Completion::Return;
constexpr details::Completion Break = details::Completion::Break;
constexpr details::Completion Continue = details::Completion::Continue;

// for initializing Tcl environment
void init(const char *argv0);

// for falling into the event loop
void runEventLoop();

// for setting command output stream
void setDumpStream(std::ostream &os);

} // namespace Tk

#endif // CPPTKBASE_H_INCLUDED
