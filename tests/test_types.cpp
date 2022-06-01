#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <gtest/gtest.h>
#include <qi/anyobject.hpp>
#include <qi/session.hpp>
#include <qi/jsoncodec.hpp>
#include <qipython/pysession.hpp>
#include <qipython/common.hpp>
#include <qipython/pyguard.hpp>
#include <boost/thread.hpp>
#include "common.hpp"

namespace py = pybind11;

struct AnyValueFrom
{
  template<typename T>
  qi::AnyValue operator()(T&& t)
  {
    return qi::AnyValue::from(std::forward<T>(t));
  }
};

struct PybindObjectCast
{
  qi::AnyValue operator()(py::object obj)
  {
    qi::py::GILAcquire lock;
    return obj.cast<qi::AnyValue>();
  }

};

template<typename Convert>
struct ToAnyValueConversionTest : qi::py::GILAcquire, testing::Test
{
  template<typename T>
  qi::AnyValue toAnyValue(T&& t)
  {
    return Convert()(std::forward<T>(t));
  }
};
using ConvertTypes = testing::Types<AnyValueFrom, PybindObjectCast>;
TYPED_TEST_SUITE(ToAnyValueConversionTest, ConvertTypes);

TYPED_TEST(ToAnyValueConversionTest, PyInt)
{
  auto v = this->toAnyValue(py::int_(42));
  EXPECT_EQ(qi::TypeKind_Int, v.kind());

  const auto itf = static_cast<qi::IntTypeInterface*>(v.type());
  EXPECT_EQ(sizeof(long long), itf->size());
  EXPECT_TRUE(itf->isSigned());

  EXPECT_EQ(42, v.toInt());

  v.setInt(12);
  EXPECT_EQ(12, v.toInt());
  EXPECT_TRUE(v.template to<py::int_>().equal(py::int_(12)));
}

TYPED_TEST(ToAnyValueConversionTest, PyFloat)
{
  auto v = this->toAnyValue(py::float_(3.14));
  EXPECT_EQ(qi::TypeKind_Float, v.kind());
  EXPECT_DOUBLE_EQ(3.14, v.toDouble());

  v.setDouble(329.329);
  EXPECT_DOUBLE_EQ(329.329, v.toDouble());

  auto isclose = py::module::import("math").attr("isclose");
  EXPECT_TRUE(isclose(py::float_(329.329), v.template to<py::float_>()).template cast<bool>());
}

TYPED_TEST(ToAnyValueConversionTest, PyBool)
{
  auto v = this->toAnyValue(py::bool_(true));
  EXPECT_EQ(qi::TypeKind_Int, v.kind());

  const auto itf = static_cast<qi::IntTypeInterface*>(v.type());
  EXPECT_EQ(0u, itf->size());
  EXPECT_FALSE(itf->isSigned());

  EXPECT_TRUE(v.template to<bool>());

  v.set(false);
  EXPECT_FALSE(v.template to<bool>());
  EXPECT_EQ(py::bool_(false), v.template to<py::bool_>());
}

TYPED_TEST(ToAnyValueConversionTest, PyStr)
{
  auto v = this->toAnyValue(py::str("cookies"));
  EXPECT_EQ(qi::TypeKind_String, v.kind());
  EXPECT_EQ("cookies", v.toString());

  v.setString("muffins");
  EXPECT_EQ("muffins", v.toString());
  EXPECT_TRUE(v.template to<py::str>().equal(py::str("muffins")));
}

TYPED_TEST(ToAnyValueConversionTest, PyBytes)
{
  auto v = this->toAnyValue(py::bytes("cupcakes"));
  EXPECT_EQ(qi::TypeKind_String, v.kind());
  EXPECT_EQ("cupcakes", v.toString());

  v.setString("donuts");
  EXPECT_EQ("donuts", v.toString());
  EXPECT_TRUE(v.template to<py::bytes>().equal(py::bytes("donuts")));
}

struct ToAnyValueObjectConversionTest : qi::py::GILAcquire, testing::Test {};

TEST_F(ToAnyValueObjectConversionTest, AnyValueFromReturnsDynamic)
{
  auto v = qi::AnyValue::from(py::object(py::int_(42)));
  EXPECT_EQ(qi::TypeKind_Dynamic, v.kind());
  EXPECT_TRUE(v.template to<py::object>().equal(py::int_(42)));
}

// No test with `PybindObjectCast` conversion with `PyObject` as it's equivalent
// to `PybindObjectCast` conversion with the underlying object which should
// already be tested.

struct ToAnyValueListConversionTest : qi::py::GILAcquire, testing::Test
{
  static std::vector<int> toVec(qi::AnyReference listRef)
  {
    const auto vcVec = listRef.asListValuePtr();

    std::vector<int> res;
    std::transform(vcVec.begin(), vcVec.end(), std::back_inserter(res),
                   [](const qi::AnyReference& v) { return v.to<int>(); });
    return res;
  }

