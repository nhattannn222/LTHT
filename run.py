from flask import Flask
from flask_mqtt import Mqtt
from flask_sqlalchemy import SQLAlchemy
from datetime import datetime
import json

app = Flask(__name__)

# --- CẤU HÌNH ---
# Kết nối MySQL
app.config['SQLALCHEMY_DATABASE_URI'] = 'mysql+pymysql://root:nhattan123@localhost/ltht_finalterm'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
db = SQLAlchemy(app)

# Kết nối MQTT
app.config['MQTT_BROKER_URL'] = 'localhost' # Hoặc IP của broker nếu khác
app.config['MQTT_BROKER_PORT'] = 1883
app.config['MQTT_KEEPALIVE'] = 60
app.config['MQTT_CLIENT_ID'] = 'flask_mqtt_backend'
mqtt = Mqtt(app)

# Biến tạm để lưu trữ dữ liệu cảm biến trước khi ghi vào DB
latest_sensor_data = {}

# --- MODELS (Khớp với Database Schema) ---
class SensorData(db.Model):
    __tablename__ = 'sensor_data' # Chỉ định rõ tên bảng
    id = db.Column(db.Integer, primary_key=True)
    temperature = db.Column(db.Float, nullable=True)
    humidity = db.Column(db.Float, nullable=True)
    pir_status = db.Column(db.Boolean, nullable=True) # Đổi sang pir_status, kiểu Boolean
    created_at = db.Column(db.DateTime, server_default=db.func.now())

class RfidData(db.Model):
    __tablename__ = 'rfid_data' # Chỉ định rõ tên bảng
    id = db.Column(db.Integer, primary_key=True)
    card_uid = db.Column(db.String(50), nullable=False)
    status = db.Column(db.String(20))
    created_at = db.Column(db.DateTime, server_default=db.func.now())


# --- MQTT EVENT HANDLERS ---
@mqtt.on_connect()
def handle_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT Broker successfully!")
        # Lắng nghe các topic có trong database
        mqtt.subscribe("esp32/dht11/temperature")
        mqtt.subscribe("esp32/dht11/humidity")
        # Bạn có thể thêm topic cho PIR ở đây nếu ESP32 có gửi
        # mqtt.subscribe("esp32/pir/status")
    else:
        print(f"Failed to connect, return code {rc}\n")


@mqtt.on_message()
def handle_mqtt_message(client, userdata, message):
    global latest_sensor_data
    # Cần tạo application context để tương tác với database từ thread của MQTT
    with app.app_context():
        topic = message.topic
        payload = message.payload.decode()
        print(f"Received message '{payload}' from topic '{topic}'")

        try:
            # Lưu giá trị nhận được vào biến tạm
            if topic == "esp32/dht11/temperature":
                latest_sensor_data['temperature'] = float(payload)
            elif topic == "esp32/dht11/humidity":
                latest_sensor_data['humidity'] = float(payload)
            # ví dụ: elif topic == "esp32/pir/status":
            #    latest_sensor_data['pir_status'] = bool(int(payload))

            # Khi đã nhận đủ cả nhiệt độ và độ ẩm, ghi vào DB
            if 'temperature' in latest_sensor_data and 'humidity' in latest_sensor_data:
                new_entry = SensorData(
                    temperature=latest_sensor_data.get('temperature'),
                    humidity=latest_sensor_data.get('humidity'),
                    pir_status=latest_sensor_data.get('pir_status') # Lấy pir nếu có
                )
                db.session.add(new_entry)
                db.session.commit()
                print(f"New combined entry added to database: {latest_sensor_data}")
                
                # Xóa dữ liệu tạm để chờ lần gửi tiếp theo
                latest_sensor_data.clear()

        except ValueError:
            print(f"Could not convert payload '{payload}' to a number.")
        except Exception as e:
            print(f"An error occurred: {e}")
            db.session.rollback()


if __name__ == '__main__':
    with app.app_context():
        db.create_all() # Tạo bảng nếu chưa tồn tại
    app.run(host="0.0.0.0", port=5000)
