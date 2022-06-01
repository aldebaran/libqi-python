/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#include <qipython/pysession.hpp>
#include <qipython/common.hpp>
#include <qipython/pyguard.hpp>
#include <qipython/pytypes.hpp>
#include <qipython/pyobject.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <pybind11/operators.h>
#include <thread>
#include <future>

namespace py = pybind11;

namespace qi
{
namespace py
{

namespace
{

::py::object makeSession()
{
  // Ensures that the Session is deleted after the shared_pointer on it is deleted.
  // This is to mimic the behavior of qiApplication.
  // This is a fix for #38167.
  struct Deleter
  {
    boost::weak_ptr<Session> wptr;

    void operator()(Session* p) const
    {
      constexpr const auto category = "qi.python.session.deleter";
      constexpr const auto msg = "Waiting for the shared pointer destruction...";
      GILRelease x;
      auto fut = std::async(std::launch::async, [=](std::unique_ptr<Session>) {
        while (!wptr.expired())
        {
          qiLogDebug(category) << msg;
          constexpr const std::chrono::milliseconds delay(100);
          std::this_thread::sleep_for(delay);
        }
        qiLogDebug(category) << msg << " done.";
      }, std::unique_ptr<Session>(p));
      QI_IGNORE_UNUSED(fut);
    }
  };

  auto ptr = SessionPtr(new qi::Session{}, Deleter{});
  boost::get_deleter<Deleter>(ptr)->wptr = ka::weak_ptr(ptr);

  GILAcquire lock;
  return py::makeSession(ptr);
}

} // namespace

::py::object makeSession(SessionPtr sess)
{
  GILAcquire lock;
  return toPyObject(sess);
}

void exportSession(::py::module& m)
{
  using namespace ::py;

  GILAcquire lock;

  m.def("Session", []{ return makeSession(); });
}

} // namespace py
} // namespace qi
