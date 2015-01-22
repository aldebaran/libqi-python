/*
**  Copyright (C) 2013 Aldebaran Robotics
**  See COPYING for the license
*/
#include <qipython/pysession.hpp>

namespace qi {
namespace py {

boost::python::object makeSession()
{
  return makePySession(qi::makeSession());
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
