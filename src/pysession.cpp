/*
**  Copyright (C) 2013 Aldebaran Robotics
**  See COPYING for the license
*/
#include <thread>
#include <future>
#include <qipython/gil.hpp>
#include <qipython/pysession.hpp>

namespace qi {
namespace py {

boost::python::object makeSession()
{
  // Ensures that the Session is deleted after the shared_pointer on it is deleted.
  // This is to mimic the behavior of qiApplication.
  // This is a fix for #38167.
  struct Deleter
  {
    boost::weak_ptr<Session> wptr;

    void operator()(Session* p) const
    {
      static const char* category = "qi.python.session.deleter";
      static const char* msg = "Waiting for the shared pointer destruction...";
      GILScopedUnlock x;
      qi::async([=]{
        while (!wptr.expired())
        {
          qiLogDebug(category) << msg;
          qi::os::msleep(100);
        }
        qiLogDebug(category) << msg << " done.";
        delete p;
      });
    }
  };

  auto ptr = boost::shared_ptr<qi::Session>(new qi::Session{}, Deleter{});
  boost::get_deleter<Deleter>(ptr)->wptr = boost::weak_ptr<Session>{ptr};
  return makePySession(ptr);
}

boost::python::object makePySession(const SessionPtr& sess)
{
  return qi::AnyValue::from(sess).to<boost::python::object>();
}

void export_pysession() {
  boost::python::def("Session", &makeSession);
}

}
}
