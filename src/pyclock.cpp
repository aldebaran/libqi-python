/*
**  Copyright (C) 2013 Aldebaran Robotics
**  See COPYING for the license
*/
#include <qipython/pyclock.hpp>
#include <qi/clock.hpp>
#include <qi/log.hpp>

qiLogCategory("qipy.clock");

namespace qi {
namespace py {

  int64_t pyClockNow()
  {
    return qi::Clock::now().time_since_epoch().count();
  }
  int64_t pySteadyClockNow()
  {
    return qi::SteadyClock::now().time_since_epoch().count();
  }
  int64_t pySystemClockNow()
  {
    return qi::SystemClock::now().time_since_epoch().count();
  }

  void export_pyclock() {
    boost::python::def("clockNow", &pyClockNow,
        "clockNow() -> Int\n"
        ":return: current timestamp on qi::Clock, as a number of nanoseconds\n"
        );
    boost::python::def("steadyClockNow", &pySteadyClockNow,
        "steadyClockNow() -> Int\n"
        ":return: current timestamp on qi::SteadyClock, as a number of nanoseconds\n"
        );
    boost::python::def("systemClockNow", &pySystemClockNow,
        "systemClockNow() -> Int\n"
        ":return: current timestamp on qi::SystemClock, as a number of nanoseconds\n"
        );
  }
}
}
