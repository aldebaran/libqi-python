#include <iostream>
#include <qi/anymodule.hpp>
#include <qi/session.hpp>

qiLogCategory("qi.python.moduletest");

class Mouse
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

class Purr
{
public:
  Purr(std::shared_ptr<std::atomic<int>> counter) : counter(counter)
  {
    qiLogInfo() << this << " purr constructor ";
    ++(*counter);
  }
  ~Purr()
  {
    qiLogInfo() << this << " purr destructor ";
    --(*counter);
  }
  void run()
  {
    qiLogInfo() << this << " purring";
  }
  qi::Property<int> volume;
private:
  std::shared_ptr<std::atomic<int>> counter;
};

QI_REGISTER_OBJECT(Purr, run, volume);

class Sleep
{
public:
  Sleep(std::shared_ptr<std::atomic<int>> counter) : counter(counter)
  {
    qiLogInfo() << this << " sleep constructor ";
    ++(*counter);
  }
  ~Sleep()
  {
    qiLogInfo() << this << " sleep destructor ";
    --(*counter);
  }
  void run()
  {
    qiLogInfo() << this << " sleeping";
  }
private:
  std::shared_ptr<std::atomic<int>> counter;
};

QI_REGISTER_OBJECT(Sleep, run);

class Play
{
public:
  Play(std::shared_ptr<std::atomic<int>> counter) : counter(counter)
  {
    qiLogInfo() << this << " play constructor ";
    ++(*counter);
  }
  ~Play()
  {
    qiLogInfo() << this << " play destructor ";
    --(*counter);
  }
  void run()
  {
    qiLogInfo() << this << " playing";
  }
  qi::Signal<void> caught;
private:
  std::shared_ptr<std::atomic<int>> counter;
};

QI_REGISTER_OBJECT(Play, run, caught);

class Cat
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

    boost::shared_ptr<Purr> makePurr() const
    {
      return boost::make_shared<Purr>(purrCounter);
    }

    boost::shared_ptr<Sleep> makeSleep() const
    {
      return boost::make_shared<Sleep>(sleepCounter);
    }

    boost::shared_ptr<Play> makePlay() const
    {
      return boost::make_shared<Play>(playCounter);
    }

    void order(qi::AnyObject /*action*/) const
    {
      // Cats do not follow orders, they do nothing.
    }

    int nbPurr()
    {
      return purrCounter->load();
    }

    int nbSleep()
    {
      return sleepCounter->load();
    }

    int nbPlay()
    {
      return playCounter->load();
    }

    qi::Property<float> hunger;
    qi::Property<float> boredom;
    qi::Property<float> cuteness;

    std::shared_ptr<std::atomic<int>> purrCounter = std::make_shared<std::atomic<int>>(0);
    std::shared_ptr<std::atomic<int>> sleepCounter = std::make_shared<std::atomic<int>>(0);
    std::shared_ptr<std::atomic<int>> playCounter = std::make_shared<std::atomic<int>>(0);
};

QI_REGISTER_OBJECT(Cat, meow, cloneMe, hunger, boredom, cuteness,
                   makePurr, makeSleep, makePlay, order,
                   nbPurr, nbSleep, nbPlay);

Cat::Cat()
{
  qiLogInfo() << "Cat constructor";
}

Cat::Cat(const std::string& s) : Cat()
{
  qiLogInfo() << "Cat string constructor: " << s;
}

Cat::Cat(const qi::SessionPtr& s) : Cat()
{
  qiLogInfo() << "Cat string constructor with session";
  s->services(); // SEGV?
}

std::string Cat::meow(int volume)
{
  qiLogInfo() << "meow: " << volume;
  return "meow";
}

bool Cat::eat(const Mouse&)
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
