//
// Copyright (C) 2004-2006, Maciej Sobczak
// Copyright (C) 2017, Alexander Galanin
//
// Permission to copy, use, modify, sell and distribute this software
// is granted provided this copyright notice appears in all copies.
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//

#include "cpptkbase.h"
#include <tcl.h>
#include <tk.h>
#include <map>
#include <ostream>
#include <iostream>
#include <sstream>

using namespace Tk;
using namespace Tk::details;
using boost::lexical_cast;
using namespace std;

namespace { // anonymous

class Interp
{
public:
     Interp()
     {
          interp_ = Tcl_CreateInterp();
	
          int cc = Tcl_Init(interp_);
          if (cc != TCL_OK)
          {
               throw TkError(Tcl_GetStringResult(interp_));
          }

          cc = Tk_Init(interp_);
          if (cc != TCL_OK)
          {
               throw TkError(Tcl_GetStringResult(interp_));
          }
          
          cc = Tcl_Eval(interp_, "namespace eval CppTk {}");
          if (cc != TCL_OK)
          {
               throw TkError(Tcl_GetStringResult(interp_));
          }
     }
     
     ~Interp()
     {
          // GUI programs are supposed to exit by calling "exit"
          // then - explicit delete here is harmful
          //Tcl_DeleteInterp(interp_);
     }
     
     Tcl_Interp * get() const { return interp_; }

private:
     Tcl_Interp * interp_;
};

// lazy-initialization of Tcl interpreter
Tcl_Interp * getInterp()
{
     static Interp interp;
     return interp.get();
}

#ifdef CPPTK_DUMP_COMMANDS
// output stream for dumping Tk commands
// (useful for automated testing)
static ostream *dumpstream = &cerr;
#endif // CPPTK_DUMP_COMMANDS

void do_eval(string const &str)
{
#ifdef CPPTK_DUMP_COMMANDS
     *dumpstream << str << '\n';
#endif // CPPTK_DUMP_COMMANDS

#ifndef CPPTK_DONT_EVALUATE
     int cc = Tcl_Eval(getInterp(), str.c_str());
     if (cc != TCL_OK)
     {
          throw TkError(Tcl_GetStringResult(getInterp()));
     }
#endif
}

// map for callbacks
typedef map<int, shared_ptr<CallbackBase> > CallbacksMap;
CallbacksMap callbacks;

// callback id
int callbackId = 0;

char const *callbackPrefix = "CppTk::callback";

typedef map<int *,    string> IntLinks;
typedef map<double *, string> DoubleLinks;
typedef map<string *, string> StringLinks;

IntLinks intLinks;
DoubleLinks doubleLinks;
StringLinks stringLinks;

typedef map<string *, char *> StringLinkBuffers;
StringLinkBuffers stringLinkBuffers;

int linkId = 0;

char const *linkVarPrefix = "CppTk::variable";

// this function refreshes Tcl variables from C++ variables
void linkCpptoTcl()
{
     // synchronize C++ variables with Tcl variables
     // it is enough to refresh string buffers and update links
     
     // refresh string buffers
     for (StringLinks::iterator it = stringLinks.begin();
          it != stringLinks.end(); ++it)
     {
          string *ps = it->first; // pointer to C++ string (original value)

          StringLinkBuffers::iterator itb = stringLinkBuffers.find(ps);
          char *&pb = itb->second; // pointer to Tcl buffer (destination)
          
          if (pb != NULL)
          {
               Tcl_Free(pb);
          }
          pb = Tcl_Alloc(static_cast<unsigned int>(ps->size()) + 1);
          copy(ps->begin(), ps->end(), pb);
          pb[ps->size()] = '\0';
          
          Tcl_UpdateLinkedVar(getInterp(), it->second.c_str());
     }
     
     // update other (int and double) Tcl links
     for (IntLinks::iterator it = intLinks.begin();
          it != intLinks.end(); ++it)
     {
          Tcl_UpdateLinkedVar(getInterp(), it->second.c_str());
     }
     for (DoubleLinks::iterator it = doubleLinks.begin();
          it != doubleLinks.end(); ++it)
     {
          Tcl_UpdateLinkedVar(getInterp(), it->second.c_str());
     }
}

// this function refreshes C++ variables from Tcl variables
void linkTcltoCpp()
{
     // it is enough to refresh strings from their buffers
     for (StringLinks::iterator it = stringLinks.begin();
          it != stringLinks.end(); ++it)
     {
          string *ps = it->first; // pointer to C++ string (destination)
          
          StringLinkBuffers::iterator itb = stringLinkBuffers.find(ps);
          char *pb = itb->second; // pointer to Tcl buffer (original value)
          
          if (pb != NULL)
          {
               ps->assign(pb);
          }
          else
          {
               ps->clear();
          }
     }
}

} // namespace // anonymous



