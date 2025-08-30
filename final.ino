  /*
  * ESP32 FreeRTOS Code
  * Chức năng:
  * 1. Điều khiển relay qua Blynk (V1).
  * 2. Đọc thẻ RFID (MFRC522) để mở khóa Servo.
  * 3. Điều khiển Servo làm khóa cửa.
  * 4. Đọc & gửi dữ liệu DHT11 lên Blynk (V8, V9) và MQTT.
  * 5. Đọc & gửi dữ liệu LDR lên Blynk (V10) và MQTT.
  * 6. Kết nối MQTT broker để gửi dữ liệu cảm biến.
  */

  // --- THÔNG TIN BLYNK ---
  #define BLYNK_TEMPLATE_ID   ""
  #define BLYNK_TEMPLATE_NAME ""
  #define BLYNK_AUTH_TOKEN    ""
  #define BLYNK_PRINT Serial

  // --- THƯ VIỆN ---
  #include <WiFi.h>
  #include <BlynkSimpleEsp32.h>
  #include <ESP32Servo.h>
  #include <SPI.h>
  #include <MFRC522.h>
  #include <DHT.h>
  #include <PubSubClient.h> // Thư viện MQTT

  // --- WIFI ---
  char ssid[] = "";
  char pass[] = "";

  // --- MQTT BROKER ---
  const char* mqtt_server = "";
  const int mqtt_port = 1883;
  #define MQTT_TOPIC_TEMP "esp32/dht11/temperature"
  #define MQTT_TOPIC_HUMID "esp32/dht11/humidity"
  #define MQTT_TOPIC_LIGHT "esp32/ldr/light"

  // --- KHỞI TẠO CLIENTS ---
  WiFiClient espClient;
  PubSubClient mqttClient(espClient);

  // --- PHẦN CỨNG ---
  // Relay
  const int RELAY_PIN = 14;
  // Servo
  #define SERVO_PIN 13
  Servo myServo;
  int servoOpenAngle = 90, servoCloseAngle = 0;
  // RFID
  #define RST_PIN 4
  #define SS_PIN 5
  MFRC522 rfid(SS_PIN, RST_PIN);
  String authorizedUID = "DD AF 13 05"; // UID thẻ
  // DHT11
  #define DHTPIN 15
  #define DHTTYPE DHT11
  DHT dht(DHTPIN, DHTTYPE);
  // LDR
  #define LDR_PIN 34

  // --- BIẾN TRẠNG THÁI ---
  bool relayState = false;
  bool lockState = false;

  // --- TIMER BLYNK ---
  BlynkTimer timer;

  // --- TASK HANDLE ---
  TaskHandle_t taskBlynkHandle;
  TaskHandle_t taskTimerHandle;
  TaskHandle_t taskRFIDHandle;
  TaskHandle_t taskSensorHandle;
  TaskHandle_t taskServoHandle;
  TaskHandle_t taskMQTTHandle; // Task handle cho MQTT

  // --- QUEUE / FLAG ---
  QueueHandle_t servoQueue;

  // --- HÀM ---
  void openLock() {
    Serial.println("Mo khoa...");
    myServo.write(servoOpenAngle);
    lockState = true;
    Blynk.virtualWrite(V6, 1);

    // Tự động đóng khóa sau 5 giây
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    Serial.println("Tu dong dong khoa.");
    myServo.write(servoCloseAngle);
    lockState = false;
    Blynk.virtualWrite(V6, 0);
  }

  // Hàm gửi dữ liệu cảm biến lên Blynk và MQTT
  void sendSensorData() {
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    int lightValue = analogRead(LDR_PIN);

    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Loi DHT11!");
      return;
    }

    Serial.printf("Nhiet do: %.1f°C, Do am: %.1f%%, Anh sang: %d\n",
                  temperature, humidity, lightValue);

    // Gửi lên Blynk
    Blynk.virtualWrite(V8, temperature);
    Blynk.virtualWrite(V9, humidity);
    Blynk.virtualWrite(V10, lightValue);

    // Gửi lên MQTT Broker
    if (mqttClient.connected()) {
      char msgBuffer[10];

      // Gửi nhiệt độ
      dtostrf(temperature, 4, 1, msgBuffer);
      mqttClient.publish(MQTT_TOPIC_TEMP, msgBuffer);

      // Gửi độ ẩm
      dtostrf(humidity, 4, 1, msgBuffer);
      mqttClient.publish(MQTT_TOPIC_HUMID, msgBuffer);

      // Gửi ánh sáng
      itoa(lightValue, msgBuffer, 10);
      mqttClient.publish(MQTT_TOPIC_LIGHT, msgBuffer);
    }
  }

  // Hàm kết nối lại MQTT (đã được cải thiện)
  void reconnectMQTT() {
    while (!mqttClient.connected()) {
      // Chỉ thử kết nối MQTT khi đã có WiFi
      if (WiFi.status() == WL_CONNECTED) {
        Serial.print("Dang ket noi MQTT Broker...");
        // Cố gắng kết nối
        if (mqttClient.connect("ESP32_Client")) { // "ESP32_Client" là Client ID
          Serial.println("da ket noi.");
        } else {
          Serial.print("that bai, rc=");
          Serial.print(mqttClient.state());
          Serial.println(" -> Thu lai sau 5 giay");
          // Đợi 5 giây trước khi thử lại
          vTaskDelay(5000 / portTICK_PERIOD_MS);
        }
      } else {
        Serial.println("Mat ket noi WiFi. Dang cho...");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
      }
    }
  }


  // --- TASKS ---
  // Task Blynk
  void taskBlynk(void *pvParameters) {
    while (1) {
      Blynk.run();
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
  }

  // Task Timer
  void taskTimer(void *pvParameters) {
    while (1) {
      timer.run();
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
  }

  // Task RFID
  void taskRFID(void *pvParameters) {
    while (1) {
      if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        String currentUID = "";
        for (byte i = 0; i < rfid.uid.size; i++) {
          currentUID.concat(String(rfid.uid.uidByte[i] < 0x10 ? " 0" : " "));
          currentUID.concat(String(rfid.uid.uidByte[i], HEX));
        }
        currentUID.toUpperCase();
        currentUID.trim();

        Serial.print("UID quet duoc: ");
        Serial.println(currentUID);
        Blynk.virtualWrite(V7, currentUID);

        if (currentUID == authorizedUID) {
          Serial.println("The hop le!");
          Blynk.logEvent("valid_card", "The hop le da duoc quet");
          int cmd = 1; // mở khóa
          xQueueSend(servoQueue, &cmd, portMAX_DELAY);
        } else {
          Serial.println("The khong hop le!");
          Blynk.logEvent("invalid_card", "The khong hop le: " + currentUID);
        }

        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
      }
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
  }

  // Task Sensor
  void taskSensor(void *pvParameters) {
    while (1) {
      sendSensorData();
      vTaskDelay(2000 / portTICK_PERIOD_MS); // 2s đọc 1 lần
    }
  }

  // Task Servo (nhận lệnh từ queue)
  void taskServo(void *pvParameters) {
    int cmd;
    while (1) {
      if (xQueueReceive(servoQueue, &cmd, portMAX_DELAY)) {
        if (cmd == 1) {
          openLock();
        }
      }
    }
  }

  // Task MQTT
  void taskMQTT(void *pvParameters) {
    reconnectMQTT();
    while (1) {
      if (!mqttClient.connected()) {
        reconnectMQTT();
      }
      mqttClient.loop(); // Duy trì kết nối và xử lý message
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
  }


  // --- BLYNK CALLBACKS ---
  BLYNK_CONNECTED() {
    Blynk.syncVirtual(V1, V6);
  }

  BLYNK_WRITE(V1) {
    relayState = param.asInt();
    digitalWrite(RELAY_PIN, relayState);
    Serial.printf("Relay da %s\n", relayState ? "BAT" : "TAT");
  }

  BLYNK_WRITE(V6) {
    if (param.asInt() == 1) {
      int cmd = 1;
      xQueueSend(servoQueue, &cmd, portMAX_DELAY);
    }
  }

  // --- SETUP ---
  void setup() {
    Serial.begin(115200);
    Serial.println("\nBat dau chuong trinh...");

    pinMode(RELAY_PIN, OUTPUT);

    myServo.attach(SERVO_PIN);
    myServo.write(servoCloseAngle);

    SPI.begin();
    rfid.PCD_Init();

    dht.begin();

    // Kết nối WiFi và Blynk
    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

    // Cấu hình MQTT
    mqttClient.setServer(mqtt_server, mqtt_port);

    // Cài timer cho Blynk nếu cần
    timer.setInterval(2000L, []() {
      // Có thể bỏ vì đã có taskSensor
    });

    // Tạo queue cho Servo
    servoQueue = xQueueCreate(5, sizeof(int));

    // Tạo task
    xTaskCreatePinnedToCore(taskBlynk, "BlynkTask", 4096, NULL, 1, &taskBlynkHandle, 0);
    xTaskCreatePinnedToCore(taskTimer, "TimerTask", 2048, NULL, 1, &taskTimerHandle, 0);
    xTaskCreatePinnedToCore(taskMQTT, "MQTTTask", 4096, NULL, 1, &taskMQTTHandle, 0); // Thêm task MQTT
    xTaskCreatePinnedToCore(taskRFID, "RFIDTask", 4096, NULL, 1, &taskRFIDHandle, 1);
    xTaskCreatePinnedToCore(taskSensor, "SensorTask", 4096, NULL, 1, &taskSensorHandle, 1);
    xTaskCreatePinnedToCore(taskServo, "ServoTask", 4096, NULL, 1, &taskServoHandle, 1);

    Serial.println("He thong da khoi tao xong.");
  }

  void loop() {
    // Loop bỏ trống vì dùng FreeRTOS task
  }
