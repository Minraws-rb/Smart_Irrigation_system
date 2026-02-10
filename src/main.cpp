 #include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// WiFi Access Point credentials
const char* ssid = "ESP32_Irrigation";  // AP name - ESP32 creates its own WiFi network
const char* password = "12345678";      // AP password (min 8 characters)

// Pin definitions
#define LED_PIN 2        // Built-in LED for motor mixing indicator
#define PUMP_PIN 4       // Pin for pump control (if needed later)
#define SOIL_MOISTURE_PIN 4  // Soil moisture sensor pin (D4)

// Ultrasonic sensor pin definitions
#define TRIG_PIN 19      // Ultrasonic sensor trigger pin
#define ECHO_PIN 21      // Ultrasonic sensor echo pin

// Motor driver pin definitions
#define ENA 23           // Enable A
#define ENB 22           // Enable B
#define IN1 26           // Input 1
#define IN2 32           // Input 2
#define IN3 33           // Input 3
#define IN4 25           // Input 4

// Vessel parameters
const float VESSEL_HEIGHT = 16.0;    // Height of vessel in cm
const float VESSEL_AREA = 153.0;     // Base area in cm² (circle)
const float EMPTY_THRESHOLD = 0.2;   // Consider empty if water height < 1cm

// Global variables
WebServer server(80);
int motorMixTime = 180;  // Default 180 seconds (3 minutes)
bool systemRunning = false;
bool motor1Running = false;  // ENB motor (mixing motor)
bool motor2Running = false;  // ENA motor (pump motor)
unsigned long motor1StartTime = 0;
unsigned long motor1EndTime = 0;
// Motor speed variables (0-255)
int motor1Speed = 255;  // Mixing motor speed (default max)
int motor2Speed = 180;  // Pump motor speed (default max)
// Water volume variables
float waterDistance = 0;    // Distance from sensor to water surface in cm
float waterHeight = 0;      // Height of water in vessel in cm
float waterVolume = 0;      // Volume of water in cm³

// Soil moisture sensor
int soilMoistureValue = 0;  // Analog reading from moisture sensor (0-4095)
const int DRY_THRESHOLD = 2500;  // Values above this = dry soil
bool isSoilDry = false;     // true = soil is dry, false = soil is wet

