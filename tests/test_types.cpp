#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <gtest/gtest.h>
#include <qi/anyobject.hpp>
#include <qi/session.hpp>
#include <qi/jsoncodec.hpp>
#include <qipython/pysession.hpp>
#include <qipython/common.hpp>
#include <boost/thread.hpp>
#include "common.hpp"

namespace py = pybind11;

struct AnyValueDefaultConversionTest : qi::py::test::WithGIL, testing::Test {};

TEST_F(AnyValueDefaultConversionTest, PyInt)
{
  py::gil_scoped_acquire lock;
  py::int_ i(42);
  auto v = qi::AnyValue::from(i);
  EXPECT_EQ(qi::TypeKind_Int, v.kind());
  EXPECT_EQ(42, v.toInt());

  v.setInt(12);
  EXPECT_EQ(12, v.toInt());
  EXPECT_TRUE(v.to<py::int_>().equal(py::int_(12)));
}

TEST_F(AnyValueDefaultConversionTest, PyFloat)
{
  py::gil_scoped_acquire lock;

  py::float_ f(3.14);
  auto v = qi::AnyValue::from(f);
  EXPECT_EQ(qi::TypeKind_Float, v.kind());
  EXPECT_DOUBLE_EQ(3.14, v.toDouble());

  v.setDouble(329.329);
  EXPECT_DOUBLE_EQ(329.329, v.toDouble());

  auto isclose = py::module::import("math").attr("isclose");
  EXPECT_TRUE(isclose(py::float_(329.329), v.to<py::float_>()).cast<bool>());
}

TEST_F(AnyValueDefaultConversionTest, PyBool)
{
  py::bool_ b(true);
  auto v = qi::AnyValue::from(b);
  EXPECT_EQ(qi::TypeKind_Int, v.kind());
  EXPECT_TRUE(v.to<bool>());

  v.set(false);
  EXPECT_FALSE(v.to<bool>());
  EXPECT_EQ(py::bool_(false), v.to<py::bool_>());
}

TEST_F(AnyValueDefaultConversionTest, PyStr)
{
  py::str s("cookies");
  auto v = qi::AnyValue::from(s);
  EXPECT_EQ(qi::TypeKind_String, v.kind());
  EXPECT_EQ("cookies", v.toString());

  v.setString("muffins");
  EXPECT_EQ("muffins", v.toString());
  EXPECT_TRUE(v.to<py::str>().equal(py::str("muffins")));
}

TEST_F(AnyValueDefaultConversionTest, PyBytes)
{
  py::bytes b("cupcakes");
  auto v = qi::AnyValue::from(b);
  EXPECT_EQ(qi::TypeKind_String, v.kind());
  EXPECT_EQ("cupcakes", v.toString());

  v.setString("donuts");
  EXPECT_EQ("donuts", v.toString());
  EXPECT_TRUE(v.to<py::bytes>().equal(py::bytes("donuts")));
}

TEST_F(AnyValueDefaultConversionTest, PyObject)
{
  py::object i = py::int_(42);
  auto v = qi::AnyValue::from(i);
  EXPECT_EQ(qi::TypeKind_Dynamic, v.kind());
  EXPECT_TRUE(v.to<py::object>().equal(py::int_(42)));
}

TEST_F(AnyValueDefaultConversionTest, PyList)
{
  std::vector<int> values = {1, 2, 4, 8, 16};
  const py::list l = py::cast(values);
  auto v = qi::AnyValue::from(l);
  EXPECT_EQ(qi::TypeKind_Dynamic, v.kind());
  EXPECT_TRUE(v.to<py::list>().equal(l));

  auto vc = v.content();
  const auto vcVec = vc.asListValuePtr();

  std::vector<int> res;
  std::transform(vcVec.begin(), vcVec.end(), std::back_inserter(res),
                 [](const qi::AnyReference& v) { return v.to<int>(); });
  EXPECT_EQ(values, res);
}

using DictTypes = testing::Types<py::dict, py::kwargs>;
template <typename T>
struct AnyValueDefaultConversionDictTest : AnyValueDefaultConversionTest {};
TYPED_TEST_SUITE(AnyValueDefaultConversionDictTest, DictTypes);

TYPED_TEST(AnyValueDefaultConversionDictTest, Dict)
{
  using Dict = TypeParam;
  std::map<std::string, int> values = {{ "two", 2 },
                                       { "four", 4 },
                                       { "eight", 8 },
                                       { "sixteen", 16 }};
  const Dict d = py::cast(values);
  auto v = qi::AnyValue::from(d);
  EXPECT_EQ(qi::TypeKind_Dynamic, v.kind());
  EXPECT_TRUE(v.template to<Dict>().equal(d));

  auto vc = v.content();
  const auto vcMap = vc.asMapValuePtr();
  std::map<std::string, int> res;
  std::transform(vcMap.begin(), vcMap.end(), std::inserter(res, res.begin()),
                 [](const std::pair<qi::AnyReference, qi::AnyReference>& v) {
                   return std::make_pair(v.first.to<std::string>(), v.second.to<int>());
                 });
  EXPECT_EQ(values, res);
}

using TupleTypes = testing::Types<py::tuple, py::args>;
template <typename T>
struct AnyValueDefaultConversionTupleTest : AnyValueDefaultConversionTest {};
TYPED_TEST_SUITE(AnyValueDefaultConversionTupleTest, TupleTypes);

TYPED_TEST(AnyValueDefaultConversionTupleTest, Tuple)
{
  using Tuple = TypeParam;
  const std::tuple<int, std::string, std::int8_t> values(21, "cookies", 's');
  const Tuple t = py::cast(values);
  auto v = qi::AnyValue::from(t);
  EXPECT_EQ(qi::TypeKind_Dynamic, v.kind());
  EXPECT_TRUE(v.template to<Tuple>().equal(t));

  auto vc = v.content();
  const auto vcMap = vc.asTupleValuePtr();
  const auto res =
    std::make_tuple(vcMap[0].template to<int>(), vcMap[1].toString(),
                    vcMap[2].template to<std::int8_t>());
  EXPECT_EQ(values, res);
}

struct TypePassing : qi::py::test::Execute,
                     testing::Test
{
protected:
  virtual void SetUp()
  {
    session = qi::makeSession();
    session->listenStandalone("tcp://127.0.0.1:0");

    py::gil_scoped_acquire lock;
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

