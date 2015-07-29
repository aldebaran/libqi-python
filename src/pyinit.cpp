#include <Python.h>
#include <qi/application.hpp>
#include <qipython/pyinit.hpp>

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
      _mainThread = PyThreadState_Swap(NULL);
      // Py_Initialize takes the GIL, so we release it. If one wants to call
      // Python API, they must take the lock by themselves.
      PyEval_ReleaseLock();

      if(autoUninitialization)
        qi::Application::atExit(&uninitialize);
    }

    void uninitialize()
    {
      if (!_mainThread)
        return;

      PyEval_AcquireLock();
      PyThreadState_Swap(_mainThread);
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