// HTML page (your irrigation control interface)
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Irrigation Control System</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
        }
        
        .container {
            background: white;
            border-radius: 15px;
            box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
            padding: 40px;
            max-width: 550px;
            width: 100%;
        }
        
        h1 {
            color: #333;
            margin-bottom: 8px;
            text-align: center;
            font-size: 32px;
        }
        
        .subtitle {
            text-align: center;
            color: #666;
            margin-bottom: 30px;
            font-size: 14px;
        }
        
        .status-box {
            background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);
            border-radius: 12px;
            padding: 20px;
            margin-bottom: 25px;
            border-left: 5px solid #667eea;
        }
        
        .status-item {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 12px;
            font-size: 14px;
        }
        
        .status-item:last-child {
            margin-bottom: 0;
        }
        
        .status-label {
            color: #333;
            font-weight: 600;
        }
        
        .status-value {
            display: flex;
            align-items: center;
            color: #555;
        }
        
        .status-indicator {
            display: inline-block;
            width: 12px;
            height: 12px;
            border-radius: 50%;
            margin-right: 8px;
        }
        
        .status-on {
            background-color: #4CAF50;
            box-shadow: 0 0 10px rgba(76, 175, 80, 0.6);
            animation: pulse-green 1.5s infinite;
        }
        
        .status-off {
            background-color: #f44336;
        }
        
        @keyframes pulse-green {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }
        
        .control-section {
            margin-bottom: 25px;
        }
        
        .section-title {
            font-size: 16px;
            font-weight: 700;
            color: #333;
            margin-bottom: 15px;
            text-transform: uppercase;
            letter-spacing: 1px;
        }
        
        .input-group {
            display: flex;
            gap: 10px;
            margin-bottom: 15px;
        }
        
        input[type="number"] {
            flex: 1;
            padding: 12px;
            border: 2px solid #ddd;
            border-radius: 8px;
            font-size: 14px;
            transition: border-color 0.3s;
        }
        
        input[type="number"]:focus {
            outline: none;
            border-color: #667eea;
        }
        
        .btn {
            padding: 12px 24px;
            border: none;
            border-radius: 8px;
            font-size: 14px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        
        .btn-primary {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            flex: 1;
        }
        
        .btn-primary:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 20px rgba(102, 126, 234, 0.4);
        }
        
        .btn-primary:active {
            transform: translateY(0);
        }
        
        .btn-secondary {
            background: #f44336;
            color: white;
            flex: 1;
        }
        
        .btn-secondary:hover {
            background: #da190b;
            transform: translateY(-2px);
            box-shadow: 0 5px 20px rgba(244, 67, 54, 0.4);
        }
        
        .btn-secondary:active {
            transform: translateY(0);
        }
        
        .btn-set {
            background: #2196F3;
            color: white;
            padding: 12px 20px;
        }
        
        .btn-set:hover {
            background: #0b7dda;
            transform: translateY(-2px);
            box-shadow: 0 5px 20px rgba(33, 150, 243, 0.4);
        }
        
        .message {
            padding: 14px;
            border-radius: 8px;
            margin-bottom: 20px;
            font-size: 13px;
            text-align: center;
            display: none;
            animation: slideIn 0.3s ease;
        }
        
        .message.success {
            background: #d4edda;
            color: #155724;
            border: 1px solid #c3e6cb;
        }
        
        .message.error {
            background: #f8d7da;
            color: #721c24;
            border: 1px solid #f5c6cb;
        }
        
        @keyframes slideIn {
            from {
                transform: translateY(-20px);
                opacity: 0;
            }
            to {
                transform: translateY(0);
                opacity: 1;
            }
        }
        
        .info-box {
            background: #e3f2fd;
            color: #1565c0;
            padding: 12px;
            border-radius: 8px;
            font-size: 12px;
            margin-top: 10px;
            border-left: 4px solid #1565c0;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🌾 Irrigation Control</h1>
        <p class="subtitle">ESP32 Pesticide Mixing & Spraying System</p>
        
        <!-- Messages -->
        <div id="message" class="message"></div>
        
        <!-- Status Display -->
        <div class="status-box">
            <div class="status-item">
                <span class="status-label">System Status</span>
                <span class="status-value">
                    <span class="status-indicator status-off" id="sysIndicator"></span>
                    <span id="sysStatus">Inactive</span>
                </span>
            </div>
            <div class="status-item">
                <span class="status-label">Motor (Mixing)</span>
                <span class="status-value">
                    <span class="status-indicator status-off" id="motorIndicator"></span>
                    <span id="motorStatus">Off</span>
                </span>
            </div>
            <div class="status-item">
                <span class="status-label">Pump (Spraying)</span>
                <span class="status-value">
                    <span class="status-indicator status-off" id="pumpIndicator"></span>
                    <span id="pumpStatus">Off</span>
                </span>
            </div>
            <div class="status-item">
                <span class="status-label">Motor Mix Time</span>
                <span class="status-value" id="motorTime">180 seconds</span>
            </div>
            <div class="status-item">
                <span class="status-label">⏱️ Time Remaining</span>
                <span class="status-value" id="timeRemaining" style="font-weight: bold; color: #667eea;">--</span>
            </div>
            <div class="status-item">
                <span class="status-label">💧 Water Height</span>
                <span class="status-value" id="waterHeight">-- cm</span>
            </div>
            <div class="status-item">
                <span class="status-label">🚰 Water Volume</span>
                <span class="status-value" id="waterVolume" style="font-weight: bold; color: #2196F3;">-- L</span>
            </div>
            <div class="status-item">
                <span class="status-label">🌱 Soil Status</span>
                <span class="status-value" id="soilStatus" style="font-weight: bold;">--</span>
            </div>
        </div>
        
        <!-- Motor Speed Control -->
        <div class="control-section">
            <div class="section-title">⚡ Motor Speed Control</div>
            
            <div style="margin-bottom: 15px;">
                <label style="color: #333; font-weight: 600; display: block; margin-bottom: 8px;">Mixing Motor: <span id="motor1SpeedValue">255</span>/255</label>
                <input type="range" id="motor1SpeedSlider" min="0" max="255" value="255" style="width: 100%;" onchange="setMotor1Speed()">
            </div>
            
            <div style="margin-bottom: 15px;">
                <label style="color: #333; font-weight: 600; display: block; margin-bottom: 8px;">Pump Motor: <span id="motor2SpeedValue">255</span>/255</label>
                <input type="range" id="motor2SpeedSlider" min="0" max="255" value="255" style="width: 100%;" onchange="setMotor2Speed()">
            </div>
            
            <div class="info-box">
                ⚙️ 0 = Off | 127 = 50% Speed | 255 = Full Speed
            </div>
        </div>
        
        <!-- Motor Mixing Time -->
        <div class="control-section">
            <div class="section-title">⏱️ Set Motor Mixing Time</div>
            <div class="input-group">
                <input type="number" id="motorTimeInput" placeholder="Enter seconds" min="1" max="600" value="180">
                <button class="btn btn-set" onclick="setMotorTime()">Set</button>
            </div>
            <div class="info-box">
                💡 Default: 180 seconds (3 minutes) | Maximum: 600 seconds (10 minutes)
            </div>
        </div>
        
        <!-- System Controls -->
        <div class="control-section">
            <div class="section-title">🎮 System Controls</div>
            <div class="input-group">
                <button class="btn btn-primary" onclick="startSystem()">▶ START</button>
                <button class="btn btn-secondary" onclick="stopSystem()">⏹ STOP</button>
            </div>
            <div class="info-box">
                📋 Process: Motor mixes pesticides → Pump sprays mixture through nozzles
            </div>
        </div>
    </div>

    <script>
        function showMessage(text, type) {
            const msg = document.getElementById('message');
            msg.textContent = text;
            msg.className = 'message ' + type;
            msg.style.display = 'block';
            setTimeout(() => { msg.style.display = 'none'; }, 3000);
        }

        function updateStatus() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    // System status
                    document.getElementById('sysStatus').textContent = data.systemRunning ? 'Active' : 'Inactive';
                    document.getElementById('sysIndicator').className = 'status-indicator ' + (data.systemRunning ? 'status-on' : 'status-off');
                    
                    // Motor status
                    document.getElementById('motorStatus').textContent = data.motorRunning ? 'Running' : 'Off';
                    document.getElementById('motorIndicator').className = 'status-indicator ' + (data.motorRunning ? 'status-on' : 'status-off');
                    
                    // Pump status
                    document.getElementById('pumpStatus').textContent = data.pumpRunning ? 'Running' : 'Off';
                    document.getElementById('pumpIndicator').className = 'status-indicator ' + (data.pumpRunning ? 'status-on' : 'status-off');
                    
                    // Motor time
                    document.getElementById('motorTime').textContent = data.motorTime + ' seconds';
                    
                    // Time remaining countdown
                    if (data.timeRemaining !== undefined && data.timeRemaining >= 0) {
                        const minutes = Math.floor(data.timeRemaining / 60);
                        const seconds = data.timeRemaining % 60;
                        document.getElementById('timeRemaining').textContent = 
                            minutes + ':' + (seconds < 10 ? '0' : '') + seconds;
                    } else {
                        document.getElementById('timeRemaining').textContent = '--';
                    }
                    
                    // Water measurements
                    if (data.waterHeight !== undefined) {
                        document.getElementById('waterHeight').textContent = data.waterHeight.toFixed(1) + ' cm';
                    } else {
                        document.getElementById('waterHeight').textContent = '-- cm';
                    }
                    
                    if (data.waterVolume !== undefined) {
                        const liters = (data.waterVolume / 1000.0).toFixed(2);
                        document.getElementById('waterVolume').textContent = liters + ' L (' + data.waterVolume.toFixed(0) + ' cm³)';
                    } else {
                        document.getElementById('waterVolume').textContent = '-- L';
                    }
                    
                    // Update motor speeds
                    document.getElementById('motor1SpeedValue').textContent = data.motor1Speed;
                    document.getElementById('motor2SpeedValue').textContent = data.motor2Speed;
                    
                    // Update soil status
                    const soilStatusEl = document.getElementById('soilStatus');
                    if (data.soilDry) {
                        soilStatusEl.textContent = 'DRY';
                        soilStatusEl.style.color = '#f44336';
                    } else {
                        soilStatusEl.textContent = 'WET';
                        soilStatusEl.style.color = '#4CAF50';
                    }
                })
                .catch(err => console.error('Status update failed:', err));
        }

        function setMotorTime() {
            const time = document.getElementById('motorTimeInput').value;
            if (time < 1 || time > 600) {
                showMessage('⚠️ Please enter a value between 1 and 600 seconds', 'error');
                return;
            }
            
            fetch('/setMotorTime?time=' + time)
                .then(response => response.json())
                .then(data => {
                    if (data.success) {
                        showMessage('✅ Motor mixing time set to ' + time + ' seconds', 'success');
                        updateStatus();
                    } else {
                        showMessage('❌ Failed to set motor time', 'error');
                    }
                })
                .catch(err => {
                    showMessage('❌ Connection error', 'error');
                });
        }

        function setMotor1Speed() {
            const speed = document.getElementById('motor1SpeedSlider').value;
            fetch('/setMotor1Speed?speed=' + speed)
                .then(response => response.json())
                .then(data => {
                    if (data.success) {
                        document.getElementById('motor1SpeedValue').textContent = speed;
                    }
                })
                .catch(err => console.error('Failed to set motor 1 speed'));
        }

        function setMotor2Speed() {
            const speed = document.getElementById('motor2SpeedSlider').value;
            fetch('/setMotor2Speed?speed=' + speed)
                .then(response => response.json())
                .then(data => {
                    if (data.success) {
                        document.getElementById('motor2SpeedValue').textContent = speed;
                    }
                })
                .catch(err => console.error('Failed to set motor 2 speed'));
        }

        function startSystem() {
            fetch('/start')
                .then(response => response.json())
                .then(data => {
                    if (data.success) {
                        showMessage('✅ System started! Motor will mix for ' + data.motorTime + ' seconds', 'success');
                        updateStatus();
                    } else {
                        showMessage('❌ Failed to start system', 'error');
                    }
                })
                .catch(err => {
                    showMessage('❌ Connection error', 'error');
                });
        }

        function stopSystem() {
            fetch('/stop')
                .then(response => response.json())
                .then(data => {
                    if (data.success) {
                        showMessage('⏹ System stopped', 'success');
                        updateStatus();
                    } else {
                        showMessage('❌ Failed to stop system', 'error');
                    }
                })
                .catch(err => {
                    showMessage('❌ Connection error', 'error');
                });
        }

        // Update status every 1 second
        setInterval(updateStatus, 1000);
        updateStatus();
    </script>
