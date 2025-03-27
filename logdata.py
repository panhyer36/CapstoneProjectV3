import serial
import threading
import time
import json
import requests
from datetime import datetime

# Global variable to store the latest data (if needed)
latest_data = {}
data_lock = threading.Lock()

# Serial port settings (adjust SERIAL_PORT according to your Arduino connection)
SERIAL_PORT = "/dev/ttyACM0"
BAUD_RATE = 9600

LOG_FILENAME = "airdata.log"

# API endpoint to send sensor data
API_URL = "http://23.94.61.249:8000/api/sensor-data/"

def send_sensor_data(payload):
    """
    Sends the sensor data to the server's API via a POST request.
    """
    try:
        response = requests.post(API_URL, json=payload)
        if response.status_code in (200, 201):
            print("Data successfully sent to the server.")
        else:
            print("Failed to send data. Status code:", response.status_code, "Response:", response.text)
    except Exception as e:
        print("Error sending data:", e)

def serial_listener():
    """
    Listens to the serial port, reads incoming JSON data,
    logs it to a file, and sends the data to the server API.
    """
    global latest_data
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    except Exception as e:
        print("Unable to open serial port:", e)
        return

    with open(LOG_FILENAME, "a", buffering=1) as logfile:
        buffer = ""
        while True:
            try:
                # Read a line from the serial port
                line = ser.readline().decode('utf-8', errors='replace')
                if not line:
                    continue

                buffer += line
                # Attempt to parse JSON when a closing bracket is found
                if "}" in buffer:
                    start = buffer.find("{")
                    end = buffer.find("}", start)
                    if start != -1 and end != -1:
                        json_str = buffer[start:end+1]
                        buffer = buffer[end+1:]  # Remove the processed part
                        try:
                            data = json.loads(json_str)
                            # Attach a timestamp
                            current_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                            data["Time"] = current_time
                            with data_lock:
                                latest_data = data
                            # Log the data (one JSON object per line)
                            logfile.write(json.dumps(data) + "\n")
                            print("Received data:", data)
                            
                            # Prepare the payload for the API (removing the "Time" field if not needed)
                            payload = {
                                "co2": data.get("co2"),
                                "humidity": data.get("humidity"),
                                "pm1_0": data.get("pm1_0"),
                                "pm2_5": data.get("pm2_5"),
                                "pm10_0": data.get("pm10_0"),
                                "temperature": data.get("temperature")
                            }
                            send_sensor_data(payload)
                            
                        except Exception as json_err:
                            print("Failed to parse JSON:", json_err, "Raw data:", json_str)
            except Exception as err:
                print("Error reading from serial port:", err)
            time.sleep(0.1)

if __name__ == '__main__':
    # Start the serial listener thread
    thread = threading.Thread(target=serial_listener, daemon=True)
    thread.start()
    
    # Keep the main thread alive
    while True:
        time.sleep(1)