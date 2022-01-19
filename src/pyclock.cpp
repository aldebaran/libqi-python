/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#include <qipython/pyclock.hpp>
#include <qipython/common.hpp>
#include <qipython/pyguard.hpp>
#include <qi/clock.hpp>
#include <pybind11/pybind11.h>

namespace py = pybind11;

namespace qi
{
namespace py
{

namespace
{

template<typename Clock>
typename Clock::rep now()
{
  return Clock::now().time_since_epoch().count();
}

} // namespace

void exportClock(::py::module& m)
{
  using namespace ::py;
  using namespace ::py::literals;

  GILAcquire lock;

  m.def("clockNow", &now<Clock>,
        doc(":returns: current timestamp on qi::Clock, as a number of nanoseconds"));
  m.def("steadyClockNow", &now<SteadyClock>,
        doc(":returns: current timestamp on qi::SteadyClock, as a number of nanoseconds"));
  m.def("systemClockNow", &now<SystemClock>,
        doc(":returns: current timestamp on qi::SystemClock, as a number of nanoseconds"));
}

} // namespace py
} // namespace qi