// generic callback handler

extern "C"
int callbackHandler(ClientData cd, Tcl_Interp *interp,
     int objc, Tcl_Obj *CONST objv[])
{
     int slot = static_cast<int>(reinterpret_cast<size_t>(cd));
     
     CallbacksMap::iterator it = callbacks.find(slot);
     if (it == callbacks.end())
     {
          Tcl_AppendResult(interp,
               "Trying to invoke non-existent callback", TCL_STATIC);
          return TCL_ERROR;
     }
     
     try
     {
          // refresh C++ variables
          linkTcltoCpp();
          
          Params p(objc, reinterpret_cast<void*>(
               const_cast<Tcl_Obj **>(objv)));
          it->second->invoke(p);
          
          // refresh Tcl variables
          linkCpptoTcl();
     }
     catch (std::exception const &e)
     {
          Tcl_SetResult(interp, const_cast<char*>(e.what()), TCL_VOLATILE);
          return TCL_ERROR;
     }
     return TCL_OK;
}

// generic callback deleter

extern "C"
void callbackDeleter(ClientData cd)
{
     int slot = static_cast<int>(reinterpret_cast<size_t>(cd));
     callbacks.erase(slot);
}

string Tk::details::addCallback(shared_ptr<CallbackBase> cb)
{
     int newSlot = callbackId++;
     callbacks[newSlot] = cb;
     
     string newCmd(callbackPrefix);
     newCmd += lexical_cast<string>(newSlot);
     
     Tcl_CreateObjCommand(getInterp(), newCmd.c_str(),
          callbackHandler, reinterpret_cast<ClientData>(
			static_cast<size_t>(newSlot)),
          callbackDeleter);
     
     return newCmd;
}

string Tk::details::addLinkVar(int &i)
{
     int newLink = linkId++;
     string newLinkVar(linkVarPrefix);
     newLinkVar += lexical_cast<string>(newLink);
     
     int cc = Tcl_LinkVar(getInterp(), newLinkVar.c_str(),
          reinterpret_cast<char*>(&i), TCL_LINK_INT);
     if (cc != TCL_OK)
     {
          throw TkError(Tcl_GetStringResult(getInterp()));
     }
     
     intLinks[&i] = newLinkVar;
     return newLinkVar;
}

string Tk::details::addLinkVar(double &d)
{
     int newLink = linkId++;
     string newLinkVar(linkVarPrefix);
     newLinkVar += lexical_cast<string>(newLink);
     
     int cc = Tcl_LinkVar(getInterp(), newLinkVar.c_str(),
          reinterpret_cast<char*>(&d), TCL_LINK_DOUBLE);
     if (cc != TCL_OK)
     {
          throw TkError(Tcl_GetStringResult(getInterp()));
     }
     
     doubleLinks[&d] = newLinkVar;
     return newLinkVar;
}

