#ifndef lint
static char *rcsid = "$Id: deleteshmem.cc,v 1.4 2009/01/16 15:17:13 jromera Exp $";
#endif

#include "treeconf.h"
#include <string>
#include <iostream>
#include <sstream>
#include <map>

using namespace std;
using namespace treeconf;


class Lamp {

private:

  string name;
  bool isOn_;

  Lamp(const Lamp&) {}

public:
  Lamp(const string& n) : name(n),
                          isOn_(false)
  {}

  void turnOn () {
    cout << "Turning " << name << " ON!\n";
    isOn_ = true;
  }
  void turnOff () {
    cout << "Turning  " << name << " OFF!\n";
    isOn_ = false;
  }

  bool isOn() {
    return isOn_;
  }

  void dim(float value) {
    cout << "Dimming ligths to " << value << " percent\n";
  }

  const string& getName(){return name;}

};

class LampController : public Token {

  class OnOffSwitch : public Command {
    Lamp* const lamp;

  public:
    OnOffSwitch(Lamp& l) : Command("toggle", false, "Switch from ON to OFF or vice versa"), lamp(&l) {}

    const Result run () throw (RunException){
      if (lamp->isOn()) lamp->turnOff(); else lamp->turnOn();
      return Result(0, "Lamp toggled successfully");
    }
  };

  class Dimmer : public Command {

    Lamp* const lamp;
    Argument darg;

  public:
    Dimmer(Lamp& l) : Command("dim", true, "Dim lights"), lamp(&l), darg("<dim_value>", false, "A percentage between 0 and 100 ") {
      push(&darg);
    }

    const Result run () throw (RunException){
      float value;
      const string& val(darg.getText());
      stringstream ss(val);
      if (val.empty()) {
        lamp->dim(50);
        return Result (0, "Lamp dimmed successfully to default value");
      } else if (ss >> value) {
        lamp->dim(value);
        return Result (0, "Lamp dimmed successfully");
      } else {
        throw RunException(this, ("Cannot convert \"" + val + "\" to dim value").c_str());
      }
    }
  };

  Lamp* lamp;
  Dimmer dimmer;
  OnOffSwitch sw;

public:
  LampController (Lamp& l) : Token(l.getName().c_str()), lamp(&l), dimmer(l), sw(l){
    push(&sw);
    push(&dimmer);
  }

};

typedef map<string, Lamp*> LampMap;

class ArgSwitch : Command {
  Argument* whatLamp_;
  LampMap* lampMap_;

public:
  ArgSwitch(Argument* whatLamp, LampMap* lmap) : Command("toggle", false, "Switch from ON to OFF and vice versa"),
                                                 whatLamp_(whatLamp),
                                                 lampMap_(lmap)
  {
    whatLamp_->push(this);
  }

  const Result run () throw (RunException) {
    const string& val(whatLamp_->getText());
    LampMap::iterator l = lampMap_->find(val);
    if ( l != lampMap_->end() ) {
      Lamp* lamp = l->second;
      if (lamp->isOn()) lamp->turnOff(); else lamp->turnOn();
      return Result (0, "Lamp toggled successfully");
    } else {
      throw RunException(this, ("Could not find lamp called: \"" + val + "\"").c_str());
    }
  }
};

class ArgDimmer : Command {
  Argument* whatLamp_;
  TArgument<float> darg_;
  LampMap* lampMap_;

public:
  ArgDimmer(Argument* whatLamp, LampMap* lmap) : Command("dim", false, "Dim lights"),
                                                 whatLamp_(whatLamp),
                                                 darg_("<dim_value>", false, "A percentage between 0 and 100"),
                                                 lampMap_(lmap)
  {
    whatLamp_->push(this);
    push(&darg_);
  }

  const Result run () throw (RunException) {
    const string& val(whatLamp_->getText());
    LampMap::iterator l = lampMap_->find(val);
    if ( l != lampMap_->end() ) {
      Lamp* lamp = l->second;
      float value;
      if (darg_.getValue(value)) {
        lamp->dim(value);
        return Result (0, "Lamp dimmed successfully");
      } else {
        throw RunException(this, string("Could not covnert:!").c_str());
      }
    } else {
      throw RunException(this, ("Could not find lamp called: \"" + val + "\"").c_str());
    }
  }
};