</body>
</html>
)rawliteral";

// Function to measure distance using ultrasonic sensor
float measureDistance() {
  long duration;
  float distanceCm;
  
  // Clear the trigger pin
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  
  // Send 10us pulse to trigger
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Read the echo pin
  duration = pulseIn(ECHO_PIN, HIGH, 30000);  // 30ms timeout
  
  // Calculate distance in cm
  // Speed of sound = 343 m/s = 0.0343 cm/µs
  // Distance = (duration / 2) * speed of sound
  if (duration == 0) {
    return -1;  // Measurement failed
  }
  
  distanceCm = (duration * 0.0343) / 2.0;
  return distanceCm;
}

// Function to update water measurements
void updateWaterMeasurements() {
  waterDistance = measureDistance();
  
  Serial.println("========== Water Measurement ==========");
  
  if (waterDistance > 0 && waterDistance < 400) {  // Valid range
    // Calculate water height
    waterHeight = VESSEL_HEIGHT - waterDistance;
    
    // Ensure water height is not negative
    if (waterHeight < 0) {
      waterHeight = 0;
    }
    
    // Calculate water volume (Area × Height)
    waterVolume = VESSEL_AREA * waterHeight;
    
    // Print measurements to Serial Monitor
    Serial.print("Distance from sensor: ");
    Serial.print(waterDistance);
    Serial.println(" cm");
    
    Serial.print("Water height: ");
    Serial.print(waterHeight);
    Serial.println(" cm");
    
    Serial.print("Water volume: ");
    Serial.print(waterVolume);
    Serial.print(" cm³ (");
    Serial.print(waterVolume / 1000.0);
    Serial.println(" Liters)");
    
    Serial.print("Fill percentage: ");
    Serial.print((waterHeight / VESSEL_HEIGHT) * 100.0);
    Serial.println(" %");
  } else {
    // Invalid reading
    waterHeight = 0;
    waterVolume = 0;
    
    Serial.println("⚠️ Invalid ultrasonic reading!");
    Serial.print("Distance: ");
    Serial.println(waterDistance);
  }
  
  Serial.println("=======================================\n");
}

