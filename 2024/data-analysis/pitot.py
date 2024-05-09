import pandas as pd
import matplotlib.pyplot as plt



# Read the CSV file
df = pd.read_csv('WUFR 23_WAP 23_a_0833.csv')
df_filtered = df[['Time', 'FAnlg19']]

# Filter the data
for i in range(4, len(df)):
    diff = abs(df_filtered['FAnlg19'][i] - df_filtered['FAnlg19'][i - 4])
    if diff > 6:
        for j in range(4):
            df_filtered.loc[i - j, 'FAnlg19'] = df_filtered['FAnlg19'][i - 4]
        # df_filtered.loc[i, 'FAnlg19'] = df_filtered['FAnlg19'][i - 1]

# Convert to PSI
df['FAnlg19'] = df['FAnlg19'] * 0.00045 - 0.0225
df_filtered['FAnlg19'] = df_filtered['FAnlg19'] * 0.00045 - 0.0225

# Convert to pascals
df['FAnlg19'] = df['FAnlg19'] * 6894.76
df_filtered['FAnlg19'] = df_filtered['FAnlg19'] * 6894.76

# Convert to velocity (m/s)
df['FAnlg19'] = (2 * df['FAnlg19'] / 1.185) ** 0.5
df_filtered['FAnlg19'] = (2 * df_filtered['FAnlg19'] / 1.185) ** 0.5

# Convert to mph
df['FAnlg19'] = df['FAnlg19'] * 2.23694
df_filtered['FAnlg19'] = df_filtered['FAnlg19'] * 2.23694

# Plot FAnlg19 over Time
plt.plot(df['Time'], df['FAnlg19'],
         df_filtered['Time'], df_filtered['FAnlg19'])


# Set the labels and title
plt.xlabel('Time (s)')
plt.ylabel('Pitot (pascals)')
plt.title('Pitot over Time')


# Show the plot
plt.show()
