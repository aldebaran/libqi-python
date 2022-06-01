/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#include <qipython/pyobject.hpp>
#include <qipython/common.hpp>
#include <qipython/pyguard.hpp>
#include <qipython/pyproperty.hpp>
#include <qipython/pysignal.hpp>
#include <qipython/pyfuture.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <boost/optional/optional_io.hpp>
#include "common.hpp"

namespace py = pybind11;
using namespace qi::py;

#define TEST_PYTHON_OBJECT_UID \
  R"(b"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14")"

namespace
{

constexpr const auto declareTypeAWithUid = R"py(
class A(object):
    __qi_objectuid__ = )py" TEST_PYTHON_OBJECT_UID;

constexpr const std::array<std::uint8_t, 20> testUidData =
  { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
const auto testUid = *qi::deserializeObjectUid(testUidData);

}

struct ReadObjectUidTest : qi::py::test::Execute, testing::Test {};

TEST_F(ReadObjectUidTest, ReturnsEmptyIfAbsent)
{
  GILAcquire lock;
  const auto a = eval("object()");
  auto objectUid = qi::py::detail::readObjectUid(a);
  EXPECT_FALSE(objectUid);
}

TEST_F(ReadObjectUidTest, ReturnsValueIfPresent)
{
  GILAcquire lock;
  exec(declareTypeAWithUid);
  auto a = eval("A()");
  auto objectUid = qi::py::detail::readObjectUid(a);

  ASSERT_TRUE(objectUid);
  EXPECT_THAT(qi::serializeObjectUid<std::vector<std::uint8_t>>(*objectUid),
              testing::ElementsAreArray(testUidData));
}

struct WriteObjectUidTest : qi::py::test::Execute, testing::Test
{
  static constexpr const std::array<std::uint8_t, 20> data =
    { 10,  20,  30,  40,  50,  60,  70,  80,  90,  100,
      110, 120, 130, 140, 150, 160, 170, 180, 190, 200 };

  const qi::ObjectUid uid = *qi::deserializeObjectUid(data);
};

constexpr const std::array<std::uint8_t, 20> WriteObjectUidTest::data;

TEST_F(WriteObjectUidTest, CanBeReadAfterWritten)
{
  GILAcquire lock;
  exec(R"(
class A(object):
    pass
)");
  auto a = eval("A()");

  qi::py::detail::writeObjectUid(a, uid);
  auto uidRead = qi::py::detail::readObjectUid(a);
  ASSERT_TRUE(uidRead);
  EXPECT_EQ(uid, *uidRead);
}

TEST_F(WriteObjectUidTest, CanBeReadAfterOverwritten)
{
  GILAcquire lock;
  exec(declareTypeAWithUid);
  auto a = eval("A()");

  auto uidRead = qi::py::detail::readObjectUid(a);
  EXPECT_TRUE(uidRead);
  EXPECT_NE(uid, uidRead);

  qi::py::detail::writeObjectUid(a, uid);
  uidRead = qi::py::detail::readObjectUid(a);
  ASSERT_TRUE(uidRead);
  EXPECT_EQ(uid, *uidRead);
}

struct ToObjectTest : qi::py::test::Execute, testing::Test
{
  ToObjectTest()
  {
    constexpr const auto declareType = R"py(
class Cookies(object):
    def __init__(self, count=1):
        self.count = count
        self.__qi_objectuid__ = )py" TEST_PYTHON_OBJECT_UID R"py(

        t = type(self)
        t.prepare.__qi_signature__ = "([s])"
        t.prepare.__qi_return_signature__ = "s"
        t.must_not_be_bound.__qi_signature__ = "DONOTBIND"
        t.unnamed.__qi_name__ = "do_nothing"

    @staticmethod
    def bake_instructions(temp, dura):
        return "Your cookies must be baked at {} degrees for {} minutes." \
          .format(temp, dura)

    def bake(self, temp, dura):
        return [(temp, dura) for i in range(self.count)]

    def prepare_with(self, recipe, *ingredients):
        return "To prepare {} cookies according to {} recipe, " \
               "you need {} as ingredients.".format(self.count,
                                                    recipe,
                                                    ", ".join(ingredients))

    def prepare(self, ingredients):
        return "Put {} in a bowl".format(", ".join(ingredients))

    def must_not_be_bound(self):
        pass

    def unnamed(self):
        return "This function does nothing."
)py";
    exec(declareType);
  }

  template<typename... Args>
  qi::AnyObject makeObject(Args&&... args)
  {
    GILAcquire lock;
    const auto type = locals()["Cookies"];
    return qi::py::toObject(type(std::forward<Args>(args)...));
  }
};

TEST_F(ToObjectTest, StaticMethodCanBeCalled)
{
  auto obj = makeObject();
  const auto res = obj.call<std::string>("bake_instructions", 180, 20);
  EXPECT_EQ("Your cookies must be baked at 180 degrees for 20 minutes.", res);
}

TEST_F(ToObjectTest, MemberMethodCanBeCalled)
{
  auto obj = makeObject(12);
  const auto res = obj.call<std::vector<std::pair<int, int>>>("bake", 180, 20);
  const std::vector<std::pair<int, int>> expected(12, std::make_pair(180, 20));
  EXPECT_EQ(expected, res);
}