// Handle root page
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

// Handle status request
void handleStatus() {
  int timeRemaining = -1;
  
  // Calculate remaining time if motor is running
  if (motor1Running && systemRunning) {
    unsigned long currentTime = millis();
    if (currentTime < motor1EndTime) {
      timeRemaining = (motor1EndTime - currentTime) / 1000;
    } else {
      timeRemaining = 0;
    }
  }
  
  String json = "{";
  json += "\"systemRunning\":" + String(systemRunning ? "true" : "false") + ",";
  json += "\"motorRunning\":" + String(motor1Running ? "true" : "false") + ",";
  json += "\"pumpRunning\":" + String(motor2Running ? "true" : "false") + ",";
  json += "\"motorTime\":" + String(motorMixTime) + ",";
  json += "\"timeRemaining\":" + String(timeRemaining) + ",";
  json += "\"waterHeight\":" + String(waterHeight, 1) + ",";
  json += "\"waterVolume\":" + String(waterVolume, 1) + ",";
  json += "\"motor1Speed\":" + String(motor1Speed) + ",";
  json += "\"motor2Speed\":" + String(motor2Speed) + ",";
  json += "\"soilDry\":" + String(isSoilDry ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

// Handle set motor time
void handleSetMotorTime() {
  if (server.hasArg("time")) {
    int newTime = server.arg("time").toInt();
    if (newTime >= 1 && newTime <= 600) {
      motorMixTime = newTime;
      server.send(200, "application/json", "{\"success\":true}");
      Serial.println("Motor time set to: " + String(motorMixTime) + " seconds");
    } else {
      server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid time range\"}");
    }
  } else {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Missing time parameter\"}");
  }
}

