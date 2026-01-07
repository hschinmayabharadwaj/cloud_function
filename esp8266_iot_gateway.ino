/*
 * ESP8266 IoT Device - Cloud Function Command Receiver
 *
 * IMPORTANT: This ESP8266 code receives commands from Firebase Cloud Function
 * - Cloud Function detects new commands in Firestore
 * - Cloud Function sends HTTP GET request to ESP8266
 * - ESP8266 HTTP server receives and executes commands
 * - No status update needed (Cloud Function handles it)
 *
 * Data Flow:
 * Mobile App → Firestore → Cloud Function → HTTP GET → ESP8266
 *
 * Hardware Setup:
 * - ESP8266 board (NodeMCU, Wemos D1 Mini, etc.)
 * - LED connected to D4 (GPIO2) or use built-in LED
 * - WiFi connection
 *
 * Required Libraries:
 * - ESP8266WiFi
 * - ESP8266WebServer (built-in)
 *
 * HTTP Endpoint:
 * GET http://ESP8266_IP/command?cmd=TURN_ON
 * GET http://ESP8266_IP/command?cmd=TURN_OFF
 * GET http://ESP8266_IP/command?cmd=BLINK&duration=2000
 * GET http://ESP8266_IP/command?cmd=PULSE&duration=3000
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

// ============================================
// CONFIGURATION - UPDATE THESE VALUES
// ============================================

// WiFi Credentials
#define WIFI_SSID "Galaxy S24 FE"
#define WIFI_PASSWORD "Chiranthu23"

// Firebase Firestore Configuration
#define FIREBASE_PROJECT_ID "ledone-84d2d"
#define FIREBASE_API_KEY "AIzaSyCf5Bd1Kz-oDv3DXT5_uIrT25SSe6ic6j8"

// Device Configuration
#define DEVICE_ID "esp8266_01"

// Hardware Configuration
#define LED_PIN D4  // GPIO2 on most ESP8266 boards (built-in LED)
#define LED_ACTIVE_LOW true  // Built-in LED is active LOW on most boards

// Polling Configuration
#define POLL_INTERVAL 5000  // Poll every 5 seconds

// ============================================
// GLOBAL VARIABLES
// ============================================

ESP8266WebServer server(80);  // HTTP server on port 80
unsigned long lastHeartbeat = 0;
const unsigned long HEARTBEAT_INTERVAL = 30000;  // 30 seconds


// ============================================
// SETUP
// ============================================

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("\n\n");
  Serial.println("====================================");
  Serial.println("ESP8266 IoT Device Starting...");
  Serial.println("Firestore Commands Listener");
  Serial.println("====================================");

  // Initialize LED to OFF (default state)
  pinMode(LED_PIN, OUTPUT);
  setLED(false);  // Ensure LED starts OFF
  Serial.println("📍 LED initialized to OFF (default state)");

  // Connect to WiFi
  connectWiFi();

  // Start HTTP server to receive commands from Cloud Function
  startHTTPServer();

  Serial.println("Setup complete! Waiting for commands from Cloud Function...");
  Serial.println("====================================\n");
}

// ============================================
// MAIN LOOP
// ============================================

void loop() {
  // Handle HTTP requests from Cloud Function
  server.handleClient();

  // Send periodic heartbeat
  if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL) {
    Serial.println("💓 Heartbeat - Device online");
    lastHeartbeat = millis();
  }

  // Small delay
  delay(100);
}

// ============================================
// WIFI CONNECTION
// ============================================

void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed!");
    Serial.println("Restarting...");
    ESP.restart();
  }
}

// ============================================
// HTTP SERVER - RECEIVE COMMANDS FROM CLOUD FUNCTION
// ============================================

void startHTTPServer() {
  Serial.println("\n� Starting HTTP server...");

  // Route for receiving commands from Cloud Function
  server.on("/command", handleCommand);

  // Route for health check
  server.on("/health", []() {
    server.send(200, "application/json", "{\"status\": \"ok\", \"device\": \"" + String(DEVICE_ID) + "\"}");
  });

  // Route not found
  server.onNotFound([]() {
    server.send(404, "text/plain", "Not Found");
  });

  server.begin();
  Serial.println("✅ HTTP server started on port 80");
  Serial.println("Device IP: ");
  Serial.println(WiFi.localIP());
}

// ============================================
// HANDLE INCOMING COMMANDS
// ============================================

void handleCommand() {
  if (!server.hasArg("cmd")) {
    server.send(400, "application/json", "{\"error\": \"Missing cmd parameter\"}");
    return;
  }

  String cmd = server.arg("cmd");
  long duration = 1000;  // Default duration

  // Get duration parameter if provided
  if (server.hasArg("duration")) {
    duration = server.arg("duration").toInt();
  }

  Serial.println("\n🚀 Received command from Cloud Function");
  Serial.println("Command: " + cmd);
  Serial.println("Duration: " + String(duration) + "ms");

  // Execute the command
  executeCommand(cmd, duration);

  // Send success response
  server.send(200, "application/json", "{\"status\": \"executed\", \"cmd\": \"" + cmd + "\"}");
}

// ============================================
// FIRESTORE COMMANDS POLLING (REMOVED)
// ============================================

// This function is no longer needed - commands come directly from Cloud Function via HTTP

// ============================================
// MARK COMMAND AS PROCESSED
// ============================================

// This function is no longer needed - Cloud Function handles status updates

// ============================================
// EXECUTE COMMAND
// ============================================

void executeCommand(String action, long duration) {
  Serial.println("\n╔══════════════════════════════════╗");
  Serial.println("║     EXECUTING COMMAND            ║");
  Serial.println("╚══════════════════════════════════╝");
  Serial.print("Action: ");
  Serial.println(action);
  Serial.println();

  // Execute the action
  String executionMessage = "";
  bool success = true;

  if (action == "BLINK") {
    executionMessage = blinkLED(duration);
  }
  else if (action == "TURN_ON") {
    setLED(true);
    executionMessage = "LED turned ON";
  }
  else if (action == "TURN_OFF") {
    setLED(false);
    executionMessage = "LED turned OFF";
  }
  else if (action == "PULSE") {
    executionMessage = pulseLED(duration);
  }
  else {
    executionMessage = "Unknown action: " + action;
    success = false;
    Serial.println("ERROR: " + executionMessage);
  }

  Serial.println("✅ " + executionMessage);
  Serial.println("═══════════════════════════════════\n");
}

// ============================================
// LED CONTROL FUNCTIONS
// ============================================

void setLED(bool state) {
  // Handle active-low LED configuration
  if (LED_ACTIVE_LOW) {
    digitalWrite(LED_PIN, state ? LOW : HIGH);
  } else {
    digitalWrite(LED_PIN, state ? HIGH : LOW);
  }

  Serial.print("💡 LED: ");
  Serial.println(state ? "ON" : "OFF");
}

String blinkLED(long duration) {
  int blinkCount = duration / 200;  // 200ms per blink (100ms on, 100ms off)
  if (blinkCount < 1) blinkCount = 1;

  Serial.print("✨ Blinking LED ");
  Serial.print(blinkCount);
  Serial.println(" times...");

  for (int i = 0; i < blinkCount; i++) {
    setLED(true);
    delay(100);
    setLED(false);
    delay(100);
  }

  // Ensure LED returns to default OFF state
  setLED(false);
  return "LED blinked " + String(blinkCount) + " times (returned to OFF)";
}

String pulseLED(long duration) {
  Serial.print("🌊 Pulsing LED for ");
  Serial.print(duration);
  Serial.println("ms...");

  unsigned long startTime = millis();

  while (millis() - startTime < duration) {
    // Fade in
    for (int i = 0; i <= 255; i += 5) {
      analogWrite(LED_PIN, LED_ACTIVE_LOW ? 255 - i : i);
      delay(10);
      if (millis() - startTime >= duration) break;
    }

    // Fade out
    for (int i = 255; i >= 0; i -= 5) {
      analogWrite(LED_PIN, LED_ACTIVE_LOW ? 255 - i : i);
      delay(10);
      if (millis() - startTime >= duration) break;
    }
  }

  // Ensure LED returns to default OFF state
  setLED(false);
  return "LED pulsed for " + String(duration) + "ms (returned to OFF)";
}
