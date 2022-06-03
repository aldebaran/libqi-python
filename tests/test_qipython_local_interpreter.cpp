#include <gtest/gtest.h>
#include <qi/application.hpp>
#include <qi/property.hpp>
#include <qi/session.hpp>
#include <qipython/common.hpp>
#include <qipython/pyguard.hpp>
#include <qipython/pyobject.hpp>
#include <qipython/pysession.hpp>
#include <qipython/pyexport.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>

PYBIND11_EMBEDDED_MODULE(test_local_interpreter, m) {
  struct ObjectDtorOutsideGIL
  {
    ~ObjectDtorOutsideGIL()
    {
      qi::py::GILRelease _rel;
      // nothing.
    }

  };
  pybind11::class_<ObjectDtorOutsideGIL>(m, "ObjectDtorOutsideGIL")
    .def(pybind11::init([]{
      return std::make_unique<ObjectDtorOutsideGIL>();
    }));
}

TEST(InterpreterFinalize, DoesNotSegfaultGarbageObjectDtorOutsideGIL)
{
  pybind11::scoped_interpreter interp;
  pybind11::globals()["qitli"] = pybind11::module::import("test_local_interpreter");
  pybind11::exec("obj = qitli.ObjectDtorOutsideGIL()");
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  qi::Application app(argc, argv);
  return RUN_ALL_TESTS();
}
