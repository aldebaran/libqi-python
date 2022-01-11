#include <gtest/gtest.h>
#include <qi/anyobject.hpp>
#include <qi/application.hpp>
#include <qi/session.hpp>
#include <qi/jsoncodec.hpp>
#include <ka/errorhandling.hpp>
#include <ka/functional.hpp>
#include <qipython/common.hpp>
#include <qipython/pyguard.hpp>
#include <qipython/pyexport.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>

qiLogCategory("TestQiPython");

namespace py = pybind11;

PYBIND11_EMBEDDED_MODULE(qi, m) {
  qi::py::exportAll(m);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  pybind11::scoped_interpreter interp;

  boost::optional<qi::Application> app;
  app.emplace(argc, argv);

  py::globals()["qi"] = py::module::import("qi");


  int ret = EXIT_FAILURE;
  {
    qi::py::GILRelease unlock;
    ret = RUN_ALL_TESTS();

    // Destroy the application outside of the GIL to avoid deadlocks, but while
    // the interpreter still runs to avoid crashes while trying to release the
    // references we still hold in callbacks.
    app.reset();
  }

  return ret;
}
