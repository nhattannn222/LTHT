# 🏠 Dự án Nhà Thông Minh IoT với ESP32, MQTT, Flask và Grafana

Một hệ thống **Smart Home hoàn chỉnh**, sử dụng **ESP32** để thu thập dữ liệu từ cảm biến (nhiệt độ, độ ẩm, PIR) và điều khiển thiết bị (khóa cửa servo, đèn).  
Dữ liệu được truyền qua **MQTT** đến **Flask Backend**, lưu trong **MySQL**, và trực quan hóa theo thời gian thực bằng **Grafana**.

---

## 🏗️ Kiến trúc hệ thống

ESP32 (Sensors/RFID) → MQTT Broker (Mosquitto) → Flask Backend (Python) → MySQL DB (Lưu trữ) → Grafana Dashboard (Trực quan hóa)


---

## ✨ Tính năng chính

- 📡 **Giám sát thời gian thực**: Nhiệt độ, độ ẩm, trạng thái chuyển động.  
- 🔑 **Điều khiển truy cập**: RFID để mở/khóa cửa servo.  
- 💡 **Tự động hóa**: Bật đèn khi phát hiện chuyển động hoặc khi cửa mở.  
- 🗃️ **Lưu trữ dữ liệu lịch sử**: Toàn bộ dữ liệu cảm biến trong MySQL.  
- 📈 **Trực quan hóa dữ liệu**: Dashboard Grafana hiển thị đẹp và dễ theo dõi.  
- 🐳 **Triển khai dễ dàng**: Đóng gói toàn bộ bằng Docker Compose.  

---

## 🛠 Công nghệ sử dụng

- **Phần cứng**: ESP32, DHT11, PIR, RFID MFRC522, Servo  
- **Firmware**: C++ trên Arduino (FreeRTOS)  
- **Giao thức**: MQTT  
- **Backend**: Flask (Python, SQLAlchemy)  
- **Database**: MySQL  
- **Dashboard**: Grafana  
- **Triển khai**: Docker & Docker Compose  

---

## 🚀 Hướng dẫn triển khai

### 📌 Yêu cầu
- Docker & Docker Compose cài đặt sẵn  
- ESP32 đã nạp firmware  
- ESP32 và máy chạy Docker cùng WiFi  

### 📂 Chuẩn bị
Clone repo hoặc đảm bảo có đủ 4 file trong thư mục dự án:  

docker-compose.yml
Dockerfile
requirements.txt
run.py


### ⚙️ Cấu hình ESP32
Trong file `.ino`, chỉnh thông tin WiFi và MQTT:
```cpp
const char* ssid = "Ten_WiFi";
const char* password = "Mat_khau_WiFi";
const char* mqtt_server = "IP_may_chay_Docker";
```

---

▶️ Khởi chạy Backend

Tại thư mục chứa docker-compose.yml, chạy:

docker-compose up -d


Hệ thống sẽ khởi chạy: Mosquitto, MySQL, Flask Backend, Grafana.

🔍 Kiểm tra hoạt động

Flask Backend → http://localhost:5000

(log MQTT: docker-compose logs -f flask-backend)

Grafana → http://localhost:3000

(đăng nhập mặc định: admin/admin)

MQTT Broker (Mosquitto) → localhost:1883

📊 Cấu hình Grafana

Add Data Source → MySQL

Host: mysql-db:3306

Database: ltht_finalterm

User: root

Password: your_password

Tạo Dashboard với query ví dụ:

SELECT
  CONVERT_TZ(created_at, '+00:00', '+07:00') AS "time",
  temperature,
  humidity
FROM sensor_data
ORDER BY time;

---

⏹️ Dừng hệ thống
docker-compose down


👉 Dữ liệu vẫn được giữ lại nhờ Docker volumes.

---

Giảng viên hướng dẫn: ThS. Lê Duy Hùng
Nhóm sinh viên thực hiện: Nguyễn Hoàng Nhật Tân - Phạm Quốc Việt