import python_csv
import python_plot
import python_fft
import matplotlib.pyplot as plt
import numpy as np

def moving_average(time, data, window_size):

    filtered_data = [sum(data[i:i + window_size]) / window_size for i in range(len(data) - window_size + 1)]
    filtered_time = time[:len(filtered_data)]
    
    return filtered_time, filtered_data

def iir_low_pass(signal, A):
    
    B = 1 - A
    filtered = [signal[0]]  # initialize with the first value
    for i in range(1, len(signal)):
        filtered.append(A * filtered[-1] + B * signal[i])
    return filtered

def fir_filter(d, weights):
   
    return np.convolve(d, weights, mode='same')




# Q4.
t, d = python_csv.read_csv('sigB.csv')
frq, Y = python_fft.fft_and_plot(t, d)

# Q5.
# window_size = 200
# fd_t, fd_d = moving_average(t, d, window_size)
# fd_frq, fd_Y = python_fft.fft_and_plot(fd_t, fd_d)


# fig, (ax1, ax2) = plt.subplots(2, 1)
# ax1.plot(t, d,'black')
# ax1.plot(fd_t, fd_d,'red')
# ax1.set_xlabel('Time')
# ax1.set_ylabel('Amplitude')

# ax2.loglog(frq, abs(Y),'black') # plotting the fft
# ax2.loglog(fd_frq, fd_Y,'red') # plotting the fft
# ax2.set_xlabel('Freq (Hz)')
# ax2.set_ylabel('|Y(freq)|')
# fig.suptitle(f'{window_size} data points averaged')
# plt.show()

# Q6.
A = 0.95
fd_d = iir_low_pass(d, A)
fd_t = t
fd_frq, fd_Y = python_fft.fft_and_plot(fd_t, fd_d)


fig, (ax1, ax2) = plt.subplots(2, 1)
ax1.plot(t, d,'black')
ax1.plot(fd_t, fd_d,'red')
ax1.set_xlabel('Time')
ax1.set_ylabel('Amplitude')

ax2.loglog(frq, abs(Y),'black') # plotting the fft
ax2.loglog(fd_frq, fd_Y,'red') # plotting the fft
ax2.set_xlabel('Freq (Hz)')
ax2.set_ylabel('|Y(freq)|')
fig.suptitle(f'A:{A:.2f} and B:{1-A:.1f}')
plt.show()

# Q7.
# weight_D = python_fft.weight_D
# fd_d = fir_filter(d, weight_D)
# fd_t = t
# fd_frq, fd_Y = python_fft.fft_and_plot(fd_t, fd_d)


# fig, (ax1, ax2) = plt.subplots(2, 1)
# ax1.plot(t, d,'black')
# ax1.plot(fd_t, fd_d,'red')
# ax1.set_xlabel('Time')
# ax1.set_ylabel('Amplitude')

# ax2.loglog(frq, abs(Y),'black') # plotting the fft
# ax2.loglog(fd_frq, fd_Y,'red') # plotting the fft
# ax2.set_xlabel('Freq (Hz)')
# ax2.set_ylabel('|Y(freq)|')
# fig.suptitle(f'cutoff frq: 12hz and bandwidth: 4')
# plt.show()
