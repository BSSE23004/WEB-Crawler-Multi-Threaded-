import matplotlib.pyplot as plt

threads = [1, 2, 4, 8]
times = [20.768, 11.943, 5.139, 4.377] 

plt.plot(threads, times, 'bo-')
plt.xlabel('Number of Threads')
plt.ylabel('Total Time (s)')
plt.title('Thread Count vs. Completion Time')
plt.savefig('performance.png')