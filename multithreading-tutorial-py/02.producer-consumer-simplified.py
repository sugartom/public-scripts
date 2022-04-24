import time
import threading
import queue

NUM_PRODUCER = 2
NUM_CONSUMER = 4

def producer(tid, queue):
  pass



def consumer(tid, queue):
  pass

if __name__ == "__main__":

  format = "%(asctime)s: %(message)s"
  logging.basicConfig(format=format, level=logging.INFO,
                      datefmt="%H:%M:%S")

  myQueue = queue.Queue()

  t_producer = []
  for i in range(NUM_PRODUCER):
    t = threading.Thread(target = producer, args = (i, myQueue))
    t.start()
    t_producer.append(t)

  t_consumer = []
  for i in range(NUM_CONSUMER):
    t = threading.Thread(target = consumer, args = (i, myQueue))
    t.start()
    t_producer.append(t)

  for i in range(NUM_PRODUCER):
    t_producer[i].join()

  for i in range(NUM_CONSUMER):
    t_consumer[i].join()
