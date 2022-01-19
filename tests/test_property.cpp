/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#include <qipython/pyproperty.hpp>
#include <qipython/pyfuture.hpp>
#include <qipython/common.hpp>
#include <qipython/pyguard.hpp>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "common.hpp"

namespace py = pybind11;
using namespace qi::py;

struct PropertyTest : qi::py::test::Execute, testing::Test
{
  PropertyTest()
  {
    GILAcquire lock;
    type = py::globals()["qi"].attr("Property");
  }

  ~PropertyTest()
  {
    GILAcquire lock;
    type.release().dec_ref();
  }

  py::object type;
};

TEST_F(PropertyTest, DefaultConstructedHasDynamicSignature)
{
  GILAcquire lock;
  const auto pyProp = type();
  const auto prop = pyProp.cast<qi::py::Property*>();
  const auto sigChildren = prop->signature().children();
  ASSERT_FALSE(sigChildren.empty());
  EXPECT_EQ(qi::Signature::Type_Dynamic, sigChildren[0].type());
}

TEST_F(PropertyTest, ConstructedWithSignature)
{
  GILAcquire lock;
  const auto pyProp =
    type(qi::Signature::fromType(qi::Signature::Type_String).toString());
  const auto prop = pyProp.cast<qi::py::Property*>();
  const auto sigChildren = prop->signature().children();
  ASSERT_FALSE(sigChildren.empty());
  EXPECT_EQ(qi::Signature::Type_String, sigChildren[0].type());
}

struct ConstructedPropertyTest : PropertyTest
{
  ConstructedPropertyTest()
  {
    GILAcquire lock;
    pyProp = type("i");
    prop = pyProp.cast<qi::py::Property*>();
  }

  ~ConstructedPropertyTest()
  {
    GILAcquire lock;
    prop = nullptr;
    pyProp = {};
  }

  py::object pyProp;
  qi::py::Property* prop = nullptr;
};

using PropertyValueTest = ConstructedPropertyTest;

TEST_F(PropertyValueTest, CanBeReadAfterSetFromCxx)
{
  auto setFut = prop->setValue(839).async();
  ASSERT_TRUE(test::finishesWithValue(setFut));

  GILAcquire lock;
  const auto value = pyProp.attr("value")().cast<int>();
  EXPECT_EQ(839, value);
}

TEST_F(PropertyValueTest, ValueCanBeReadAfterSetFromCxxAsync)
{
  auto setFut = prop->setValue(59940).async();
  ASSERT_TRUE(test::finishesWithValue(setFut));

  const auto valueFut = [&] {
    GILAcquire lock;
    return qi::py::test::toFutOf<int>(
      pyProp.attr("value")(py::arg("_async") = true).cast<qi::py::Future>());
  }();

  ASSERT_TRUE(test::finishesWithValue(valueFut));
  EXPECT_EQ(59940, valueFut.value());
}

using PropertySetValueTest = ConstructedPropertyTest;

TEST_F(PropertySetValueTest, ValueCanBeReadFromCxx)
{
  {
    GILAcquire lock;
    pyProp.attr("setValue")(4893);
  }

  const auto valueFut = qi::py::test::toFutOf<int>(prop->value());
  ASSERT_TRUE(test::finishesWithValue(valueFut));
  EXPECT_EQ(4893, valueFut.value());
}

namespace
{
}

TEST_F(PropertySetValueTest, ValueCanBeReadFromCxxAsync)
{
  const auto setValueFut = [&] {
    GILAcquire lock;
    return qi::py::test::toFutOf<void>(
      pyProp.attr("setValue")(3423409, py::arg("_async") = true)
        .cast<qi::py::Future>());
  }();

  ASSERT_TRUE(test::finishesWithValue(setValueFut));

  const auto valueFut = qi::py::test::toFutOf<int>(prop->value());
  ASSERT_TRUE(test::finishesWithValue(valueFut));
  EXPECT_EQ(3423409, valueFut.value());
}

using PropertyAddCallbackTest = ConstructedPropertyTest;

TEST_F(PropertyAddCallbackTest, CallbackIsCalledWhenValueIsSet)
{
  using namespace testing;
  StrictMock<MockFunction<void(int)>> mockFn;

  {
    GILAcquire lock;
    const auto id =
      pyProp.attr("addCallback")(mockFn.AsStdFunction()).cast<qi::SignalLink>();
    EXPECT_NE(qi::SignalBase::invalidSignalLink, id);
  }

  qi::py::test::CheckPoint cp;
  EXPECT_CALL(mockFn, Call(4893)).WillOnce(Invoke(std::ref(cp)));
  auto setValueFut = prop->setValue(4893);
  ASSERT_TRUE(test::finishesWithValue(setValueFut));
  ASSERT_TRUE(test::finishesWithValue(cp.fut()));
}