string Tk::details::addLinkVar(string &s)
{
     int newLink = linkId++;
     string newLinkVar(linkVarPrefix);
     newLinkVar += lexical_cast<string>(newLink);
     
     pair<map<string *, char *>::iterator, bool> it =
          stringLinkBuffers.insert(make_pair(&s, static_cast<char*>(NULL)));
     
     char *&pb = it.first->second;
     pb = Tcl_Alloc(static_cast<unsigned int>(s.size()) + 1);
     copy(s.begin(), s.end(), pb);
     pb[s.size()] = '\0';
     
     int cc = Tcl_LinkVar(getInterp(), newLinkVar.c_str(),
          reinterpret_cast<char*>(&it.first->second), TCL_LINK_STRING);
     if (cc != TCL_OK)
     {
          throw TkError(Tcl_GetStringResult(getInterp()));
     }
     
     stringLinks[&s] = newLinkVar;
     return newLinkVar;
}

void Tk::details::deleteLinkVar(int &i)
{
     IntLinks::iterator it = intLinks.find(&i);
     if (it == intLinks.end())
     {
          return;
     }
     
     Tcl_UnlinkVar(getInterp(), it->second.c_str());
     intLinks.erase(it);
}

void Tk::details::deleteLinkVar(double &d)
{
     DoubleLinks::iterator it = doubleLinks.find(&d);
     if (it == doubleLinks.end())
     {
          return;
     }
     
     Tcl_UnlinkVar(getInterp(), it->second.c_str());
     doubleLinks.erase(it);
}

void Tk::details::deleteLinkVar(string &s)
{
     StringLinks::iterator it = stringLinks.find(&s);
     if (it == stringLinks.end())
     {
          return;
     }
     
     Tcl_UnlinkVar(getInterp(), it->second.c_str());
     stringLinks.erase(it);
     
     StringLinkBuffers::iterator itb = stringLinkBuffers.find(&s);
     char *pb = itb->second;
     if (pb != NULL)
     {
          Tcl_Free(pb);
     }
     
     stringLinkBuffers.erase(itb);
}

void details::setResult(bool b)
{
     Tcl_SetObjResult(getInterp(), Tcl_NewBooleanObj(b));
}

void details::setResult(long i)
{
     Tcl_SetObjResult(getInterp(), Tcl_NewLongObj(i));
}

void details::setResult(double d)
{
     Tcl_SetObjResult(getInterp(), Tcl_NewDoubleObj(d));
}

void details::setResult(string const &s)
{
     Tcl_SetObjResult(getInterp(),
          Tcl_NewStringObj(s.data(), static_cast<int>(s.size())));
}

int details::getResultLen()
{
     Tcl_Interp *interp = getInterp();
     
     Tcl_Obj *list = Tcl_GetObjResult(interp);
     int len, cc;
     
     cc = Tcl_ListObjLength(interp, list, &len);
     if (cc != TCL_OK)
     {
          throw TkError(Tcl_GetStringResult(interp));
     }
     
     return len;
}

template <>
int details::getResultElem<int>(int indx)
{
     Tcl_Interp *interp = getInterp();

     Tcl_Obj *list = Tcl_GetObjResult(interp);
     Tcl_Obj *obj;
     
     int cc = Tcl_ListObjIndex(interp, list, indx, &obj);
     if (cc != TCL_OK)
     {
          throw TkError(Tcl_GetStringResult(interp));
     }
     
     int val;
     cc = Tcl_GetIntFromObj(interp, obj, &val);
     if (cc != TCL_OK)
     {
          throw TkError(Tcl_GetStringResult(interp));
     }
     
     return val;
}

template <>
double details::getResultElem<double>(int indx)
{
     Tcl_Interp *interp = getInterp();

     Tcl_Obj *list = Tcl_GetObjResult(interp);
     Tcl_Obj *obj;
     
     int cc = Tcl_ListObjIndex(interp, list, indx, &obj);
     if (cc != TCL_OK)
     {
          throw TkError(Tcl_GetStringResult(interp));
     }
     
     double val;
     cc = Tcl_GetDoubleFromObj(interp, obj, &val);
     if (cc != TCL_OK)
     {
          throw TkError(Tcl_GetStringResult(interp));
     }
     
     return val;
}

