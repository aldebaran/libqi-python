/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#include <qipython/pysignal.hpp>
#include <qipython/common.hpp>
#include <qipython/pyguard.hpp>
#include <qipython/pyobject.hpp>
#include <qi/session.hpp>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "common.hpp"

namespace py = pybind11;
using namespace qi::py;

struct DefaultConstructedSignalTest : qi::py::test::Execute, testing::Test
{
  DefaultConstructedSignalTest()
  {
    GILAcquire lock;
    type = py::globals()["qi"].attr("Signal");
    pySig = type();
    sig = pySig.cast<qi::py::Signal*>();
  }

  ~DefaultConstructedSignalTest()
  {
    GILAcquire lock;
    sig = nullptr;
    pySig = {};
    type = {};
  }

  py::object type;
  py::object pySig;
  qi::py::Signal* sig = nullptr;
};

TEST_F(DefaultConstructedSignalTest, AcceptsCallbackWithoutArgument)
{
  using namespace testing;
  StrictMock<MockFunction<void()>> mockFn;

  {
    GILAcquire lock;
    const auto id = pySig.attr("connect")(mockFn.AsStdFunction())
                      .cast<qi::SignalLink>();
    EXPECT_NE(qi::SignalBase::invalidSignalLink, id);
  }

  qi::py::test::CheckPoint cp;
  EXPECT_CALL(mockFn, Call()).WillOnce(Invoke(std::ref(cp)));
  (*sig)();
  ASSERT_TRUE(test::finishesWithValue(cp.fut()));
}

TEST_F(DefaultConstructedSignalTest, AcceptsCallbackWithAnyArgument)
{
  using namespace testing;
  StrictMock<MockFunction<void(int, std::string)>> mockFn;

  {
    GILAcquire lock;
    const auto id = pySig.attr("connect")(mockFn.AsStdFunction())
                      .cast<qi::SignalLink>();
    EXPECT_NE(qi::SignalBase::invalidSignalLink, id);
  }

  qi::py::test::CheckPoint cp(2);
  EXPECT_CALL(mockFn, Call(13, "cookies")).WillOnce(Invoke(std::ref(cp)));
  EXPECT_CALL(mockFn, Call(52, "muffins")).WillOnce(Invoke(std::ref(cp)));
  using Ref = qi::AutoAnyReference;
  using RefVec = qi::AnyReferenceVector;
  sig->trigger(RefVec{ Ref(13), Ref("cookies") });
  sig->trigger(RefVec{ Ref(52), Ref("muffins") });
  sig->trigger(RefVec{ Ref("cupcakes") }); // wrong arguments types, the callback is not called.
  ASSERT_TRUE(test::finishesWithValue(cp.fut()));
}

struct ConstructedThroughServiceSignalTest : qi::py::test::Execute, testing::Test
{
  void SetUp() override
  {
    servSession = qi::makeSession();
    ASSERT_TRUE(
      test::finishesWithValue(servSession->listenStandalone("tcp://localhost:0")));
    clientSession = qi::makeSession();
    ASSERT_TRUE(
      test::finishesWithValue(clientSession->connect(servSession->url())));

    {
      GILAcquire lock;
      exec(R"py(
class Cookies(object):
  def __init__(self):
    self.baked = qi.Signal()
)py");

      const auto pyObj = eval("Cookies()");
      pySig = pyObj.attr(sigName.c_str());
      servObj = qi::py::toObject(pyObj);
    }

    ASSERT_TRUE(test::finishesWithValue(
      servSession->registerService(serviceName, servObj)));
    ASSERT_TRUE(
      test::finishesWithValue(clientSession->waitForService(serviceName)));
    const auto clientObjFut = clientSession->service(serviceName).async();
    ASSERT_TRUE(test::finishesWithValue(clientObjFut));
    clientObj = clientObjFut.value();
  }

  void TearDown() override
  {
    GILAcquire lock;
    pySig = {};
  }

  const std::string serviceName = "Cookies";
  const std::string sigName = "baked";
  py::object pySig;
  qi::SessionPtr servSession;
  qi::SessionPtr clientSession;
  qi::AnyObject servObj;
  qi::AnyObject clientObj;
};

TEST_F(ConstructedThroughServiceSignalTest,
       AcceptDynamicCallbackThroughService)
{
  using namespace testing;
  StrictMock<MockFunction<void()>> mockFn;

  ASSERT_TRUE(test::finishesWithValue(
    clientObj.connect(sigName, qi::AnyFunction::fromDynamicFunction(
                                 [&](const qi::AnyReferenceVector&) {
                                   mockFn.Call();
                                   return qi::AnyValue::makeVoid().release();
                                 }))));

  {
    GILAcquire lock;
    pySig();
  }

  qi::py::test::CheckPoint cp;
  EXPECT_CALL(mockFn, Call()).WillOnce(Invoke(std::ref(cp)));
  EXPECT_TRUE(test::finishesWithValue(cp.fut()));
}
