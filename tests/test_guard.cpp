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

using GuardedTest = GuardTest;

TEST_F(GuardedTest, GuardsConstruction)
{
  using T = GuardedTest;

  struct Check
  {
    int i;
    Check(int i) : i(i) { EXPECT_TRUE(T::guarded); }
    explicit operator bool() const { return true; }
  };

  EXPECT_FALSE(T::guarded);
  qi::py::detail::Guarded<SetFlagGuard, Check> g(42);
  EXPECT_FALSE(T::guarded);
  EXPECT_EQ(42, g->i);
}

TEST_F(GuardedTest, GuardsCopyConstruction)
{
  using T = GuardedTest;

  struct Check
  {
    int i = 0;
    Check(int i) : i(i) {}
    Check(const Check& o)
      : i(o.i)
    { EXPECT_TRUE(T::guarded); }
    explicit operator bool() const { return true; }
  };

  EXPECT_FALSE(T::guarded);
  qi::py::detail::Guarded<SetFlagGuard, Check> a(42);
  qi::py::detail::Guarded<SetFlagGuard, Check> b(a);
  EXPECT_FALSE(T::guarded);
  EXPECT_EQ(42, a->i);
  EXPECT_EQ(42, b->i);
}

TEST_F(GuardedTest, GuardsCopyAssignment)
{
  using T = GuardedTest;

  struct Check
  {
    int i = 0;
    Check(int i) : i(i) {}
    Check& operator=(const Check& o)
    { EXPECT_TRUE(T::guarded); i = o.i; return *this; }
    explicit operator bool() const { return true; }
  };

  EXPECT_FALSE(T::guarded);
  qi::py::detail::Guarded<SetFlagGuard, Check> a(42);
  qi::py::detail::Guarded<SetFlagGuard, Check> b(37);
  EXPECT_FALSE(T::guarded);
  b = a;
  EXPECT_FALSE(T::guarded);
  EXPECT_EQ(42, a->i);
  EXPECT_EQ(42, b->i);
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
