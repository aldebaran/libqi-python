#include <Python.h>
#include <gtest/gtest.h>
#include <qi/anyobject.hpp>
#include <qi/application.hpp>
#include <qi/session.hpp>
#include <qi/jsoncodec.hpp>
#include <qipython/gil.hpp>
#include <qipython/error.hpp>
#include <qipython/pysession.hpp>
#include <boost/thread.hpp>

qiLogCategory("TestQiPython");

namespace py = qi::py;

PyThreadState *mainstate;

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  qi::Application app(argc, argv);
  if (argc < 3)
  {
    std::cerr << "Usage: " << argv[0] << " <sdk path> <src path> [pythonhome]" << std::endl;
    return 1;
  }

  std::string sdk_dir = argv[1];
  std::string src_dir = argv[2];

  if (argc >= 4)
  {
#if PY_MAJOR_VERSION >= 3
    std::string argv3 = argv[3];
    std::vector<wchar_t> wargv3(argv3.begin(), argv3.end());
    Py_SetPythonHome(&wargv3[0]);
#else
    Py_SetPythonHome(argv[3]);
#endif
  }

  Py_InitializeEx(0);
  PyEval_InitThreads();
  mainstate = PyThreadState_Swap(NULL);
  PyEval_ReleaseLock();

  std::ostringstream ss;

  {
    py::GILScopedLock _lock;

    try
    {
      boost::python::object sys(boost::python::import("sys"));
      boost::python::object os(boost::python::import("os"));
      sys.attr("path").attr("insert")(0, src_dir);
      sys.attr("path").attr("insert")(0, sdk_dir);

      boost::python::object globals = boost::python::import("__main__").attr("__dict__");
      boost::python::exec("import qi", globals, globals);
    }
    catch(...)
    {
      std::string s = PyFormatError();
      qiLogError() << "init error: " << s;
      return 1;
    }
  }

  int ret = RUN_ALL_TESTS();

  PyThreadState_Swap(mainstate);
  PyEval_AcquireLock();
  Py_Finalize();

  return ret;
}