  const std::vector<int> values = {1, 2, 4, 8, 16};
  const py::list list = py::cast(values);
};

TEST_F(ToAnyValueListConversionTest, AnyValueFromReturnsDynamic)
{
  auto v = qi::AnyValue::from(list);
  EXPECT_EQ(qi::TypeKind_Dynamic, v.kind());
  EXPECT_TRUE(v.template to<py::list>().equal(list));
  EXPECT_EQ(values, toVec(v.content()));
}

TEST_F(ToAnyValueListConversionTest, PybindObjectCastReturnsList)
{
  auto v = list.cast<qi::AnyValue>();
  EXPECT_EQ(qi::TypeKind_List, v.kind());
  EXPECT_TRUE(v.template to<py::list>().equal(list));
  EXPECT_EQ(values, toVec(v.asReference()));
}

using DictTypes = testing::Types<py::dict, py::kwargs>;
template<typename Dict>
struct ToAnyValueDictConversionTest : qi::py::GILAcquire, testing::Test
{
  static std::map<std::string, int> toMap(qi::AnyReference map)
  {
    const auto vcMap = map.asMapValuePtr();
    std::map<std::string, int> res;
    std::transform(vcMap.begin(), vcMap.end(), std::inserter(res, res.begin()),
                   [](const std::pair<qi::AnyReference, qi::AnyReference>& v) {
                     return std::make_pair(v.first.to<std::string>(), v.second.to<int>());
                   });
    return res;
  }

  const std::map<std::string, int> values = {{ "two", 2 },
                                             { "four", 4 },
                                             { "eight", 8 },
                                             { "sixteen", 16 }};
  const Dict dict = py::cast(values);
};
TYPED_TEST_SUITE(ToAnyValueDictConversionTest, DictTypes);

TYPED_TEST(ToAnyValueDictConversionTest, AnyValueFromReturnsDynamic)
{
  using Dict = TypeParam;
  auto v = qi::AnyValue::from(this->dict);
  ASSERT_EQ(qi::TypeKind_Dynamic, v.kind());
  EXPECT_TRUE(v.template to<Dict>().equal(this->dict));
  EXPECT_EQ(this->values, this->toMap(v.content()));
}

TYPED_TEST(ToAnyValueDictConversionTest, PybindObjectCastReturnsMap)
{
  using Dict = TypeParam;
  auto v = this->dict.template cast<qi::AnyValue>();
  ASSERT_EQ(qi::TypeKind_Map, v.kind());
  EXPECT_TRUE(v.template to<Dict>().equal(this->dict));
  EXPECT_EQ(this->values, this->toMap(v.asReference()));
}

using TupleTypes = testing::Types<py::tuple, py::args>;
template <typename Tuple>
struct ToAnyValueTupleConversionTest : qi::py::GILAcquire, testing::Test
{
  static std::tuple<int, std::string, std::int8_t> toTuple(qi::AnyReference tuple)
  {
    const auto vcMap = tuple.asTupleValuePtr();
    const auto res =
      std::make_tuple(vcMap[0].to<int>(), vcMap[1].toString(),
                      vcMap[2].to<std::int8_t>());
    return res;
  }

  const std::tuple<int, std::string, std::int8_t> values = std::make_tuple(21, "cookies", 15);
  const Tuple tuple = py::cast(values);
};
TYPED_TEST_SUITE(ToAnyValueTupleConversionTest, TupleTypes);

TYPED_TEST(ToAnyValueTupleConversionTest, AnyValueReturnsDynamic)
{
  using Tuple = TypeParam;
  auto v = qi::AnyValue::from(this->tuple);
  ASSERT_EQ(qi::TypeKind_Dynamic, v.kind());
  EXPECT_TRUE(v.template to<Tuple>().equal(this->tuple));
  EXPECT_EQ(this->values, this->toTuple(v.content()));
}

TYPED_TEST(ToAnyValueTupleConversionTest, PybindObjectCastReturnsTuple)
{
  using Tuple = TypeParam;
  auto v = this->tuple.template cast<qi::AnyValue>();
  ASSERT_EQ(qi::TypeKind_Tuple, v.kind());
  EXPECT_TRUE(v.template to<Tuple>().equal(this->tuple));
  EXPECT_EQ(this->values, this->toTuple(v.asReference()));
}

struct TypePassing : qi::py::test::Execute,
                     testing::Test
{
protected:
  virtual void SetUp()
  {
    session = qi::makeSession();
    session->listenStandalone("tcp://127.0.0.1:0");

    qi::py::GILAcquire lock;
    locals()["sd"] = qi::py::makeSession(session);
  }

  virtual void TearDown()
  {
    session->close();
  }

  void registerService()
  {
    exec(
        "m = TestService()\n"
        "sd.registerService('TestService', m)\n"
        );
  }

  qi::AnyObject getService()
  {
    return session->service("TestService").value();
  }

  qi::SessionPtr session;
};

