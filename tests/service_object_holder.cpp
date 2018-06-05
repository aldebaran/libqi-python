/*
**  Copyright (C) 2018 SoftBank Robotics Europe
**  See COPYING for the license
*/

#include <qi/anyobject.hpp>
#include <qi/applicationsession.hpp>
#include <ka/errorhandling.hpp>
#include <iostream>

using namespace qi;

class ObjectHolder
{
public:
  void emit()
  {
    signal(_obj);
  }

  void setObject(AnyObject newObj)
  {
    _obj = newObj;
  }

  void resetObject()
  {
    _obj = {};
  }

  AnyObject getObject() const
  {
    return _obj;
  }

  Signal<AnyObject> signal;

private:
  AnyObject _obj;
};

QI_REGISTER_OBJECT(ObjectHolder, emit, setObject, resetObject, getObject, signal)

int main(int argc, char** argv)
{
  return ka::invoke_catch(
      ka::compose([] { return EXIT_FAILURE; },
                  qi::ExceptionLogError<const char*>{ "service_object_holder.main",
                                                      "Terminating because of unhandled error" }),
      [&] {
        ApplicationSession app{ argc, argv };
        app.startSession();

        auto sess = app.session();
        if (!sess || !sess->isConnected())
          throw std::runtime_error{ "Session is not connected." };
        const auto serviceName = "ObjectHolder";
        std::cout << "service_name=" << serviceName << std::endl;
        std::cout << "endpoint=" << sess->endpoints()[0].str() << std::endl;
        sess->registerService(serviceName, boost::make_shared<ObjectHolder>());
        app.run();
        return EXIT_SUCCESS;
      });
}