template <>
string details::getResultElem<string>(int indx)
{
     Tcl_Interp *interp = getInterp();

     Tcl_Obj *list = Tcl_GetObjResult(interp);
     Tcl_Obj *obj;
     
     int cc = Tcl_ListObjIndex(interp, list, indx, &obj);
     if (cc != TCL_OK)
     {
          throw TkError(Tcl_GetStringResult(interp));
     }
     
     return Tcl_GetString(obj);
}

void details::ResultBase::invokeOnce()
{
     if (!cmd_.empty())
     {
          do_eval(exchange(cmd_, {}));
     }
}

details::Expr::~Expr() noexcept(false)
{
     if (!uncaught_exception())
     {
          invokeOnce();
     }
}

string details::ResultBase::toString() const
{
     return Tcl_GetStringResult(getInterp());
}

int details::ResultBase::toInt() const
{
     Tcl_Interp *interp = getInterp();
     Tcl_Obj *obj = Tcl_GetObjResult(interp);

     int val, cc;
     cc = Tcl_GetIntFromObj(interp, obj, &val);
     if (cc != TCL_OK)
     {
          throw TkError(Tcl_GetStringResult(interp));
     }
     
     return val;
}

double details::ResultBase::toDouble() const
{
     Tcl_Interp *interp = getInterp();
     Tcl_Obj *obj = Tcl_GetObjResult(interp);
     
     double val;
     int cc = Tcl_GetDoubleFromObj(interp, obj, &val);
     if (cc != TCL_OK)
     {
          throw TkError(Tcl_GetStringResult(interp));
     }
     
     return val;
}

Tk::Point details::ResultBase::toPoint() const
{
     auto ret = toString();
     if (ret.empty())
     {
          return Tk::Point(0, 0);
     }

     int len = getResultLen();
     if (len < 2)
     {
          throw TkError("Cannot convert the result list to Point\n");
     }
     
     int x = getResultElem<int>(0);
     int y = getResultElem<int>(1);
     
     return Point(x, y);
}

Tk::Box details::ResultBase::toBox() const
{
     auto ret = toString();
     if (ret.empty())
     {
          return Tk::Box(0, 0, 0, 0);
     }
     
     int len = getResultLen();
     if (len < 4)
     {
          throw TkError("Cannot convert the result list to Box\n");
     }
     
     int x1 = getResultElem<int>(0);
     int y1 = getResultElem<int>(1);
     int x2 = getResultElem<int>(2);
     int y2 = getResultElem<int>(3);
     
     return Box(x1, y1, x2, y2);
}

details::ExprWithPostfix::ExprWithPostfix(std::string const &str, std::string const &postfix) :
     Result(str),
     postfix_(postfix)
{
}

details::ExprWithPostfix::~ExprWithPostfix() noexcept(false)
{
     if (!uncaught_exception())
     {
          invokeOnce();
     }
}

void details::ExprWithPostfix::invokeOnce()
{
     cmd_ += exchange(postfix_, {});
     Result::invokeOnce();
}

// these specializations are used to extract parameters
// with the requested type

template <>
int details::Params::get<int>(int argno) const
{
     if (argno < 1 || argno >= argc_)
     {
          throw TkError("Parameter number out of valid range");
     }
     
     Tcl_Obj *CONST *objv = reinterpret_cast<Tcl_Obj *CONST *>(objv_);
     
     int res, cc;
     cc = Tcl_GetIntFromObj(getInterp(), objv[argno], &res);
     if (cc != TCL_OK)
     {
          throw TkError(Tcl_GetStringResult(getInterp()));
     }
     
     return res;
}

template <>
string details::Params::get<string>(int argno) const
{
     if (argno < 1 || argno >= argc_)
     {
          throw TkError("Parameter number out of valid range");
     }
     
     Tcl_Obj *CONST *objv = reinterpret_cast<Tcl_Obj *CONST *>(objv_);
     
     string res = Tcl_GetString(objv[argno]);
     return res;
}

