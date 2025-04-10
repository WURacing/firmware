import tkinter as tk
from tkinter import messagebox
import serial
import csv
import os
from threading import Thread, Lock

class SensorDataCollector:
    def __init__(self, port, baud_rate, csv_file):
        self.port = port
        self.baud_rate = baud_rate
        self.csv_file = csv_file
        self.running = False
        self.ser = None
        self.root = None
        self.latest_voltages = [None] * 6
        self.lock = Lock()
        self.header_written = os.path.exists(self.csv_file)

    def read_serial(self):
        while self.running:
            if self.ser and self.ser.is_open:
                try:
                    line = self.ser.readline().decode('utf-8').strip()
                    if line.startswith("Voltages: "):
                        # Parse voltage data
                        voltages = []
                        parts = line.replace("V", "").split()
                        for part in parts[1:]:  # Skip "Voltages:" part
                            try:
                                voltages.append(float(part))
                            except ValueError:
                                pass
                        if len(voltages) == 6:
                            with self.lock:
                                self.latest_voltages = voltages
                                print("Updated voltages:", voltages)
                except (serial.SerialException, UnicodeDecodeError) as e:
                    print(f"Serial error: {e}")

    def save_data(self, temp):
        with self.lock:
            voltages = self.latest_voltages.copy()
            
        if None in voltages:
            messagebox.showerror("Error", "No valid sensor data available")
            return
            
        with open(self.csv_file, 'a', newline='') as f:
            writer = csv.writer(f)
            if not self.header_written:
                writer.writerow(["Temp", "A0", "A1", "A2", "A3", "A4", "A5"])
                self.header_written = True
            writer.writerow([temp] + voltages)
        messagebox.showinfo("Saved", f"Temperature {temp}°F saved with voltages")

    def add_external_temp(self, entry_widget):
        try:
            temp = float(entry_widget.get())
            entry_widget.delete(0, tk.END)
            # Send temperature to Arduino
            if self.ser and self.ser.is_open:
                self.ser.write(f"{temp}\n".encode())
            self.save_data(temp)
        except ValueError:
            messagebox.showerror("Error", "Invalid temperature value")

    def stop_program(self):
        self.running = False
        if self.ser and self.ser.is_open:
            self.ser.write(b"END\n")  # Send END command to Arduino
            self.ser.close()
        if self.root:
            self.root.quit()

    def start(self):
        try:
            self.ser = serial.Serial(self.port, self.baud_rate, timeout=1)
            self.running = True
        except serial.SerialException as e:
            messagebox.showerror("Error", f"Could not open serial port: {e}")
            return

        # Create GUI
        self.root = tk.Tk()
        self.root.title("Temperature Data Collector")

        # Temperature input
        input_frame = tk.Frame(self.root)
        input_frame.pack(pady=10)
        
        tk.Label(input_frame, text="Temperature (°F):").pack(side=tk.LEFT)
        temp_entry = tk.Entry(input_frame, width=10)
        temp_entry.pack(side=tk.LEFT, padx=5)
        tk.Button(input_frame, text="Record", 
                 command=lambda: self.add_external_temp(temp_entry)).pack(side=tk.LEFT)

        # Status display
        status_frame = tk.Frame(self.root)
        status_frame.pack(pady=10)
        self.status_label = tk.Label(status_frame, text="Waiting for sensor data...")
        self.status_label.pack()

        # Stop button
        tk.Button(self.root, text="Stop Program", 
                 command=self.stop_program, bg="red", fg="white").pack(pady=10)

        # Start serial thread
        Thread(target=self.read_serial, daemon=True).start()
        
        # Start UI updates
        self.update_ui()
        self.root.mainloop()

    def update_ui(self):
        if self.running:
            with self.lock:
                voltages = self.latest_voltages
                
            if None not in voltages:
                status_text = "\n".join([f"A{i}: {v:.2f}V" for i, v in enumerate(voltages)])
                self.status_label.config(text=status_text)
            
            self.root.after(500, self.update_ui)

# Configuration - Update these values
PORT = '/dev/cu.usbmodem143101'  # Replace with your actual port
BAUD_RATE = 9600
CSV_FILE = 'sensor_data.csv'

# Start application
app = SensorDataCollector(PORT, BAUD_RATE, CSV_FILE)
app.start()
