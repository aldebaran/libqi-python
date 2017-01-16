#include <iostream>
#include <qi/anymodule.hpp>
#include <qi/session.hpp>
#include "api.hpp"

qiLogCategory("moduletest");

class MODULE_TEST_API Mouse
{
  public:
    int squeak();

    qi::Property<int> size;
};

QI_REGISTER_OBJECT(Mouse, squeak, size);

int Mouse::squeak()
{
  qiLogInfo() << "squeak";
  return 18;
}

class MODULE_TEST_API Cat
{
  public:
    Cat();
    Cat(const std::string& s);
    Cat(const qi::SessionPtr& s);

    std::string meow(int volume);
    bool eat(const Mouse& m);

    boost::shared_ptr<Cat> cloneMe()
    {
      return boost::make_shared<Cat>();
    }

    qi::Property<float> hunger;
    qi::Property<float> boredom;
    qi::Property<float> cuteness;
};

QI_REGISTER_OBJECT(Cat, meow, cloneMe, hunger, boredom, cuteness);

Cat::Cat()
{
  qiLogInfo() << "Cat constructor";
}

Cat::Cat(const std::string& s)
{
  qiLogInfo() << "Cat string constructor: " << s;
}

Cat::Cat(const qi::SessionPtr& s)
{
  qiLogInfo() << "Cat string constructor with session";
  s->services(); // SEGV?
}

std::string Cat::meow(int volume)
{
  qiLogInfo() << "meow: " << volume;
  return "meow";
}

bool Cat::eat(const Mouse& m)
{
  qiLogInfo() << "eating mouse";
  return true;
}

int lol()
{
  return 3;
}

void registerObjs(qi::ModuleBuilder* mb)
{
  mb->advertiseFactory<Mouse>("Mouse");
  mb->advertiseFactory<Cat>("Cat");
  mb->advertiseFactory<Cat, std::string>("Cat");
  mb->advertiseFactory<Cat, const qi::SessionPtr&>("Cat");
  mb->advertiseMethod("lol", &lol);
  mb->advertiseMethod("_hidden", []{});
}

QI_REGISTER_MODULE("moduletest", &registerObjs);
