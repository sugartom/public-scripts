import matplotlib.pyplot as plt
import logging
import time
import threading
from queue import Queue

_ONE_DAY_IN_SECONDS = 60 * 60 * 24

TOTAL_FRAME = 256
BATCH_SIZE = 4
BATCH_DURATION = 0.1
NUM_OF_WORKER = 1
REQUEST_RATE = float(BATCH_SIZE) * NUM_OF_WORKER / BATCH_DURATION

log_start = dict()
log_end = dict()
log_duration = dict()

def request_generation(request_queue):
  frame_id = 0

  while (frame_id < TOTAL_FRAME):
    start_time = time.time()
    log_start[frame_id] = start_time

    request_queue.put(frame_id)

    frame_id += 1

    diff = time.time() - start_time
    while (diff < (1.0 / REQUEST_RATE)):
      diff = time.time() - start_time

def batch_generation(request_queue, batched_request_queue):
  while (True):
    if (request_queue.qsize() >= BATCH_SIZE):
      batched_request = []

      for i in range(BATCH_SIZE):
        batched_request.append(request_queue.get())
        # request_queue.task_done()

      batched_request_queue.put(batched_request)

def process_batch(tid, batched_request_queue):
  while (True):
    batched_request = batched_request_queue.get()
    tt = time.time()

    # simulate batched request execution time

    # # Method I
    # time.sleep(BATCH_DURATION)

    # Method II
    diff_time = time.time() - tt
    while (diff_time < BATCH_DURATION):
      diff_time = time.time() - tt

    logging.info("[tid = %3d] finished %s" % (tid, batched_request))

    end_time = time.time()
    for frame_id in batched_request:
      log_end[frame_id] = end_time

    batched_request_queue.task_done()

if __name__ == "__main__":
  format = "%(asctime)s.%(msecs)03d: %(message)s"
  logging.basicConfig(format = format, level = logging.INFO, datefmt = "%H:%M:%S")

  request_queue = Queue()
  batched_request_queue = Queue()

  bg_thread = threading.Thread(target = batch_generation, args = (request_queue, batched_request_queue))
  bg_thread.daemon = True
  bg_thread.start()

  for tid in range(NUM_OF_WORKER):
    pb_thread = threading.Thread(target = process_batch, args = (tid, batched_request_queue))
    pb_thread.daemon = True
    pb_thread.start()

  try:
    input("Press Enter to continue...\n")

    rg_thread = threading.Thread(target = request_generation, args = (request_queue,))
    rg_thread.daemon = True
    rg_thread.start()

    while True:
      time.sleep(float(TOTAL_FRAME) / REQUEST_RATE + 2.0)
      print("\nDone. Please press Ctrl + C to see output.")
      time.sleep(_ONE_DAY_IN_SECONDS)

  except KeyboardInterrupt:
    print("\nEnd by keyboard interrupt...\n")

    duration_list = []

    for frame_id in range(TOTAL_FRAME):
      log_duration[frame_id] = log_end[frame_id] - log_start[frame_id]
      duration_list.append(log_duration[frame_id])

    plt.subplot(121)
    plt.plot(range(len(duration_list)), duration_list, "-*", label = "b = %d, p = %d, d = %.3f" % (BATCH_SIZE, NUM_OF_WORKER, BATCH_DURATION))

    duration_list.sort()

    print("Duration min = %.3f, max = %.3f" % (min(duration_list), max(duration_list)))

    x_val = []
    y_val = []

    prev = duration_list[0]
    for i in range(len(duration_list)):
      cur = duration_list[i]
      if (cur != prev):
        x_val.append(prev)
        y_val.append(float(i) / len(duration_list))
        prev = cur

    x_val.append(duration_list[-1])
    y_val.append(1.0)

    plt.subplot(122)
    plt.plot(x_val, y_val, label = "b = %d, p = %d, d = %.3f" % (BATCH_SIZE, NUM_OF_WORKER, BATCH_DURATION))
    plt.show()
