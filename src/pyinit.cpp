#include <Python.h>
#include <qi/application.hpp>
#include <qipython/pyinit.hpp>
#include <qipython/gil.hpp>

namespace qi {
  namespace py {
    namespace {
      PyThreadState* _mainThread = 0;
    }

    void initialize(bool autoUninitialization)
    {
      if (_mainThread)
        return;

      PyEval_InitThreads();
      Py_InitializeEx(0);
      _mainThread = PyEval_SaveThread();

      if(autoUninitialization)
        qi::Application::atExit(&uninitialize);
    }

    void uninitialize()
    {
      if (!_mainThread)
        return;

      PyEval_RestoreThread(_mainThread);
      Py_Finalize();
    }

    void initialise()
    {
      initialize();
    }

    void uninitialise()
    {
      uninitialize();
    }
  }
}