// Handle start system
void handleStart() {
  if (!systemRunning) {
    systemRunning = true;
    motor1Running = true;  // Start ENB motor (mixing motor)
    motor2Running = false;
    motor1StartTime = millis();
    motor1EndTime = motor1StartTime + (motorMixTime * 1000);
    digitalWrite(LED_PIN, HIGH);  // Turn on LED when motor starts
    
    // Start Motor 1 (ENB side - mixing motor) with PWM speed control
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    analogWrite(ENB, motor1Speed);
    
    // Ensure Motor 2 is off
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, 0);
    
    String json = "{\"success\":true,\"motorTime\":" + String(motorMixTime) + ",\"motor1Speed\":" + String(motor1Speed) + "}";
    server.send(200, "application/json", json);
    Serial.println("System started! Motor 1 (ENB) mixing at speed " + String(motor1Speed) + " for " + String(motorMixTime) + " seconds");
  } else {
    server.send(200, "application/json", "{\"success\":false,\"error\":\"System already running\"}");
  }
}

// Handle stop system
void handleStop() {
  systemRunning = false;
  motor1Running = false;
  motor2Running = false;
  digitalWrite(LED_PIN, LOW);  // Turn off LED
  
  // Stop both motors
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  digitalWrite(ENA, LOW);
  digitalWrite(ENB, LOW);
  
  server.send(200, "application/json", "{\"success\":true}");
  Serial.println("System stopped - Both motors OFF");
}

// Handle set Motor 1 speed (Pump motor)
void handleSetMotor1Speed() {
  if (server.hasArg("speed")) {
    int speed = server.arg("speed").toInt();
    if (speed >= 0 && speed <= 255) {
      motor1Speed = speed;
      server.send(200, "application/json", "{\"success\":true,\"motor1Speed\":" + String(motor1Speed) + "}");
      Serial.println("Motor 1 speed set to: " + String(motor1Speed));
    } else {
      server.send(400, "application/json", "{\"success\":false,\"error\":\"Speed must be 0-255\"}");
    }
  } else {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Missing speed parameter\"}");
  }
}

