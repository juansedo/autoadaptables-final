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

ser = serial.Serial(SERIAL_PORT, 9600)
url = 'https://isa.requestcatcher.com/post'

model = LinearRegression()

# Datos de entrenamiento iniciales (simulados)
X_train = np.array([[400], [300], [200], [100], [50]])  # Valores de CO2
y_train = np.array([120, 90, 60, 30, 15])  # Tiempos de espera en segundos

model.fit(X_train, y_train)

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

while True:
    try:
        time_of_day = get_time_of_day()
        print("Time of day:", time_of_day)

        ser.write(f"CMD:{time_of_day}\n".encode('utf-8'))
        
        line = ser.readline().decode('utf-8').strip()
        print("Datos leídos del Arduino:", line)

        response = requests.post(URL, data={'sensor_data': line})
        
        # Extraer el valor del sensor de CO2 de los datos leídos del Arduino
        sensor_data = dict(item.split(": ") for item in line.split(", "))
        co2_value = int(sensor_data.get("CO2", 0))
        
        # Predecir el tiempo de espera del semáforo usando el modelo de regresión
        predicted_wait_time = model.predict(np.array([[co2_value]]))[0]
        
        # Enviar el tiempo de espera predicho al Arduino
        ser.write(f"WAIT:{predicted_wait_time:.2f}\n".encode('utf-8'))
        
        response = requests.post(url, data={'sensor_data': line, 'predicted_wait_time': predicted_wait_time})
        print("Respuesta del servidor:", response.status_code, response.text)
        
        time.sleep(60)
        
    except serial.SerialException as e:
        print("Error de conexión serial: ", e)
        break
    except requests.RequestException as e:
        print("Error al enviar datos: ", e)
        break
    except KeyboardInterrupt:
        print("Programa interrumpido manualmente.")
        break
