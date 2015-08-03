#include <Python.h>
#include <gtest/gtest.h>
#include <qi/anyobject.hpp>
#include <qi/session.hpp>
#include <qi/jsoncodec.hpp>
#include <qipython/gil.hpp>
#include <qipython/error.hpp>
#include <qipython/pysession.hpp>
#include <boost/thread.hpp>

qiLogCategory("test_typepassing");

namespace py = qi::py;

class TypePassing : public testing::Test
{
protected:
  virtual void SetUp()
  {
    session = qi::makeSession();
    session->listenStandalone("tcp://127.0.0.1:0");

    {
      py::GILScopedLock _lock;
      myExec(
          "def setSession(session):\n"
          "    global sd\n"
          "    sd = session\n"
          );

      boost::python::object setSession = boost::python::import("__main__").attr("setSession");
      setSession(qi::py::makePySession(session));
    }
  }

  virtual void TearDown()
  {
    {
      py::GILScopedLock _lock;
      myExec("del sd\n");
    }
    session->close();
  }

  void myExec(const std::string& str)
  {
    boost::python::object globals = boost::python::import("__main__").attr("__dict__");
    boost::python::exec(str.c_str(), globals, globals);
  }

  void registerService()
  {
    py::GILScopedLock _lock;
    myExec(
        "m = TestService()\n"
        "sd.registerService('TestService', m)\n"
        );
  }

  qi::AnyObject getService()
  {
    return session->service("TestService");
  }

  qi::SessionPtr session;
};

TEST_F(TypePassing, Int)
{
  {
    py::GILScopedLock _lock;
    myExec(
        "class TestService:\n"
        "    def func(self):\n"
        "        return 42\n"
        );
  }
  registerService();
  ASSERT_EQ(42, getService().call<int>("func"));
}

//Long have been merged with Int
#if PY_MAJOR_VERSION == 2
TEST_F(TypePassing, Long)
{
  {
    py::GILScopedLock _lock;
    myExec(
        "class TestService:\n"
        "    def func(self):\n"
        "        return 42L\n"
        );
  }
  registerService();
  ASSERT_EQ(42, getService().call<int>("func"));
}
#endif

TEST_F(TypePassing, Bool)
{
  {
    py::GILScopedLock _lock;
    myExec(
        "class TestService:\n"
        "    def func(self):\n"
        "        return True\n"
        );
  }
  registerService();
  ASSERT_EQ(true, getService().call<bool>("func"));
}

TEST_F(TypePassing, Float)
{
  {
    py::GILScopedLock _lock;
    myExec(
        "class TestService:\n"
        "    def func(self):\n"
        "        return 2.5\n"
        );
  }
  registerService();
  ASSERT_EQ(2.5, getService().call<float>("func"));
}

TEST_F(TypePassing, String)
{
  {
    py::GILScopedLock _lock;
    myExec(
        "class TestService:\n"
        "    def func(self):\n"
        "        return 'can i help you?'\n"
        );
  }
  registerService();
  ASSERT_EQ("can i help you?", getService().call<std::string>("func"));
}

TEST_F(TypePassing, ByteArray)
{
  {
    py::GILScopedLock _lock;
    myExec(
        "class TestService:\n"
        "    def func(self):\n"
        "        return bytearray('can i help you?', encoding='ascii')\n"
        );
  }
  registerService();
  ASSERT_EQ("can i help you?", getService().call<std::string>("func"));
}

TEST_F(TypePassing, List)
{
  {
    py::GILScopedLock _lock;
    myExec(
        "class TestService:\n"
        "    def func(self):\n"
        "        return [1, 2, 3]\n"
        );
  }
  registerService();
  std::vector<int> expected;
  expected.push_back(1);
  expected.push_back(2);
  expected.push_back(3);
  ASSERT_EQ(expected, getService().call<std::vector<int> >("func"));
}

TEST_F(TypePassing, Dict)
{
  {
    py::GILScopedLock _lock;
    myExec(
        "class TestService:\n"
        "    def func(self):\n"
        "        return {'one' : 1, 'two' : 2, 'three' : 3}\n"
        );
  }
  registerService();
  std::map<std::string, int> expected;
  expected["one"] = 1;
  expected["two"] = 2;
  expected["three"] = 3;
  ASSERT_EQ(expected, (getService().call<std::map<std::string, int> >("func")));
}

TEST_F(TypePassing, Recursive)
{
  {
    py::GILScopedLock _lock;
    myExec(
        "class TestService:\n"
        "    def func(self):\n"
        "        return {'one' : 1, 'two' : [1, 2], 'three' : {42 : 'answer'}}\n"
        );
  }
  registerService();
  qi::AnyValue v = getService().call<qi::AnyValue>("func");
  std::map<std::string, qi::AnyValue> map = v.to<std::map<std::string, qi::AnyValue> >();
  ASSERT_EQ(3, map.size());
  ASSERT_EQ(1, map["one"].toInt());
  ASSERT_EQ(1, (*map["two"])[0].toInt());
  ASSERT_EQ(2, (*map["two"])[1].toInt());
  std::map<int, std::string> map2 = (*map["three"]).to<std::map<int, std::string> >();
  ASSERT_EQ(1, map2.size());
  ASSERT_EQ("answer", map2[42]);
}

TEST_F(TypePassing, ReverseDict)
{
  {
    py::GILScopedLock _lock;
    myExec(
        "class TestService:\n"
        "    def func(self, dict):\n"
        "        return dict == {'one' : 1, 'two' : 2, 'three' : 3}\n"
        );
  }
  registerService();
  std::map<std::string, int> expected;
  expected["one"] = 1;
  expected["two"] = 2;
  expected["three"] = 3;
  ASSERT_TRUE(getService().call<bool>("func", expected));
}

PyThreadState *mainstate;

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  if (argc < 3)
  {
    std::cerr << "Usage: " << argv[0] << " <sdk path> <src path> [pythonhome]" << std::endl;
    return 1;
  }

  std::string sdk_dir = argv[1];
  std::string src_dir = argv[2];

  if (argc >= 4)
    qi::os::setenv("PYTHONHOME", argv[3]);

  Py_Initialize();
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
