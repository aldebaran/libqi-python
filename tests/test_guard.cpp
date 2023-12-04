/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#include <qipython/common.hpp>
#include <qipython/pyguard.hpp>
#include <gtest/gtest.h>

namespace
{
  struct GuardTest : testing::Test
  {
    static thread_local bool guarded;
    void SetUp() override { guarded = false; }
  };

  thread_local bool GuardTest::guarded = false;

  struct SetFlagGuard
  {
    SetFlagGuard()  { GuardTest::guarded = true; }
    ~SetFlagGuard() { GuardTest::guarded = false; }
  };
} // namespace

using InvokeGuardedTest = GuardTest;

TEST_F(InvokeGuardedTest, ExecutesFunctionWithGuard)
{
  using T = InvokeGuardedTest;

  EXPECT_FALSE(T::guarded);
  auto r = qi::py::invokeGuarded<SetFlagGuard>([](int i){
    EXPECT_TRUE(T::guarded);
    return i + 10;
  }, 42);
  EXPECT_FALSE(T::guarded);
  EXPECT_EQ(52, r);
}

TEST(GILAcquire, IsReentrant)
{
  qi::py::GILAcquire acq0; QI_IGNORE_UNUSED(acq0);
  qi::py::GILRelease rel;  QI_IGNORE_UNUSED(rel);
  qi::py::GILAcquire acq1; QI_IGNORE_UNUSED(acq1);
  qi::py::GILAcquire acq2; QI_IGNORE_UNUSED(acq2);
  SUCCEED();
}

TEST(GILRelease, IsReentrant)
{
  qi::py::GILRelease rel0; QI_IGNORE_UNUSED(rel0);
  qi::py::GILAcquire acq;  QI_IGNORE_UNUSED(acq);
  qi::py::GILRelease rel1; QI_IGNORE_UNUSED(rel1);
  qi::py::GILRelease rel2; QI_IGNORE_UNUSED(rel2);
  SUCCEED();
}

struct SharedObject : testing::Test
{
  SharedObject()
  {
    // GIL is only required for creation of the inner object.
    object = [&]{
      qi::py::GILAcquire lock;
      return pybind11::capsule(
        &this->destroyed,
        [](void* destroyed){
          *reinterpret_cast<bool*>(destroyed) = true;
        }
      );
    }();
  }

  ~SharedObject()
  {
    if (object) {
      qi::py::GILAcquire lock;
      object = {};
    }
  }

  bool destroyed = false;
  pybind11::capsule object;
};

TEST_F(SharedObject, KeepsRefCount)
{
  std::optional sharedObject = qi::py::SharedObject(std::move(object));
  ASSERT_FALSE(object); // object has been released.
  EXPECT_FALSE(destroyed); // inner object is still alive.
  {
    auto sharedObjectCpy = *sharedObject; // copy the shared object locally.
    EXPECT_FALSE(destroyed); // inner object is maintained by both copies.
    sharedObject.reset();
    EXPECT_FALSE(destroyed); // inner object is maintained by the copy.
  }
  EXPECT_TRUE(destroyed); // inner object has been destroyed.
}

TEST_F(SharedObject, TakeInnerStealsInnerRefCount)
{
  std::optional sharedObject = qi::py::SharedObject(std::move(object));
  auto inner = sharedObject->takeInner();
  EXPECT_FALSE(sharedObject->inner()); // inner object is null.
  sharedObject.reset();
  EXPECT_FALSE(destroyed); // inner object is still alive.
  qi::py::GILAcquire lock; // release local inner object, which requires the GIL.
  inner = {};
  EXPECT_TRUE(destroyed); // inner object has been destroyed.
}