TEST_F(ToObjectTest, MemberVariadicMethodCanBeCalled)
{
  auto obj = makeObject(16);
  const auto res =
    obj.call<std::string>("prepare_with", "grandma's", "150g of wheat",
                          "100g of sugar", "2 eggs");
  EXPECT_EQ(
    "To prepare 16 cookies according to grandma's recipe, you need 150g of "
    "wheat, 100g of sugar, 2 eggs as ingredients.",
    res);
}

TEST_F(ToObjectTest, MethodWithExplicitSignature)
{
  auto obj = makeObject();
  const auto metaObj = obj.metaObject();
  const auto methods = metaObj.findMethod("prepare");
  ASSERT_EQ(1, methods.size());

  const auto method = methods[0];
  EXPECT_EQ(qi::Signature("([s])"), method.parametersSignature());
  EXPECT_EQ(qi::Signature("s"), method.returnSignature());
}

TEST_F(ToObjectTest, MethodWithExplicitBindForbiddance)
{
  auto obj = makeObject();
  const auto metaObj = obj.metaObject();
  const auto methods = metaObj.findMethod("must_not_be_bound");
  ASSERT_TRUE(methods.empty());
}

TEST_F(ToObjectTest, MethodWithOverridenName)
{
  auto obj = makeObject();
  auto res = obj.call<std::string>("do_nothing");
  EXPECT_EQ("This function does nothing.", res);
}

TEST_F(ToObjectTest, ObjectUidIsReused)
{
  auto obj = makeObject();
  EXPECT_EQ(testUid, obj.uid());
}

struct ObjectTest : testing::Test
{
  void SetUp() override
  {
    object = boost::make_shared<Muffins>();

    GILAcquire lock;
    pyObject = qi::py::toPyObject(object);
    ASSERT_TRUE(pyObject);
  }

  void TearDown() override
  {
    GILAcquire lock;
    pyObject.release().dec_ref();
  }

  struct Muffins
  {
    std::string count(int i)
    {
      std::ostringstream oss;
      oss << "You have " << i << " muffins.";
      return oss.str();
    }

    qi::Property<int> bakedCount;
    qi::Signal<int, std::string> baked;
  };

  qi::Object<Muffins> object;
  py::object pyObject;
};

QI_REGISTER_OBJECT(ObjectTest::Muffins, count, bakedCount, baked)

TEST_F(ObjectTest, IsValid)
{
  GILAcquire lock;
  {
    EXPECT_TRUE(py::bool_(pyObject));
    EXPECT_TRUE(py::bool_(pyObject.attr("isValid")()));
  }

  {
    const auto pyObject = qi::py::toPyObject(qi::py::Object{});
    // It still returns a non null Python object.
    ASSERT_TRUE(pyObject);
    EXPECT_FALSE(py::bool_(pyObject));
    EXPECT_FALSE(py::bool_(pyObject.attr("isValid")()));
  }
}

TEST_F(ObjectTest, Call)
{
  GILAcquire lock;
  const auto res = pyObject.attr("call")("count", 832).cast<std::string>();
  EXPECT_EQ("You have 832 muffins.", res);
}

TEST_F(ObjectTest, Async)
{
  GILAcquire lock;
  {
    const auto res = qi::py::test::toFutOf<std::string>(
      py::cast<qi::py::Future>(pyObject.attr("async")("count", 2356)));
    GILRelease unlock;
    ASSERT_TRUE(test::finishesWithValue(res));
    EXPECT_EQ("You have 2356 muffins.", res.value());
  }

  {
    const auto res = qi::py::test::toFutOf<std::string>(py::cast<qi::py::Future>(
      pyObject.attr("call")("count", 32897, py::arg("_async") = true)));
    GILRelease unlock;
    ASSERT_TRUE(test::finishesWithValue(res));
    EXPECT_EQ("You have 32897 muffins.", res.value());
  }
}

using ToPyObjectTest = ObjectTest;

TEST_F(ToPyObjectTest, MethodCanBeCalled)
{
  GILAcquire lock;
  {
    const auto res = pyObject.attr("count")(8).cast<std::string>();
    EXPECT_EQ("You have 8 muffins.", res);
  }

  // Can also be called asynchronously.
  {
    const auto res = qi::py::test::toFutOf<std::string>(py::cast<qi::py::Future>(
      pyObject.attr("count")(5, py::arg("_async") = true)));

    GILRelease unlock;
    ASSERT_TRUE(test::finishesWithValue(res));
    EXPECT_EQ("You have 5 muffins.", res.value());
  }
}

TEST_F(ToPyObjectTest, PropertyIsExposed)
{
  GILAcquire lock;
  const py::object prop = pyObject.attr("bakedCount");
  EXPECT_TRUE(qi::py::isProperty(prop));
}

TEST_F(ToPyObjectTest, SignalIsExposed)
{
  GILAcquire lock;
  const py::object sig = pyObject.attr("baked");
  EXPECT_TRUE(qi::py::isSignal(sig));
}

TEST_F(ToPyObjectTest, FutureAsObjectIsReturnedAsPyFuture)
{
  GILAcquire lock;
  const auto res = pyObject.attr("count")(8).cast<std::string>();
  EXPECT_EQ("You have 8 muffins.", res);
}
