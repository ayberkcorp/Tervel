#ifndef __TERVEL_CONTAINERS_WF_VECTOR_VECTOR_ARRAY_ARRAY_H
#define __TERVEL_CONTAINERS_WF_VECTOR_VECTOR_ARRAY_ARRAY_H

#include <tervel/util/util.h>

#include <tervel/containers/wf/vector/vector_array.h>

namespace tervel {
namespace containers {
namespace wf {
namespace vector {

template<typename T>
class ArrayArray : public VectorArray<T> {
  typedef std::atomic<T> ArrayElement;
  typedef ArrayElement * ArraySegment;
 /**
  * This class contains code related to managing elements stored in the vector
  * It stores elements on a series of array segments, which are stored into a
  * global vector.
  *
  * TODO: Remove atomic type of array replace it with explicit atomic loads
  * This will remove unnecessary memory barriers since it is a single transition
  * type.
  *
  */
 public:
  ArrayArray(size_t capacity, T default_value = nullptr)
      : default_value_(default_value) {
    if (capacity < 2) {
      capacity = 2;
    }
    offset_pow_ = tervel::util::round_to_next_power_of_two(capacity);
    offset_ = std::pow(2, offset_pow_);

    array_of_arrays[0].store(allocate_array_segment(offset_));
    current_capacity_.store(offset_);
  }

  ~ArrayArray() {
    for (size_t i = 0; i < k_max_array_segments_; i++) {
      delete [] array_of_arrays[i];
    }
  }

  /**
   * This function adds an array segment to the arrays used to hold additional
   * elements
   * @param  pos the slot to place the new array segment
   * @return     the current array segment at the specified position
   */
  ArraySegment add_segment(const size_t pos) {
    ArraySegment cur_seg = array_of_arrays[pos].load();

    if (cur_seg == nullptr) {
      size_t seg_cap = 0x1 << (offset_pow_ + pos);
      ArraySegment new_seg = allocate_array_segment(seg_cap);

      if (array_of_arrays[pos].compare_exchange_strong(cur_seg, new_seg)) {
        current_capacity_.fetch_add(seg_cap);
        return new_seg;
      } else {
        delete [] new_seg;
        return cur_seg;
      }
    }  // if cur_seg == nullprt
    return cur_seg;
  };  // add_segment


  /**
   * This function returns the address of the specified position
   * @param raw_pos the position
   * @param no_add if true then it will not increase the vectors size
   * @return the address of the specified positon
   */
  ArrayElement * get_spot(const size_t raw_pos, const bool no_add = false) {
    if (raw_pos < offset_) {
      ArraySegment seg = array_of_arrays[0].load();
      return &(seg[raw_pos]);
    } else {
      static const int nobits = (sizeof(unsigned int) << 3) - 1;
      size_t pos = raw_pos + offset_;
      size_t num = nobits - __builtin_clz(pos);

      size_t elem_pos = pos ^ (1 << num);
      size_t seg_num = num - offset_pow_;

      ArraySegment seg = array_of_arrays[seg_num].load();
      if (seg == nullptr && !no_add) {
        seg = add_segment(seg_num);
        assert(seg != NULL);
      }

      if (seg != nullptr) {
        return &(seg[elem_pos]);
      } else {
        return nullptr;
      }
    }  // else it is first array
  }  // get_pos

  ArrayElement *allocate_array_segment(const size_t capacity) {
    ArrayElement * temp = new ArrayElement[capacity];
    for (uint64_t i = 0; i < capacity; i++) {
      temp[i] = default_value_;
    }
    return temp;
  }

  size_t capacity() {
    return current_capacity_.load();
  }

 private:
  const T default_value_ {nullptr};
  static const size_t k_max_array_segments_ {64};

  std::atomic<ArraySegment> array_of_arrays[k_max_array_segments_];
  size_t offset_, offset_pow_;

  std::atomic<size_t> current_capacity_;
};  // class Vector Array
}  // namespace vector
}  // namespace wf
}  // namespace containers
}  // namespace tervel

#endif  // __TERVEL_CONTAINERS_WF_VECTOR_VECTOR_