class TArgDimmer : Command {
  TArgument<Lamp*> *whatLamp_;
  TArgument<float> darg_;

public:
  TArgDimmer(TArgument<Lamp*> *whatLamp, LampMap* lmap) : Command("dim", false, "Dim lights"),
                                                          whatLamp_(whatLamp),
                                                          darg_("<dim_value>", false, "A percentage between 0 and 100")
  {
    whatLamp_->push(this);
    push(&darg_);
  }

  const Result run () throw (RunException) {
    Lamp* lamp = NULL;
    
    if (whatLamp_->getValue(lamp) and lamp) {
      float dimvalue;
      if (darg_.getValue(dimvalue)) {
        lamp->dim(dimvalue);
        return Result (0, "Lamp dimmed successfully");
      } else
        throw RunException(this, (string() + "Could not convert: \"" + darg_.getText() + "\"").c_str());
    } else
      throw RunException(this, (string() + "Could not find lamp called: \"" + whatLamp_->getText() + "\"").c_str());
  }
};

void testParse(Token& tok, int argc, char* argv[]) {
  try {
    Result retval = tok.parse(argc-1, &argv[1]);
    cout << string() + "And the result is\"" + retval.what() + "\"\n";
  } catch (ParseException& e) {
    cerr << string() + "Caught ParseException: " + e.what() + " after \""+ e.where()->getName() + "\"\n";
    cerr << string() + "Correct use of whole command is: " + e.root()->usage(false) + "\n";
  } catch (RunException& e) {
    cerr << string("Caught RunException: ") + e.what() + " for command \"" + e.where()->getName() + "\"\n";
  } catch (Result& e) {
    cout << string() + "And the thrown result is\"" + e.what() + "\"\n";
  }
}

int test1(int argc, char* argv[]) {
  cout << "Test 1\n\n";
  Token root("lighting");

  Lamp l1("lamp1");
  Lamp l2("lamp2");

  LampController lc1(l1);
  LampController lc2(l2);

  root.push(&lc1);
  root.push(&lc2);

  testParse(root, argc, argv);
  return 0;
}

int test2(int argc, char* argv[]) {
  cout << "Test 2\n\n";
  Token root("lighting");

  Lamp l1("lamp1");
  Lamp l2("lamp2");

  LampMap lmap;
  lmap.insert(make_pair(l1.getName(), &l1));
  lmap.insert(make_pair(l2.getName(), &l2));

  Argument lampArg("<lamp name>", false, "Name of lamp to control");
  ArgDimmer argDim(&lampArg, &lmap);
  ArgSwitch argSwitch(&lampArg, &lmap);

  root.push(&lampArg);

  testParse(root, argc, argv);
  return 0;
}

LampMap lmap;
int test3(int argc, char* argv[]) {
  cout << "Test 3\n\n";
  Token root("lighting");

  Lamp l1("lamp1");
  Lamp l2("lamp2");


  lmap.insert(make_pair(l1.getName(), &l1));
  lmap.insert(make_pair(l2.getName(), &l2));

  TArgument<Lamp*> lampArg("<lamp name>", false, "Name of the lamp to control");
  TArgDimmer argDim(&lampArg, &lmap);
  ArgSwitch argSwitch(&lampArg, &lmap);

  root.push(&lampArg);

  testParse(root, argc, argv);
  return 0;
}

bool operator>>(const Argument& lhs, Lamp*& rhs) throw (RunException){
  string tmp;
  stringstream ss(lhs.getText());
  if (ss >> tmp) {
    LampMap::iterator l = lmap.find(tmp);
    if ( l != lmap.end() ) {
      rhs = l->second;
      return true;
    }
  }
  throw RunException(&lhs, (string() + "Could not find a lamp named \"" + lhs.getText() + "\"").c_str());
}

bool operator>>(const Argument& lhs, float& rhs) throw (RunException){
  string tmp;
  stringstream ss(lhs.getText());
  if (ss >> rhs) {
    return true;
  }
  throw RunException(&lhs, (string() + "Could not convert \"" + lhs.getText() + "\"").c_str());
}


int
main(int argc, char **argv)
{

  test1(argc, argv);
  test2(argc, argv);
  test3(argc, argv);
  cout << "\nShould not be destroying anything\n"; 

}
