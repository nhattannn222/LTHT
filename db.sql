CREATE TABLE IF NOT EXISTS sensor_data (
    id INT AUTO_INCREMENT PRIMARY KEY,
    temperature FLOAT,
    humidity FLOAT,
    pir_status TINYINT(1),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Bảng lưu dữ liệu RFID
CREATE TABLE IF NOT EXISTS rfid_data (
    id INT AUTO_INCREMENT PRIMARY KEY,
    card_uid VARCHAR(50) NOT NULL,
    status VARCHAR(20),  -- VD: "granted", "denied"
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP

);