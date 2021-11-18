import matplotlib.pyplot as plt

f = open("log.txt", "r").readlines()
duration_list = []
for ff in f:
  val = float(ff.rstrip()) / 1000.0
  duration_list.append(val)

plt.subplot(121)
plt.plot(range(len(duration_list)), duration_list, "-*", )

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
plt.plot(x_val, y_val)
plt.show()
