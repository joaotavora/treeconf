#include "treeconf.h"

#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>

using namespace std;

namespace treeconf {

  // Result implementation
  // 
  class ResultImpl {
    friend class Result;
    int code_;
    string what_;
    unsigned int usage;
  protected:
    ResultImpl (int code, const char* what) 
      : code_(code), what_(what), usage(1) {}
  public:
    const char* what() const { return what_.c_str(); }
    int getCode() const { return code_; }
    static const ResultImpl SUCCESS;
  };
  Result::Result (int code, const char* what) : pimpl_(new ResultImpl(code, what)) {}
  Result::Result (const Result& src) {
    pimpl_ = const_cast<Result&>(src).pimpl_;
    pimpl_->usage++;
  }
    
    Result::~Result() {
    if (--pimpl_->usage == 0) delete pimpl_;
  }
  
  const char* Result::what() const { return pimpl_->what(); }
  int Result::getCode() const { return pimpl_->getCode(); }

  const int Result::SUCCESS_CODE(0);
  const int Result::FAILURE_CODE(-1);
  const Result Result::SUCCESS(Result::SUCCESS_CODE,"Success");
  
  // TokenException implementation
  //
  class TokenExceptionImpl : public std::runtime_error {
    friend class TokenException;
    const Token* where_;
    TokenExceptionImpl(const Token* where, const char* what) 
      : runtime_error(what), where_(where) {}
    ~TokenExceptionImpl() throw () {}
    const Token* where() const { return where_; }
    const char* what() const throw () { return std::runtime_error::what(); }
  };
  TokenException::TokenException(const Token* where, const char* what) 
    : pimpl_(new TokenExceptionImpl(where, what)) {}
  TokenException::~TokenException() { delete pimpl_; }
  const Token *TokenException::where() const { return pimpl_->where(); }
  const char* TokenException::what() const { return pimpl_->what(); }

  // ParseException implementation
  // 
  class ParseExceptionImpl {
    friend class ParseException;
    const Token* root_;
    string history_;
    ParseExceptionImpl(const Token* root, const char* history) 
      : root_(root) {
      if (history != NULL) history_.assign(history);
    }
    ~ParseExceptionImpl() {}

  public:
    const Token* root() const { return root_; }
    const char* history() const { return history_.c_str(); }
  };
  ParseException::ParseException(const Token* where, const char* what, const Token* root, const char* history)
    : TokenException (where, what), pimpl_(new ParseExceptionImpl(root, history)) {}
  ParseException::~ParseException() { delete pimpl_; }
  
  const Token* ParseException::root() const { return pimpl_->root(); }
  const char* ParseException::history() const { return pimpl_->history(); }
  
  // RunException implementation
  //
  RunException::RunException(const Token* where, const char* what) 
    : TokenException(where, what) {}

  // Token implementation
  //
  class TokenImpl;
  typedef Token* TokenPtr;
  typedef vector<TokenPtr> TokenVector;
  class TokenImpl {
    friend class Token;
    const string name_;
    bool mayTerminate_;
    string help_;
    
    Argument* argchild_;
    TokenVector children_;

    TokenImpl (const char *name, const char *help, bool mayTerminate) : name_(name), mayTerminate_(mayTerminate), argchild_(NULL){
      if (help != NULL) help_.assign(help);
    }
    ~TokenImpl () { }

    bool recognizes(const char* arg) {
      return name_ == arg;
    }

    const string begDelim(bool endInstead = false, bool help = false) const {
      if (mayTerminate_)
        return endInstead?" ]":"[ ";
      else {
        if (children_.size() > 1 or (children_.size() != 0 and argchild_)) {
          return endInstead?" }":"{ ";
        } else {
          return endInstead?"":(help?"  ":"");
        }
      }
    } 

    const string endDelim() const {
      return begDelim(true);
    }

    const string indent(int depth) const {
      stringstream ss;
      for (int i = 0; i<depth; i++)
        ss << "  ";
      return ss.str();
    }
      
  public:
    
    const Result parse_w(Token* where, int argc, char* argv[], Token* root, const char* history = NULL) throw (Result, TokenException){
      if (argc >= 2) {
        string stack;
        if (history != NULL) {
          stack.assign(history);
          stack += " ";
        }
        stack += getName();
        for (TokenVector::iterator i = children_.begin(); i != children_.end(); i++) {
          if (string((*i)->getName()) == string(argv[1]))
            return (*i)->parse_w(argc-1, &argv[1], root, stack.c_str());
        }
        if (argchild_)
          return static_cast<Token*>(argchild_)->parse_w(argc-1, &argv[1], root, stack.c_str());
        else if (!children_.size()) {
          throw ParseException(where, "Too many arguments", root, history);
        } else
          throw ParseException(where, "Wrong argument", root, history);
      } else if (mayTerminate_) {
        return Result::SUCCESS;
      } else if (argchild_ or children_.size() != 0) {
        throw ParseException(where, "Not enough arguments", root, history);
      } else return Result::SUCCESS;
    }

    const char* getName() const { return name_.c_str(); }
    const char* getHelp() const { return help_.c_str(); }
    
    const char* getDescription() const { return help_.c_str(); }

