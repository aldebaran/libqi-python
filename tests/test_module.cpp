#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <qi/anymodule.hpp>
#include <qipython/common.hpp>
#include <qipython/pyguard.hpp>

qiLogCategory("TestQiPython.Module");

namespace py = pybind11;

namespace
{

void globalExec(const std::string& str)
{
  qi::py::GILAcquire _lock;
  py::exec(str.c_str());
}

testing::AssertionResult assertGlobalExec(const std::string& str)
{
  try
  {
    globalExec(str);
  }
  catch (const std::exception& ex)
  {
    return testing::AssertionFailure() << ex.what();
  }
  return testing::AssertionSuccess() << "the code was executed successfully";
}

bool sameModule(const qi::ModuleInfo& lhs, const qi::ModuleInfo& rhs)
{
  return lhs.name == rhs.name && lhs.path == rhs.path && lhs.type == rhs.type;
}

} // namespace

MATCHER(ModuleEq, "") { return sameModule(std::get<0>(arg), std::get<1>(arg)); }

TEST(Module, listModules)
{
  qi::py::GILAcquire _lock;
  auto qi = py::globals()["qi"];
  auto modulesFromPython =
    qi::AnyReference::from(qi.attr("listModules")()).to<std::vector<qi::ModuleInfo>>();

  using namespace testing;
  ASSERT_THAT(modulesFromPython, Pointwise(ModuleEq(), qi::listModules()));
}

TEST(ModuleFromCpp, importModule)
{
  EXPECT_TRUE(assertGlobalExec("qi.module('moduletest')"));
}

TEST(ModuleFromCpp, importAbsentModuleThrows)
{
  EXPECT_FALSE(assertGlobalExec("qi.module('nobody_here')"));
}

TEST(ModuleFromCpp, callModuleHiddenMethod)
{
  EXPECT_TRUE(assertGlobalExec("module=qi.module('moduletest')\nmodule._hidden()"));
}
