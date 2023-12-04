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

TEST(InterpreterFinalize, GarbageObjectDtorOutsideGIL)
{
  pybind11::scoped_interpreter interp;
  pybind11::globals()["qitli"] = pybind11::module::import("test_local_interpreter");
  pybind11::exec("obj = qitli.ObjectDtorOutsideGIL()");
}

// This test checks that concurrent uses of GIL guards during finalization
// of the interpreter does not cause crashes.
//
// It uses 2 threads: a main thread, and a second native thread.
//
// The main thread starts an interpreter, starts the second thread and
// releases the GIL. The second thread acquires the GIL, then releases
// it, and waits for the interpreter to finalize. Once it is finalized,
// it tries to reacquire the GIL, and then release it, according to
// GIL guards destructors.
//
// Horizontal lines are synchronization points between the threads.
//
//    main thread (T1)
//    ~~~~~~~~~~~~~~~~
//           ▼
//   ╔═══════╪════════╗
//   ║  interpreter   ║
// ----------------------------------------------> start native thread T2
//   ║       ╎        ║     native thread (T2)
//   ║       ╎        ║     ~~~~~~~~~~~~~~~~~~
//   ║       ╎        ║             ▼
//   ║ ╔═════╪══════╗ ║             ╎
//   ║ ║ GILRelease ║ ║             ╎           -> T1 releases the GIL
// ----------------------------------------------> GIL shift T1 -> T2
//   ║ ║     ╎      ║ ║     ╔═══════╪════════╗
//   ║ ║     ╎      ║ ║     ║   GILAcquire   ║  -> T2 acquires the GIL
//   ║ ║     ╎      ║ ║     ║ ╔═════╪══════╗ ║
//   ║ ║     ╎      ║ ║     ║ ║ GILRelease ║ ║  -> T2 releases the GIL
// ----------------------------------------------> GIL shift T2 -> T1
//   ║ ╚═════╪══════╝ ║     ║ ║     ╎      ║ ║  -> T1 acquires the GIL
//   ╚═══════╪════════╝     ║ ║     ╎      ║ ║  -> interpreter starts finalizing
// ----------------------------------------------> interpreter finalized
//           ╎              ║ ╚═════╪══════╝ ║  -> T2 tries to reacquire GIL but fails, it's a noop.
//           ╎              ╚═══════╪════════╝  -> T2 tries to release GIL, but it's a noop.
//           ╎                      ╎
TEST(InterpreterFinalize, GILReleasedInOtherThread)
{
  // Synchronization mechanism for the first GIL shift, otherwise T1 will release and reacquire the
  // GIL instantly without waiting for T2.
  qi::Promise<void> shift1Prom;
  auto shift1Fut = shift1Prom.future();
  // Second GIL shift does not require an additional synchronization
  // mechanism. Waiting for interpreter finalization ensures that the
  // GIL was acquired back by thread T1.
  // Synchronization mechanism for interpreter finalization.
  qi::Promise<void> finalizedProm;
  std::future<void> asyncFut;
  {
    pybind11::scoped_interpreter interp;
    asyncFut = std::async(
        std::launch::async,
        [finalizedFut = finalizedProm.future(), shift1Prom]() mutable {
          qi::py::GILAcquire acquire;
          // First GIL shift is done.
          shift1Prom.setValue(nullptr);
          qi::py::GILRelease release;
          finalizedFut.value();
        });
    qi::py::GILRelease release;
    shift1Fut.value();
  }
  finalizedProm.setValue(nullptr);
  // Join the thread, wait for the task to finish and ensure no exception was thrown.
  asyncFut.get();
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  qi::Application app(argc, argv);
  return RUN_ALL_TESTS();
}
