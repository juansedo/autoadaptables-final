import serial
import requests
import time
from datetime import datetime
from constants import (
    SERIAL_PORT,
    URL,
    TIME_URL,
)
from dateutil import parser
from datetime import datetime, timedelta
from sklearn.linear_model import LinearRegression
import numpy as np

def connect_to_serial():
    return serial.Serial(
        port=SERIAL_PORT,
        baudrate=9600,
        write_timeout=1,
        timeout=0
    )

ser = connect_to_serial()
url = 'https://isa.requestcatcher.com/post'

model = LinearRegression()

# Datos de entrenamiento iniciales (simulados)
X_train = np.array([[400], [300], [200], [100], [50]])  # Valores de CO2
y_train = np.array([1200, 900, 600, 300, 150]) # ms

model.fit(X_train, y_train)

def get_key(item):
    data = item.split(":")
    if len(data) < 1:
        return None
    return item.split(":")[0].strip()


def get_value(item):
    data = item.split(":")
    if len(data) < 2:
        return ""
    return item.split(":")[1].strip()

def send_to_serial(data):
    ser.write(f"{data}\n".encode("utf-8", errors="strict"))
    print(f"[serv] {data}")


def get_time_of_day():
    response = requests.get(TIME_URL)
    response = requests.get("http://worldtimeapi.org/api/timezone/Etc/UTC")
    data = response.json()
    current_time = datetime.fromisoformat(data["datetime"])
    hour = current_time.hour
 
    if 'datetime' in data:
        current_time = parser.isoparse(data["datetime"])
        current_time = current_time - timedelta(hours=5)
        hour = current_time.hour
 
        if 6 <= hour < 18:
            return "DAY"
        else:
            return "NIGHT"
    else:
        raise ValueError("La respuesta de la API no contiene la clave 'datetime'")

count = 0

while True:
    try:
        time_of_day = get_time_of_day()

        send_to_serial(f"CMD:{time_of_day}")
        
        line = ser.readline().decode("utf-8", errors="ignore").strip()
        print(f"[ardu-info] {line}")

        sensor_data = {
            get_key(item): get_value(item)
            for item in line.split(", ")
        }
        co2_value = int(sensor_data.get("CO2", 0)) + 400
        
        if (count % 15 == 0):
            count = 0
            predicted_wait_time = model.predict(np.array([[co2_value]]))[0]
            send_to_serial(f"WAIT:{int(predicted_wait_time)}")
        
        response = requests.post(url, data={'sensor_data': line, 'predicted_wait_time': predicted_wait_time})
        print("[pred-info]", response.status_code, response.text)
        
        count += 1
        time.sleep(1)

    except serial.SerialTimeoutException as exc:
        print("[serv] Write timeout")
        ser = connect_to_serial()
    except serial.SerialException as e:
        print("Error de conexión serial: ", e)
        break
    except requests.RequestException as e:
        print("Error al enviar datos: ", e)
        break
    except KeyboardInterrupt:
        print("Programa interrumpido manualmente.")
        break
