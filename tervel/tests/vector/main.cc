//
//  main_tester.c
//  mCAS
//
//  Created by Steven Feldman on 11/1/12.

//  Copyright (c) 2012 Steven FELDMAN. All rights reserved.
//

// #define KILL_THREAD 1
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include <sys/time.h>
#include <time.h>
#include <vector>
#include <thread>

#include <iostream>
#include <atomic>

#include <boost/random.hpp>
#include <boost/generator_iterator.hpp>
#include <gflags/gflags.h>

#include "container_api.h"

DEFINE_int32(capacity, 64, "The initial capacity of the hash map");
DEFINE_int32(num_threads, 1, "The number of executing threads.");
DEFINE_int32(execution_time, 5, "The amount of time to run the tests");

class TestObject {
 public:
  TestObject(int num_threads, int capacity, int execution_time)
      : test_class_(num_threads, capacity)
      , num_threads_(num_threads)
      , execution_time_(execution_time) {}

  void print_test() {
    printf("Not Implemented...\n");
  };

  void print_results() {
#ifdef DEBUG
    printf("Debug Print Results Not Implemented...\n");
#else
    printf("Print Results mostly not Implemented...\n");
    std::cout << "Summed " << summed_.load() << std::endl;
#endif
  }

  std::atomic<int> ready_count_{0};
  std::atomic<bool> running_{true};
  std::atomic<bool> wait_flag_{true};
  std::atomic<int64_t> summed_{0};

  TestClass<int64_t> test_class_;
  const int num_threads_;
  const int execution_time_;
};


void run(int64_t thread_id, TestObject * test_object);

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  TestObject test_data(FLAGS_num_threads+1, FLAGS_capacity,
      FLAGS_execution_time);

#ifdef DEBUG
  test_data.print_test();
#endif


  std::vector<std::thread> thread_list;
  for (int64_t i = 0; i < FLAGS_num_threads; i++) {
    std::thread temp_thread(run, i, &test_data);
    thread_list.push_back(std::move(temp_thread));
  }

  while (test_data.ready_count_.load() < FLAGS_num_threads) {}

#ifdef DEBUG
  printf("Beginning Test.\n");
#endif
  test_data.wait_flag_.store(false);
  std::this_thread::sleep_for(std::chrono::seconds(test_data.execution_time_));
  test_data.ready_count_.store(0);
  test_data.running_.store(false);
  std::this_thread::sleep_for(std::chrono::seconds(1));

  while (test_data.ready_count_.load() < FLAGS_num_threads) {}
  test_data.wait_flag_.store(false);

  std::this_thread::sleep_for(std::chrono::seconds(test_data.execution_time_));
  std::this_thread::sleep_for(std::chrono::seconds(1));
#ifdef DEBUG
  printf("Signaled Stop!\n");
#endif

  std::for_each(thread_list.begin(), thread_list.end(), [](std::thread &t)
    { t.join(); });

  test_data.print_results();

  return 0;
}

void run(int64_t thread_id, TestObject * test_data) {
  test_data->test_class_.attach_thread();

  test_data->ready_count_.fetch_add(1);

  int64_t i, temp, total_added;
  int lcount = 0;
  total_added = 0;
  for (i = thread_id << 50; test_data->running_.load(); i += 0x10) {
    assert((i & 0x1) == 0);

    std::cout << thread_id << " " << (void *)i << std::endl;

    size_t pos = test_data->test_class_.push_back(i);
    temp = -1;
    bool res = test_data->test_class_.at(pos, temp);
    assert(res && temp == i);

    assert(((temp+0x10) & 0x1) == 0);
    res = test_data->test_class_.cas(pos, temp, temp+0x10);
    assert(res && temp == i);
    res = test_data->test_class_.at(pos, temp);
    assert(res && temp == i+0x10);
    total_added += temp;

    lcount++;
  }

  test_data->ready_count_.fetch_add(1);

  lcount = 0;
  int64_t total_removed = 0;
  while (test_data->test_class_.size() > 0) {
    temp = 0;
    bool res =  test_data->test_class_.pop_back(temp);
    if (res) {
      total_removed +=  temp;
    } else {
      assert(test_data->test_class_.size() == 0);
      break;
    }

    lcount++;
  }

  int64_t sumed = total_added - total_removed;

  test_data->summed_.fetch_add(sumed);


  test_data->test_class_.detach_thread();
}

