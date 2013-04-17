/*
**
** Author(s):
**  - Cedric GESTES <gestes@aldebaran-robotics.com>
**
** Copyright (C) 2013 Aldebaran Robotics
*/

#include <boost/python.hpp>
#include "pyapplication.hpp"
#include "pyfuture.hpp"
#include "pysession.hpp"
#include "pyobject.hpp"

BOOST_PYTHON_MODULE(libqipy)
{
  qi::py::export_pyfuture();
  qi::py::export_pyapplication();
  qi::py::export_pysession();
  qi::py::export_pyobject();
}