// Handle set Motor 2 speed (Mixing motor)
void handleSetMotor2Speed() {
  if (server.hasArg("speed")) {
    int speed = server.arg("speed").toInt();
    if (speed >= 0 && speed <= 255) {
      motor2Speed = speed;
      server.send(200, "application/json", "{\"success\":true,\"motor2Speed\":" + String(motor2Speed) + "}");
      Serial.println("Motor 2 speed set to: " + String(motor2Speed));
    } else {
      server.send(400, "application/json", "{\"success\":false,\"error\":\"Speed must be 0-255\"}");
    }
  } else {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Missing speed parameter\"}");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Initialize LED pin
  pinMode(LED_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(PUMP_PIN, LOW);
  
  // Initialize ultrasonic sensor pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Soil moisture sensor uses analog input (no pinMode needed for analog)
  
  // Initialize motor driver pins
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  
  // Ensure motor is stopped initially
  digitalWrite(ENA, LOW);
  digitalWrite(ENB, LOW);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  
  Serial.println("\n\n=== ESP32 Irrigation Control System ===");
  
  // Set up WiFi Access Point (ESP32 creates its own WiFi network)
  Serial.println("Setting up WiFi Access Point...");
  WiFi.softAP(ssid, password);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.println("\n✅ AP Created Successfully!");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.print("📶 WiFi Name (SSID): ");
  Serial.println(ssid);
  Serial.print("🔐 Password: ");
  Serial.println(password);
  Serial.print("🌐 IP Address: ");
  Serial.println(IP);
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.println("\n📱 INSTRUCTIONS:");
  Serial.println("1. Connect your phone/laptop to WiFi: " + String(ssid));
  Serial.println("2. Open browser and go to: http://" + IP.toString());
  Serial.println();
  
  // Set up web server routes
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/setMotorTime", handleSetMotorTime);
  server.on("/start", handleStart);
  server.on("/stop", handleStop);
  server.on("/setMotor1Speed", handleSetMotor1Speed);
  server.on("/setMotor2Speed", handleSetMotor2Speed);
  
  server.begin();
  Serial.println("Web server started!");
  Serial.println("=====================================\n");
}

void loop() {
  server.handleClient();
  
  // Update water measurements periodically
  static unsigned long lastMeasurement = 0;
  if (millis() - lastMeasurement >= 500) {  // Measure every 500ms
    updateWaterMeasurements();
    lastMeasurement = millis();
  }
  
  // Read soil moisture sensor (analog 0-4095, higher = drier)
  soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);
  isSoilDry = soilMoistureValue > DRY_THRESHOLD;
  
  // Handle two-stage motor operation
  if (systemRunning) {
    unsigned long currentTime = millis();
    
    // Stage 1: Motor 1 (ENB - mixing motor) runs for specified time
    if (motor1Running) {
      if (currentTime >= motor1EndTime) {
        // Stop Motor 1 (ENB - mixing motor)
        motor1Running = false;
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, LOW);
        analogWrite(ENB, 0);
        
        // Start Motor 2 (ENA - pump motor) - runs until STOP is pressed or vessel is empty
        motor2Running = true;
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
        analogWrite(ENA, motor2Speed);
        
        Serial.println("Motor 1 (ENB - mixing) complete! Motor 2 (ENA - pump) started at speed " + String(motor2Speed) + " - will run until STOP or vessel empty");
      }
    }
    
    // Stage 2: Motor 2 (ENA - pump motor) - auto-stop if vessel is empty
    if (motor2Running) {
      if (waterHeight < EMPTY_THRESHOLD) {
        // Vessel is empty, stop the pump
        motor2Running = false;
        systemRunning = false;
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, LOW);
        analogWrite(ENA, 0);
        digitalWrite(LED_PIN, LOW);
        
        Serial.println("Vessel empty! Motor 2 (ENA - pump) stopped automatically");
        Serial.println("Water height: " + String(waterHeight) + " cm");
      }
    }
  }
}