TEST_F(TypePassing, Int)
{
  exec(
      "class TestService:\n"
      "    def func(self):\n"
      "        return 42\n"
      );
  registerService();
  EXPECT_EQ(42, getService().call<int>("func"));
}

TEST_F(TypePassing, Bool)
{
  exec(
      "class TestService:\n"
      "    def func(self):\n"
      "        return True\n"
      );
  registerService();
  EXPECT_EQ(true, getService().call<bool>("func"));
}

TEST_F(TypePassing, Float)
{
  exec(
      "class TestService:\n"
      "    def func(self):\n"
      "        return 2.5\n"
      );
  registerService();
  EXPECT_FLOAT_EQ(2.5, getService().call<float>("func"));
}

TEST_F(TypePassing, String)
{
  exec(
      "class TestService:\n"
      "    def func(self):\n"
      "        return 'can i help you?'\n"
      );
  registerService();
  EXPECT_EQ("can i help you?", getService().call<std::string>("func"));
}

TEST_F(TypePassing, ByteArray)
{
  exec(
      "class TestService:\n"
      "    def func(self):\n"
      "        return bytearray('can i help you?', encoding='ascii')\n"
      );
  registerService();
  EXPECT_EQ("can i help you?", getService().call<std::string>("func"));
}

TEST_F(TypePassing, Bytes)
{
  exec(
      "class TestService:\n"
      "    def func(self):\n"
      "        return b'can i help you?'\n"
      );
  registerService();
  EXPECT_EQ("can i help you?", getService().call<std::string>("func"));
}

TEST_F(TypePassing, List)
{
  exec(
      "class TestService:\n"
      "    def func(self):\n"
      "        return [1, 2, 3]\n"
      );
  registerService();
  const std::vector<int> expected { 1, 2, 3 };
  EXPECT_EQ(expected, getService().call<std::vector<int> >("func"));
}

TEST_F(TypePassing, Dict)
{
  exec(
      "class TestService:\n"
      "    def func(self):\n"
      "        return {'one' : 1, 'two' : 2, 'three' : 3}\n"
      );
  registerService();
  const std::map<std::string, int> expected {
    {"one", 1},
    {"two", 2},
    {"three", 3},
  };
  EXPECT_EQ(expected, (getService().call<std::map<std::string, int> >("func")));
}

TEST_F(TypePassing, Recursive)
{
  exec(
      "class TestService:\n"
      "    def func(self):\n"
      "        return {'one' : 1, 'two' : [1, 2], 'three' : {42 : 'answer'}}\n"
      );
  registerService();
  const auto v = getService().call<qi::AnyValue>("func");
  const auto map = v.to<std::map<std::string, qi::AnyValue> >();
  ASSERT_EQ(3u, map.size());
  EXPECT_EQ(1, map.at("one").toInt());

  {
    const auto two = map.at("two").to<std::vector<int>>();
    const std::vector<int> expected{ 1, 2 };
    EXPECT_EQ(expected, two);
  }

  {
    const auto three = map.at("three").to<std::map<int, std::string>>();
    const std::map<int, std::string> expected{ { 42, "answer" } };
    EXPECT_EQ(expected, three);
  }
}

TEST_F(TypePassing, ReverseDict)
{
  exec(
      "class TestService:\n"
      "    def func(self, dict):\n"
      "        return dict == {'one' : 1, 'two' : 2, 'three' : 3}\n"
      );
  registerService();
  const std::map<std::string, int> expected {
    {"one", 1},
    {"two", 2},
    {"three", 3},
  };
  EXPECT_TRUE(getService().call<bool>("func", expected));
}

TEST_F(TypePassing, LogLevel)
{
  exec(
      "class TestService:\n"
      "    def func(self):\n"
      "        return qi.LogLevel.Info\n"
      );
  registerService();
  EXPECT_EQ(qi::LogLevel_Info, getService().call<int>("func"));
}

TEST_F(TypePassing, DictViews)
{
  exec(
      "class TestService:\n"
      "    def func(self, kind):\n"
      "        d = dict(one=1, two=2)\n"
      "        if kind == 0:\n"
      "            return d.keys()\n"
      "        if kind == 1:\n"
      "            return d.items()\n"
      "        return d.values()\n"
      );
  registerService();

  { // Keys
    using Keys = std::set<std::string>;
    const Keys expected{ "one", "two" };
    EXPECT_EQ(expected, getService().call<Keys>("func", 0));
  }

  { // Items
    using Items = std::set<std::pair<std::string, int>>;
    const Items expected{ { "one", 1 }, { "two", 2 } };
    EXPECT_EQ(expected, getService().call<Items>("func", 1));
  }


  { // Values
    using Values = std::vector<int>;
    const Values expected{ 1, 2 };
    auto values = getService().call<Values>("func", 2);
    std::sort(values.begin(), values.end());
    EXPECT_EQ(expected, values);
  }
}
