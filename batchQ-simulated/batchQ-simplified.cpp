// command to run: g++ batchQ-simplified.cpp -lpthread
//                 ./a.out
// ====================================================================================
// The logic of the script is to mimic the process of batching inference. The script
// is able to simulate the actual runtime under various batch size, parallel level
// and batch duration. So we can verify the hypothesis of Lwc = (1+1/p)d.
// For each of the function:
//   request_generation() will push request (in this example, frame_id only) to request_queue according to REQUEST_RATE.
//   batch_generation() will extract a batch of request and push batched_request to batched_request_queue.
//   process_batch() will fetch batched_request from batched_request_queue and execute it (in this example, just sleep for BATCH_DURATION).
// log_start and log_end will record the start and end timestamp of each request, so we can verify the above hypothesis.
// ====================================================================================

#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable> // std::condition_variable
#include <vector>
#include <map>
#include <csignal>
#include <algorithm>
#include <fstream>

#define TOTAL_FRAME 8192      // total frame that will be sent
#define BATCH_SIZE 8          // batch size
#define BATCH_DURATION 100    // the duration that each batch lasts in milliseconds
#define NUM_OF_WORKER 4       // number of worker that will execute batched requests in parallel
#define REQUEST_RATE float(BATCH_SIZE) * NUM_OF_WORKER / (float(BATCH_DURATION) / 1000.0)     // here, we assume the processing speeed matches the request rate, leading to perfect line rate

// for sync between request_generation() and batch_generation()
std::mutex req_lock;
std::condition_variable req_cv;

// for sync between batch_generation() and process_batch()
std::mutex process_lock;
std::condition_variable process_cv;

std::vector<int> request_queue;
std::vector<std::vector<int>> batched_request_queue;

std::map<int, std::chrono::time_point<std::chrono::high_resolution_clock>> log_start;
std::map<int, std::chrono::time_point<std::chrono::high_resolution_clock>> log_end;

auto get_time() {
  return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void request_generation() {
  int frame_id = 0;

  auto t1 = std::chrono::high_resolution_clock::now();
  auto cur = std::chrono::high_resolution_clock::now();
  auto last_cur = cur;
  std::chrono::duration<double, std::milli> diff_time;

  while (frame_id < TOTAL_FRAME) {
    auto start_time = std::chrono::high_resolution_clock::now();
    log_start[frame_id] = start_time;

    last_cur = cur;

    // push request to request_queue
    std::unique_lock<std::mutex> req_lk(req_lock);
    request_queue.push_back(frame_id);

    req_lk.unlock();
    req_cv.notify_one();

    // Make sure we generate request at REQUEST_RATE.
    // Note that the method here will lead to 100% CPU util at one CPU core,
    // but it is much more accurate than sleep_for().
    cur = std::chrono::high_resolution_clock::now();
    diff_time = cur - last_cur;
    while (diff_time.count() < (1000.0 / float(REQUEST_RATE))) {
      cur = std::chrono::high_resolution_clock::now();
      diff_time = cur - last_cur;
    }

    frame_id += 1;
  }

  auto t2 = std::chrono::high_resolution_clock::now();
  diff_time = t2 - t1;

  std::cout << "End-to-end sending fps = " << TOTAL_FRAME / (diff_time.count() / 1000.0) << " with duration of " << diff_time.count() << " ms." << std::endl;
}

void batch_generation() {
  while (true) {
    std::unique_lock<std::mutex> req_lk(req_lock);
    req_cv.wait(req_lk, []{
      return (request_queue.size() >= BATCH_SIZE);
    });
    std::cout << "[" << get_time() << "] Get enough req in request_queue!" << std::endl;
    std::vector<int> batched_request;
    for (int i = 0; i < BATCH_SIZE; i++) {
      int frame_id = request_queue.front();
      batched_request.push_back(frame_id);
      request_queue.erase(request_queue.begin());
    }
    batched_request_queue.push_back(batched_request);

    req_lk.unlock();
    process_cv.notify_one();
  }
}

void process_batch(int tid) {
  // For Method II
  std::chrono::time_point<std::chrono::high_resolution_clock> t1;
  std::chrono::time_point<std::chrono::high_resolution_clock> cur;
  std::chrono::duration<double, std::milli> diff_time;

  while (true) {
    std::unique_lock<std::mutex> process_lk(process_lock);
    process_cv.wait(process_lk, []{
      return (batched_request_queue.size() > 0);
    });
    auto batched_request = batched_request_queue.front();
    batched_request_queue.erase(batched_request_queue.begin());

    process_lock.unlock();

    // // Method I
    // std::this_thread::sleep_for(std::chrono::milliseconds(BATCH_DURATION));

    // Method II
    // Again this method will lead to 100% util for one CPU core.
    t1 = std::chrono::high_resolution_clock::now();
    cur = std::chrono::high_resolution_clock::now();
    diff_time = cur - t1;
    while (diff_time.count() < BATCH_DURATION) {
      cur = std::chrono::high_resolution_clock::now();
      diff_time = cur - t1;
    }

    auto end_time = std::chrono::high_resolution_clock::now();

    std::cout << "[tid = " << tid << "] finished (";
    for (auto frame_id : batched_request) {
      std::cout << frame_id << ", ";
      log_end[frame_id] = end_time;
    }
    std::cout << ")" << std::endl;
  }
}

void signalHandler(int signum) {
  std::cout << "\nInterrupt signal (" << signum << ") received." << std::endl;

  std::chrono::duration<double, std::micro> duration;
  std::vector<int> duration_array;
  for (int frame_id = 0; frame_id < TOTAL_FRAME; frame_id++) {
    duration = log_end[frame_id] - log_start[frame_id];
    duration_array.push_back(duration.count());
  }

  double min_val = *std::min_element(duration_array.begin(), duration_array.end());
  double max_val = *std::max_element(duration_array.begin(), duration_array.end());
  std::cout << "Duration min = " << min_val << ", max = " << max_val << std::endl;

  std::ofstream myoutput;
  myoutput.open("log.txt");
  for (int frame_id = 0; frame_id < TOTAL_FRAME; frame_id++) {
    myoutput << duration_array[frame_id] << std::endl;
  }
  myoutput.close();

  exit(signum);
}

int main() {
  // start thread for batch_generation()
  std::thread bg_thread(batch_generation);

  // start NUM_OF_WORKER threads for process_batch()
  std::thread pb_thread_array[NUM_OF_WORKER];
  for (int tid = 0; tid < NUM_OF_WORKER; tid++) {
    pb_thread_array[tid] = std::thread(process_batch, tid);
  }

  // start thread for request_generation()
  std::thread rg_thread(request_generation);

  signal(SIGINT, signalHandler);
  std::this_thread::sleep_for(std::chrono::seconds(24 * 60 * 60));
}
