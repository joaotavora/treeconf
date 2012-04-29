// -*- mode: c++ -*-

#ifndef TREECONF_H
#define TREECONF_H

#include <stdlib.h>

namespace treeconf {

  class Token;

  class ResultImpl;
  class Result {
    ResultImpl* pimpl_;
  public:
    Result(const Result&);
    Result(int code, const char* what);
    virtual ~Result();
    
    virtual const char* what() const;
    virtual int getCode() const;
    static const Result SUCCESS;
    static const int SUCCESS_CODE;
    static const int FAILURE_CODE;
  };
    
  class TokenExceptionImpl;
  class TokenException {
    TokenExceptionImpl* pimpl_;
    TokenException(const TokenException&);
  public:
    TokenException(const Token* where, const char* what);
    virtual ~TokenException();
    const Token* where() const;
    const char* what() const;
  };

  class ParseExceptionImpl;
  class ParseException : public TokenException {
    friend class TokenImpl;
    ParseExceptionImpl* pimpl_;
    ParseException(const ParseException&);
    ParseException(const Token* where, const char* what, const Token* root, const char* history);
  public:
    virtual ~ParseException();
    const Token* root() const;
    const char* history() const;
  };

  class RunException : public TokenException {
  public:
    RunException(const RunException&);
    RunException(const Token* where, const char* what);
  };

  class TokenImpl;
  class Token {
    friend class TokenImpl;
    TokenImpl *pimpl_;
    Token(const Token&);
  public:
    Token(const char* name, const char* help = NULL, bool mayTerminate = false);
    virtual ~Token();
    
    const char* getName() const;
    const char* getHelp() const;
    const char* getDescription() const;
    const char* usage(bool withhelp = false) const;
    const char* completions(bool withhelp = false) const;
    void push(Token* tok);
    const Result parse(int argc, char* argv[]) throw (Result, TokenException);
  protected:
    virtual const Result parse_w(int argc, char* argv[], Token* root = NULL, const char* history = NULL) throw (Result, TokenException);
    TokenImpl *getPimpl() { return pimpl_; }
    virtual void addTo(Token* tok);
    virtual void init();
  };
  
  class ArgumentImpl;
  class Argument : public Token {
    ArgumentImpl *pimpl_;
    Argument (const Argument&);
  protected:
    void addTo(Token* tok);
    void init();
    const Result parse_w(int argc, char* argv[], Token* root = NULL, const char* history = NULL) throw (Result, TokenException);
  public:
    Argument(const char* name, const char* help = NULL, bool mayTerminate = false);
    virtual ~Argument();
    
    const char* getText() const;
  };

  class FlagImpl;
  class Flag : public Token {
    FlagImpl *pimpl_;
    Flag (const Flag&);
  protected:
    void init();
    const Result parse_w(int argc, char* argv[], Token* root = NULL, const char* history = NULL) throw (Result, TokenException);
  public:
    Flag(const char* name, const char* help = NULL, bool mayTerminate = false);
    virtual ~Flag();

    bool isSet() const;
  };
  

  class CommandImpl;
  class Command : public Token {
    CommandImpl *pimpl_;
  protected:
    const Result parse_w(int argc, char* argv[], Token* root = NULL, const char* history = NULL) throw (Result, TokenException);
  public:
    Command(const char* name, const char* help = NULL, bool mayTerminate = false);
    virtual ~Command ();
    virtual const Result run() throw (Result, RunException) = 0;

  };

  #ifndef NO_TEMPLATES
  template <typename C>
  C convertTo(const char* txt);

  template <class T>
  class TArgument : public Argument {
  public:
    TArgument(const char* name, const char* help = NULL, bool mayTerminate = false)
      : Argument(name, help, mayTerminate) {}
    ~TArgument() {}
    bool getValue(T& arg) const throw (RunException) {
      return *this >> arg;
    }
  };
  #endif

}

#endif // TREECONF_H
