import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Read data from CSV file
csv_file = "pitot_data.csv"  # Update with your actual filename
try:
    df = pd.read_csv(csv_file)
except FileNotFoundError:
    print(f"Error: File {csv_file} not found!")
    exit()

# Predicted calibration points for the pitot tube
calibration_data = [
    (0, 0.5),  # 0 mbar corresponds to 0.5V
    (13.04, 4.5)  # 13.04 mbar corresponds to 4.5V
]
sorted_calibration = sorted(calibration_data, key=lambda x: x[1])
cal_mbar = [p for p, v in sorted_calibration]
cal_v = [v for p, v in sorted_calibration]

# Linear fit parameters (y = mx + b)
m = (cal_mbar[1] - cal_mbar[0]) / (cal_v[1] - cal_v[0])  # Slope
b = cal_mbar[0] - m * cal_v[0]  # Intercept

# Create plot
plt.figure(figsize=(12, 8))

# Plot calibration data and predicted line
plt.plot(cal_v, cal_mbar, 'g-', label='Calibration Curve', linewidth=2, alpha=0.7)
plt.scatter(cal_v, cal_mbar, color='red', s=80, label='Calibration Points', zorder=3)
V_fit = np.linspace(min(cal_v), max(cal_v), 100)
plt.plot(V_fit, m * V_fit + b, 'b--', label=f'Linear Fit: P = {m:.2f}V + {b:.2f}', linewidth=2)

# Plot sensor data from CSV
if 'Voltage (V)' in df.columns and 'Pressure (mbar)' in df.columns:
    plt.scatter(df['Voltage (V)'], df['Pressure (mbar)'], 
                color='#1f77b4', s=40, alpha=0.7, label='Sensor Data')
else:
    print("Error: Required columns 'Voltage (V)' or 'Pressure (mbar)' not found in CSV file.")

# Formatting
plt.title('Pitot Tube Voltage vs Pressure', fontsize=14)
plt.xlabel('Voltage (V)', fontsize=12)
plt.ylabel('Pressure (mbar)', fontsize=12)
plt.grid(True, linestyle='--', alpha=0.5)
plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
plt.tight_layout()

# Set axis limits based on calibration data
plt.xlim(min(cal_v) - 0.1, max(cal_v) + 0.1)
plt.ylim(min(cal_mbar) - 1, max(cal_mbar) + 1)

# Add calibration point labels
for v, p in zip(cal_v, cal_mbar):
    plt.text(v, p, f'({v:.2f}V, {p:.2f}mbar)', fontsize=8, 
             ha='left', va='bottom', rotation=45)

plt.show()
