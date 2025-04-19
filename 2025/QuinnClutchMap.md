#visualization for the clutch function
import numpy as np
zero_bite = 0.145
pulled_bite = 0.955
def clutch(x):
    if (x < zero_bite):
        return 165
    elif (x > pulled_bite):
        return 26
    else:
        return -50.7697 * np.power(x,3) + 120.513 * np.power(x,2) - 96.6814 * x + 136.64
x = np.linspace(0, 1, 1000)
y = [clutch(i) for i in x]
import matplotlib.pyplot as plt
plt.plot(x, y)
plt.title(‘Clutch Function Visualization’)
plt.xlabel(‘Clutch Paddle Input [0,1](normalized analog input)’)
plt.ylabel(‘Clutch Servo Output [0,255](8 bit)’)
plt.grid()
plt.show()
