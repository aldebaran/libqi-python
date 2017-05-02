#include <gtest/gtest.h>
#include <boost/python.hpp>
#include <qipython/gil.hpp>
#include <qi/anymodule.hpp>

qiLogCategory("TestQiPython.Module");

void myExec(const std::string& str)
{
  boost::python::object globals = boost::python::import("__main__").attr("__dict__");
  boost::python::exec(str.c_str(), globals, globals);
}

namespace qi
{
bool operator==(const qi::ModuleInfo& lhs, const qi::ModuleInfo& rhs)
{
  return lhs.name == rhs.name && lhs.path == rhs.path && lhs.type == rhs.type;
}
} // qi

TEST(Module, listModules)
{
  qi::py::GILScopedLock _lock;
  auto qipy = boost::python::import("qi");
  auto modulesFromPython = qi::AnyReference::from(qipy.attr("listModules")()).to<std::vector<qi::ModuleInfo>>();
  ASSERT_EQ(qi::listModules(), modulesFromPython);
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
