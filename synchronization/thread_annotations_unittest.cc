// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Tests of the static thread annotation macros. These fall into two categories,
// positive tests (testing that correct code compiles and works) and negative
// tests (testing that incorrect code does not compile).
//
// Unfortunately, we don't have systematic/automated negative compilation tests.
// So instead we have some cheesy macros that you can define to enable
// individual compilation failures.

#include "lib/ftl/synchronization/thread_annotations.h"

#include "gtest/gtest.h"
#include "lib/ftl/macros.h"
#include "lib/ftl/synchronization/mutex.h"

// Uncomment these to enable particular compilation failure tests.
// #define NC_GUARDED_BY
// TODO(vtl): |ACQUIRED_{BEFORE,AFTER}()| are currently unimplemented in clang
// as of 2015-07-06 ("To be fixed in a future update."). So this actually
// compiles!
// #define NC_ACQUIRED_BEFORE

namespace ftl {
namespace {

// Test FTL_GUARDED_BY ---------------------------------------------------------

class GuardedByClass {
 public:
  GuardedByClass() : x_() {}
  ~GuardedByClass() {}

  void GoodSet(int x) {
    mu_.Lock();
    x_ = x;
    mu_.Unlock();
  }

#ifdef NC_GUARDED_BY
  void BadSet(int x) { x_ = x; }
#endif

 private:
  Mutex mu_;
  int x_ FTL_GUARDED_BY(mu_);

  FTL_DISALLOW_COPY_AND_ASSIGN(GuardedByClass);
};

TEST(ThreadAnnotationsTest, GuardedBy) {
  GuardedByClass c;
  c.GoodSet(123);
}

// Test FTL_ACQUIRED_BEFORE ----------------------------------------------------

class AcquiredBeforeClass2;

class AcquiredBeforeClass1 {
 public:
  AcquiredBeforeClass1() {}
  ~AcquiredBeforeClass1() {}

  void NoOp() {
    mu_.Lock();
    mu_.Unlock();
  }

#ifdef NC_ACQUIRED_BEFORE
  void BadMethod(AcquiredBeforeClass2* c2);
#endif

 private:
  friend class AcquiredBeforeClass2;

  Mutex mu_;

  FTL_DISALLOW_COPY_AND_ASSIGN(AcquiredBeforeClass1);
};

class AcquiredBeforeClass2 {
 public:
  AcquiredBeforeClass2() {}
  ~AcquiredBeforeClass2() {}

  void NoOp() {
    mu_.Lock();
    mu_.Unlock();
  }

  void GoodMethod(AcquiredBeforeClass1* c1) {
    mu_.Lock();
    c1->NoOp();
    mu_.Unlock();
  }

 private:
  Mutex mu_ FTL_ACQUIRED_BEFORE(AcquiredBeforeClass1::mu_);

  FTL_DISALLOW_COPY_AND_ASSIGN(AcquiredBeforeClass2);
};

#ifdef NC_ACQUIRED_BEFORE
void AcquiredBeforeClass1::BadMethod(AcquiredBeforeClass2* c2) {
  mu_.Lock();
  c2->NoOp();
  mu_.Unlock();
}
#endif

TEST(ThreadAnnotationsTest, AcquiredBefore) {
  AcquiredBeforeClass1 c1;
  AcquiredBeforeClass2 c2;
  c2.GoodMethod(&c1);
#ifdef NC_ACQUIRED_BEFORE
  c1.BadMethod(&c2);
#endif
}

// TODO(vtl): Test more things.

}  // namespace
}  // namespace ftl
