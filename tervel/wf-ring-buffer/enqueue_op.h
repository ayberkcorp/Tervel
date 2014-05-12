#ifndef TERVEL_WFRB_ENQUEUEOP_H_
#define TERVEL_WFRB_ENQUEUEOP_H_

#include "tervel/util/memory/hp/hp_element.h" // REVIEW(steven) not needed
#include "tervel/wf-ring-buffer/buffer_op.h"
#include "tervel/wf-ring-buffer/wf_ring_buffer.h"


#include <algorithm>
#include <atomic>
#include <cstdint>

namespace tervel {
namespace wf_ring_buffer {

template<class T>
class RingBuffer;

template<class T>
class BufferOp;
/**
 * Class used for placement in the Op Table to complete an operation that failed
 *    to complete in a bounded number of steps
 */
template<class T>
class EnqueueOp : public BufferOp<T> {
 public:

  explicit EnqueueOp<T>(RingBuffer<T> *buffer, T value)
        : BufferOp<T>(buffer)
        , val_(value) {}

  ~EnqueueOp<T>() {}

  /**
   * This function overrides the virtual function in the OpRecord class
   * It is called by the progress assurance scheme.
   */
  void help_complete() {
    this->buffer_->wf_enqueue(this);
  }

  // REVIEW(steven) missing description
  T value() { return val_; }

 private:
  T val_;
};  // EnqueueOp class

}  // namespace wf_ring_buffer
}  // namespace tervel

#endif  // TERVEL_WFRB_ENQUEUEOP_H_
