#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// WiFi Access Point credentials
const char* ssid = "ESP32_Irrigation";  // AP name - ESP32 creates its own WiFi network
const char* password = "12345678";      // AP password (min 8 characters)

// Pin definitions
#define LED_PIN 2        // Built-in LED for motor mixing indicator
#define PUMP_PIN 4       // Pin for pump control (if needed later)

// Motor driver pin definitions
#define ENA 23           // Enable A
#define ENB 22           // Enable B
#define IN1 35           // Input 1
#define IN2 32           // Input 2
#define IN3 33           // Input 3
#define IN4 25           // Input 4

// Global variables
WebServer server(80);
int motorMixTime = 180;  // Default 180 seconds (3 minutes)
bool systemRunning = false;
bool motorRunning = false;
bool pumpRunning = false;
unsigned long motorStartTime = 0;
unsigned long motorEndTime = 0;

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
                <span class="status-label">WiFi Signal</span>
                <span class="status-value" id="wifiSignal">-- dBm</span>
            </div>
        </div>
        
        <!-- Motor Time Configuration -->
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
                    
                    // WiFi signal
                    document.getElementById('wifiSignal').textContent = data.rssi + ' dBm';
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

// Handle root page
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

// Handle status request
void handleStatus() {
  String json = "{";
  json += "\"systemRunning\":" + String(systemRunning ? "true" : "false") + ",";
  json += "\"motorRunning\":" + String(motorRunning ? "true" : "false") + ",";
  json += "\"pumpRunning\":" + String(pumpRunning ? "true" : "false") + ",";
  json += "\"motorTime\":" + String(motorMixTime) + ",";
  json += "\"rssi\":" + String(WiFi.RSSI());
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
    motorRunning = true;
    motorStartTime = millis();
    motorEndTime = motorStartTime + (motorMixTime * 1000);
    digitalWrite(LED_PIN, HIGH);  // Turn on LED when motor starts
    
    // Start motor
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    digitalWrite(ENA, HIGH);
    digitalWrite(ENB, HIGH);
    
    String json = "{\"success\":true,\"motorTime\":" + String(motorMixTime) + "}";
    server.send(200, "application/json", json);
    Serial.println("System started! Motor mixing for " + String(motorMixTime) + " seconds");
  } else {
    server.send(200, "application/json", "{\"success\":false,\"error\":\"System already running\"}");
  }
}

// Handle stop system
void handleStop() {
  systemRunning = false;
  motorRunning = false;
  pumpRunning = false;
  digitalWrite(LED_PIN, LOW);  // Turn off LED
  
  // Stop motor
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  digitalWrite(ENA, LOW);
  digitalWrite(ENB, LOW);
  
  server.send(200, "application/json", "{\"success\":true}");
  Serial.println("System stopped");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Initialize LED pin
  pinMode(LED_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(PUMP_PIN, LOW);
  
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
  
  server.begin();
  Serial.println("Web server started!");
  Serial.println("=====================================\n");
}

void loop() {
  server.handleClient();
  
  // Handle motor timing (LED blinks for the set duration)
  if (systemRunning && motorRunning) {
    unsigned long currentTime = millis();
    
    // Check if motor mixing time is complete
    if (currentTime >= motorEndTime) {
      motorRunning = false;
      pumpRunning = true;  // Start pump after motor finishes
      digitalWrite(LED_PIN, LOW);  // Turn off LED
      
      // Stop motor
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, LOW);
      digitalWrite(ENA, LOW);
      digitalWrite(ENB, LOW);
      
      Serial.println("Motor mixing complete! Pump starting...");
    }
    // Motor continues running for the set duration (no blinking)
  }
}
