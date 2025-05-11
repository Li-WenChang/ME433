import matplotlib.pyplot as plt # for plotting
import numpy as np # for sine function

def plot(t:list, s:list):
    plt.plot(t,s,'b-*')
    plt.xlabel('Time [s]')
    plt.ylabel('Signal')
    plt.title('Signal vs Time')
    plt.show()

