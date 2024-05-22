import serial
import requests
import datetime

ser = serial.Serial('/dev/ttyS0', 9600)  

url = 'https://isa.requestcatcher.com/post'

def get_time_of_day():
    response = requests.get("http://worldtimeapi.org/api/timezone/Etc/UTC")
    data = response.json()
    current_time = datetime.datetime.fromisoformat(data['datetime'][:-1])
    hour = current_time.hour

    # Determinar si es de día o de noche basado en la hora UTC
    if 6 <= hour < 18:
        return "DAY"
    else:
        return "NIGHT"

while True:
    try:
        # Obtén la hora actual y determina si es de día o de noche
        time_of_day = get_time_of_day()
        print("Time of day:", time_of_day)

        # Envía esta información al Arduino cada minuto
        ser.write(f"CMD:{time_of_day}\n".encode('utf-8'))
        
        # Lee una línea del puerto serie
        line = ser.readline().decode('utf-8').strip()
        print("Datos leídos del Arduino:", line)
        
        # Envía los datos a la URL especificada
        response = requests.post(url, data={'sensor_data': line})
        print("Respuesta del servidor:", response.status_code, response.text)
        
        time.sleep(60)  # Espera 60 segundos antes de enviar la siguiente actualización
        
    except serial.SerialException as e:
        print("Error de conexión serial: ", e)
        break
    except requests.RequestException as e:
        print("Error al enviar datos: ", e)
        break
    except KeyboardInterrupt:
        print("Programa interrumpido manualmente.")
        break