TEST_F(PropertyAddCallbackTest, PythonCallbackIsCalledWhenValueIsSet)
{
  using namespace testing;
  StrictMock<MockFunction<void(int)>> mockFn;

  {
    GILAcquire lock;
    auto pyFn = eval("lambda f : lambda i : f(i * 2)");
    const auto id = pyProp.attr("addCallback")(pyFn(mockFn.AsStdFunction()))
                      .cast<qi::SignalLink>();
    EXPECT_NE(qi::SignalBase::invalidSignalLink, id);
  }

  qi::py::test::CheckPoint cp;
  EXPECT_CALL(mockFn, Call(15686)).WillOnce(Invoke(std::ref(cp)));
  auto setValueFut = prop->setValue(7843);
  ASSERT_TRUE(test::finishesWithValue(setValueFut));
  ASSERT_TRUE(test::finishesWithValue(cp.fut()));
}

TEST_F(PropertyAddCallbackTest, CallbackIsCalledWhenValueIsSetAsync)
{
  using namespace testing;
  StrictMock<MockFunction<void(int)>> mockFn;

  const auto idFut = [&] {
    GILAcquire lock;
    return qi::py::test::toFutOf<qi::SignalLink>(
      pyProp
        .attr("addCallback")(mockFn.AsStdFunction(), py::arg("_async") = true)
        .cast<qi::py::Future>());
  }();
  ASSERT_TRUE(test::finishesWithValue(idFut));
  EXPECT_NE(qi::SignalBase::invalidSignalLink, idFut.value());

  qi::py::test::CheckPoint cp;
  EXPECT_CALL(mockFn, Call(7854)).WillOnce(Invoke(std::ref(cp)));
  auto setValueFut = prop->setValue(7854);
  ASSERT_TRUE(test::finishesWithValue(setValueFut));
  ASSERT_TRUE(test::finishesWithValue(cp.fut()));
}

TEST_F(PropertyAddCallbackTest, CallbackThrowsWhenCalledDoesNotReportError)
{
  using namespace testing;
  StrictMock<MockFunction<void(int)>> mockFn;

  {
    GILAcquire lock;
    const auto id =
      pyProp.attr("addCallback")(mockFn.AsStdFunction()).cast<qi::SignalLink>();
    EXPECT_NE(qi::SignalBase::invalidSignalLink, id);
  }

  qi::py::test::CheckPoint cp;
  EXPECT_CALL(mockFn, Call(3287))
    .WillOnce(DoAll(Invoke(std::ref(cp)), Throw(std::runtime_error("test error"))));
  auto setValueFut = prop->setValue(3287);
  ASSERT_TRUE(test::finishesWithValue(setValueFut));
  ASSERT_TRUE(test::finishesWithValue(cp.fut()));

  GILAcquire lock;
  EXPECT_EQ(nullptr, PyErr_Occurred());
}

using PropertyDisconnectTest = ConstructedPropertyTest;

TEST_F(PropertyDisconnectTest, AddedCallbackCanBeDisconnected)
{
  GILAcquire lock;
  const auto id = pyProp.attr("addCallback")(py::cpp_function([](int) {}))
                    .cast<qi::SignalLink>();
  EXPECT_NE(qi::SignalBase::invalidSignalLink, id);

  auto success = pyProp.attr("disconnect")(id).cast<bool>();
  EXPECT_TRUE(success);
}

TEST_F(PropertyDisconnectTest, AddedCallbackCanBeDisconnectedAsync)
{
  GILAcquire lock;
  const auto id = pyProp.attr("addCallback")(py::cpp_function([](int) {}))
                    .cast<qi::SignalLink>();
  EXPECT_NE(qi::SignalBase::invalidSignalLink, id);

  auto success = qi::py::test::toFutOf<bool>(
    pyProp.attr("disconnect")(id, py::arg("_async") = true)
      .cast<qi::py::Future>()).value();
  EXPECT_TRUE(success);
}

TEST_F(PropertyDisconnectTest, DisconnectRandomSignalLinkFails)
{
  GILAcquire lock;
  const auto id = qi::SignalLink(42);

  auto success = pyProp.attr("disconnect")(id).cast<bool>();
  EXPECT_FALSE(success);
}