    const char* usage (bool withhelp = false, bool recurse = false, int depth = 0) const {
      stringstream ss;
      static string retval("");
      if (!children_.empty() or argchild_) {
        if (withhelp) ss << indent(depth);
        ss << begDelim(false, withhelp);
        TokenVector copy = children_;
        if (argchild_) copy.push_back(argchild_);
        bool helpPrinted = false;
        for (TokenVector::const_iterator i = copy.begin(); i != copy.end(); i++) {
          bool printhelp = withhelp and !((*i)->getPimpl()->help_.empty());
          helpPrinted = helpPrinted or printhelp;
          if (i != copy.begin()) {
            if (helpPrinted) ss << "\n" << indent(depth);
            if (!withhelp) ss << " ";
            ss << "| ";
          }
          ss << (*i)->getName();
          if (printhelp) ss << " : " << (*i)->getHelp() << "";
          if (recurse or copy.size()==1) { 
            const char * subcompletions = (*i)->getPimpl()->usage(withhelp, recurse, depth + 1);
            if (subcompletions) {
              if (withhelp) ss << "\n";  else ss << " ";
              ss << subcompletions;
            }
          }
        }
        ss << endDelim();
        retval.assign(ss.str().c_str());
        return retval.c_str();
      } else return NULL;
    }

    void push(Token* father, Token* child) {
      child->addTo(father);
    }

    Token* findChild(const char* rec) {
      for (TokenVector::iterator i = children_.begin(); i != children_.end(); i++) {
        if (string((*i)->getName()) == string(rec))
          return *i;
      }
      return NULL;
    }

    void addTo(Token* child, Token* father) { father->pimpl_->children_.push_back(child); }
    void addToAsArg(Argument* child, Token* father) { father->pimpl_->argchild_=child; }

    void init() {
      for (TokenVector::iterator i = children_.begin(); i != children_.end(); i++)
        (*i)->init();
    }

  };
  Token::Token(const char* rec, const char* help, bool mayTerminate) {
    pimpl_ = new TokenImpl(rec, help, mayTerminate);
  }
  Token::~Token() {
    delete pimpl_;
  }

  const Result Token::parse_w(int argc, char* argv[], Token* root, const char* history) throw (Result, TokenException){
    if (!root) {
        root = this;
        pimpl_->init(); // FIXME:: use a wrapper function parse_w for the "init-but-only-once" trick
    }
    return pimpl_->parse_w(this, argc, argv, root, history);
  }
  const Result Token::parse(int argc, char* argv[]) throw (Result, TokenException) { return parse_w(argc, argv, NULL, NULL); }
  const char* Token::getName() const{ return pimpl_->getName(); }
  const char* Token::getHelp() const{ return pimpl_->getHelp(); }
  const char* Token::getDescription() const{ return pimpl_->getDescription(); }
  const char* Token::usage(bool withhelp) const { return pimpl_->usage(withhelp, true); }
  const char* Token::completions(bool withhelp) const { return pimpl_->usage(withhelp, false); }
  void Token::push(Token* child) { pimpl_->push(this, child); }
  void Token::addTo(Token* father) { pimpl_->addTo(this, father); }
  void Token::init() { pimpl_->init(); }
    
  // Argument implementation
  // 
  class ArgumentImpl {
    friend class Argument;
    string text_;
    
    ArgumentImpl() : text_("") {}
    ~ArgumentImpl() {}

    void setText(const char* text) { text_.assign(text); }
    const char* getText() const { return text_.c_str(); }
    
  };
  Argument::Argument(const char* name, const char* help, bool mayTerminate)
    : Token(name, help, mayTerminate), pimpl_(new ArgumentImpl()) {}
  Argument::~Argument() { delete pimpl_; }
  const Result Argument::parse_w(int argc, char* argv[], Token* root, const char* history) throw (Result, TokenException) {
    pimpl_->setText(argv[0]);
    return Token::parse_w(argc, argv, root, history);
  }
  const char* Argument::getText() const { return pimpl_->getText(); }
  void Argument::addTo(Token* father) { getPimpl()->addToAsArg(this, father); }
  void Argument::init() {pimpl_->setText("");}

  // Flag implementation
  // 
  class FlagImpl {
    friend class Flag;
    bool isSet_;
    
    FlagImpl() : isSet_(false) {}
    ~FlagImpl() {}

    void isSet(bool b) { isSet_ = b;}
    bool isSet() { return isSet_;}
    
  };
  Flag::Flag(const char* name, const char* help, bool mayTerminate)
    : Token(name, help, mayTerminate), pimpl_(new FlagImpl()) {}
  Flag::~Flag() { delete pimpl_; }
  const Result Flag::parse_w(int argc, char* argv[], Token* root, const char* history) throw (Result, TokenException) {
    pimpl_->isSet(true);
    return Token::parse_w(argc, argv, root, history);
  }
  bool Flag::isSet() const { return pimpl_->isSet(); }
  void Flag::init() { pimpl_->isSet(false); }

  // Command implementation
  //
  class CommandImpl {};
  const Result Command::parse_w(int argc, char* argv[], Token* root, const char* history) throw (Result, TokenException) {
    const Result retval = Token::parse_w(argc, argv, root, history); 
    if (retval.getCode() == Result::SUCCESS_CODE)
      return run();
    else
      return retval;
  }
  Command::Command(const char* name, const char* help, bool mayTerminate)
    : Token(name, help, mayTerminate), pimpl_(new CommandImpl()) {}
  Command::~Command() { delete pimpl_; }

}

  

  
