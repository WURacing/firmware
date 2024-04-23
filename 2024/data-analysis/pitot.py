import pandas as pd

import matplotlib.pyplot as plt

# def filter(data):
#     for i in range(len(data)):


# Read the CSV file
df = pd.read_csv('WUFR 23_WAP 23_a_0833.csv')

df['FAnlg19'] = df['FAnlg19'] * 0.00045 - 0.0225

# Plot FAnlg19 over Time
# plt.plot(df['Time'], df['FAnlg19'])
plt.scatter(df['Time'], df['FAnlg19'])


# Set the labels and title
plt.xlabel('Time (s)')
plt.ylabel('Pitot (psi)')
plt.title('Pitot over Time')


# Show the plot
plt.show()