// extract all parameters (including none) beginning at argno
template <>
vector<string> details::Params::get<vector<string>>(int argno) const
{
     if (argno < 1 || argno > argc_)	// argno == argc_ is allowed
     {
          throw TkError("Parameter number out of valid range");
     }

     Tcl_Obj *CONST *objv = reinterpret_cast<Tcl_Obj *CONST *>(objv_);

     vector<string> res(argc_ - argno);
     for (auto& s: res)
          s = Tcl_GetString(objv[argno++]);
     return res;
}

ostream & details::operator<<(ostream &os, BasicToken const &token)
{
     return os << static_cast<string>(token);
}

namespace { // anonymous

void doSingleQuote(string &s, char c)
{
     string::size_type pos = 0;
     while ((pos = s.find(c, pos)) != string::npos)
     {
          s.insert(pos, "\\");
          pos += 2;
     }
}

} // namespace anonymous

// this function is used to quote quotation marks in string values'
// in later version, it will not be needed
string details::quote(string const &s)
{
     string ret(s);
     doSingleQuote(ret, '\\');
     doSingleQuote(ret, '\"');
     doSingleQuote(ret, '$');
     doSingleQuote(ret, '[');
     doSingleQuote(ret, ']');
     
     return ret;
}

// helper functions

void Tk::deleteCallback(string const &name)
{
     string::size_type pos = name.find_first_not_of(callbackPrefix);
     if (pos == string::npos) return;
     
     int slot = lexical_cast<int>(name.substr(pos, name.size()));
     callbacks.erase(slot);
     
     int cc = Tcl_DeleteCommand(getInterp(), name.c_str());
     if (cc != TCL_OK)
     {
          throw TkError(Tcl_GetStringResult(getInterp()));
     }
}

Tk::CallbackHandle::CallbackHandle(string const &name) : name_(name) {}

Tk::CallbackHandle::~CallbackHandle() { deleteCallback(name_); }

Expr Tk::eval(string const &str)
{
     return Expr(str);
}

details::VarProxy::VarProxy(std::string name) :
	name_(std::move(name))
{
}

void details::VarProxy::operator=(int v)
{
	eval("set "s + name_ + " "s + toString(v));
}

void details::VarProxy::operator=(double v)
{
	eval("set "s + name_ + " "s + toString(v));
}

void details::VarProxy::operator=(std::string const& str)
{
	eval("set "s + name_ + " \""s + details::quote(str) + "\""s);
}

details::VarProxy::operator std::string() const
{
	return eval("expr {$"s + name_ + '}');
}

details::VarProxy::operator int() const
{
	return int(eval("expr {$"s + name_ + '}'));
}

details::VarProxy::operator double() const
{
	return double(eval("expr {$"s + name_ + '}'));
}

details::Expr Tk::literals::operator"" _tcl(const char *str, std::size_t len)
{
	return eval(std::string(str, len));
}

std::string Tk::literals::operator"" _tcls(const char *str, std::size_t len)
{
	return eval(std::string(str, len));
}

int Tk::literals::operator"" _tcli(const char *str, std::size_t len)
{
	return int(eval(std::string(str, len)));
}

double Tk::literals::operator"" _tcld(const char *str, std::size_t len)
{
	return double(eval(std::string(str, len)));
}

details::VarProxy Tk::literals::operator"" _tclv(const char *str, std::size_t len)
{
	return details::VarProxy(std::string(str, len));
}

std::string Tk::literals::operator"" _tclvs(const char *str, std::size_t len)
{
	return details::VarProxy(std::string(str, len));
}

int Tk::literals::operator"" _tclvi(const char *str, std::size_t len)
{
	return details::VarProxy(std::string(str, len));
}

double Tk::literals::operator"" _tclvd(const char *str, std::size_t len)
{
	return details::VarProxy(std::string(str, len));
}

void Tk::init(const char *argv0)
{
	Tcl_FindExecutable(argv0);
}

void Tk::runEventLoop()
{
     // refresh Tcl variables
     linkCpptoTcl();
     
     Tk_MainLoop();
}

void Tk::setDumpStream(ostream &os)
{
#ifdef CPPTK_DUMP_COMMANDS
	dumpstream = &os;
#endif // CPPTK_DUMP_COMMANDS
}
