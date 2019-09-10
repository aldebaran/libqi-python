#include <gtest/gtest.h>
#include <boost/python.hpp>
#include <qipython/gil.hpp>
#include <qi/anymodule.hpp>
#include <qi/session.hpp>
#include <qi/testutils/testutils.hpp>

qiLogCategory("TestQiPython.Module");

namespace {

  const auto pythonServiceScript = R"py(
import qi

class TestService:
  def __init__(self):
    self.testSignal = qi.Signal()

service = TestService()
)py";

static const qi::MilliSeconds defaultTimeout{200};
}

TEST(Signal, defaultConstructedSignalAcceptsCallbackWithoutArgument)
{
  qi::py::GILScopedLock _lock;
  boost::python::object globals = boost::python::import("__main__").attr("__dict__");
  boost::python::dict locals;

  boost::python::exec(pythonServiceScript, globals, locals);

  auto service = locals.get("service");
  qi::AnyObject store = qi::AnyValue::from(service).to<qi::AnyObject>();

  {
    qi::py::GILScopedUnlock _unlock;

    qi::Promise<void> prom;

    EXPECT_NO_THROW(store.connect("testSignal",
      [prom]() mutable { prom.setValue(nullptr); }));

    store.post("testSignal");
    ASSERT_TRUE(test::finishesWithValue(prom.future(), test::willDoNothing(), defaultTimeout));
  }
}

TEST(Signal, defaultConstructedSignalAcceptsCallbackWithoutArgumentThroughASession)
{
  qi::py::GILScopedLock _lock;
  boost::python::object globals = boost::python::import("__main__").attr("__dict__");
  boost::python::dict locals;

  const auto pythonSessionScript = R"py(
session = qi.Session()
url = "tcp://127.0.0.1:9559"
session.listenStandalone(url)
session.registerService('MeTestService', service))py";

  boost::python::exec(pythonServiceScript, globals, locals);
  boost::python::exec(pythonSessionScript, globals, locals);

  {
    qi::py::GILScopedUnlock _unlock;

    auto session = qi::makeSession();
    const auto url = "tcp://127.0.0.1:9559";
    session->connect(url);
    qi::AnyObject store = session->service("MeTestService").value();

    qi::Promise<void> prom;

    EXPECT_NO_THROW(store.connect("testSignal",
      [prom]() mutable { prom.setValue(nullptr); }));

    store.post("testSignal");
    ASSERT_TRUE(test::finishesWithValue(prom.future(), test::willDoNothing(), defaultTimeout));
  }
}