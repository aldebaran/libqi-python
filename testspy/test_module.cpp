#include <gtest/gtest.h>
#include <boost/python.hpp>
#include <qipython/gil.hpp>

void myExec(const std::string& str)
{
  boost::python::object globals = boost::python::import("__main__").attr("__dict__");
  boost::python::exec(str.c_str(), globals, globals);
}

TEST(ModuleFromCpp, importModule)
{
  qi::py::GILScopedLock _lock;
  myExec("import qi\nqi.module('moduletest')");
}

TEST(ModuleFromCpp, callModuleHiddenMethod)
{
  qi::py::GILScopedLock _lock;
  myExec("import qi\nmodule=qi.module('moduletest')\nmodule._hidden()");
}
