//
// Created by eugene on 14/05/2021.
//

#ifndef QNTOOLS_INCLUDE_BASE_QNTOOLS_KAHANSUM_HPP
#define QNTOOLS_INCLUDE_BASE_QNTOOLS_KAHANSUM_HPP

#include <Rtypes.h>

namespace Qn {

template <typename T>
class KahanSum {
 private:
  T sum_{0.};
  T raw_sum_{0.};
  T correction_{0.};

 public:
  KahanSum() = default;
  KahanSum(T value) : sum_(value) {}
  operator T () const {
      return sum_;
  }

  T GetSum() const { return sum_; }
  T GetCorrection() const { return correction_; }
  void Fill(T value) {
    auto y = value - correction_;
    auto t = sum_ + value;
    correction_ = (t - sum_) - y;
    sum_ = t;
    raw_sum_ += value;
  }

  ClassDefT(KahanSum, 2);
};

using KahanSumD = KahanSum<double>;
using KahanSumF = KahanSum<float>;

}  // namespace Qn

#endif  // QNTOOLS_INCLUDE_BASE_QNTOOLS_KAHANSUM_HPP
