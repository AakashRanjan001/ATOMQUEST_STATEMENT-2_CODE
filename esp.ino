#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

// WiFi credentials
const char* ssid = "Airtel_Room-336";
const char* password = "rooM##336@@";

// API Information
const String weatherURL = "https://api.openweathermap.org/data/2.5/weather?";
const String ApiKey = "1bbaa94749e0f616bab4f0b192077fb1"; 
const String aqiURL = "http://api.openweathermap.org/data/2.5/air_pollution?"; 
const String apiForLatLong = "http://ip-api.com/json/"; 

// Modes
bool test = 1;
bool automode = false;
bool isopen = false;
bool isclose = false;
bool pauseAction = false;

// Location Information
float lat;
float lon;

// Create a WebServer object on port 80
WebServer server(80);

// Function to send ON/OFF commands to Arduino via Serial
void sendCommandToArduino(const String& command) {
    Serial.println(command); 
}

// Function to fetch location (latitude and longitude)
void getLocation() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(apiForLatLong);
        int httpResponseCode = http.GET();
        if (httpResponseCode > 0) {
            String payload = http.getString();
            Serial.println("Location API Response: ");
            Serial.println(payload);
            DynamicJsonDocument doc(1024);
            deserializeJson(doc, payload);
            lat = doc["lat"];
            lon = doc["lon"];
            Serial.print("Latitude: ");
            Serial.println(lat);
            Serial.print("Longitude: ");
            Serial.println(lon);
        } else {
            Serial.print("Error in HTTP request: ");
            Serial.println(httpResponseCode);
        }
        http.end();
    } else {
        Serial.println("WiFi disconnected");
    }
}

// Function to fetch weather data (sunrise and sunset times)
std::pair<String, String> getWeatherData() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String fullURL = weatherURL + "lat=" + String(lat) + "&lon=" + String(lon) + "&units=metric&appid=" + ApiKey;
        http.begin(fullURL);
        int httpCode = http.GET();
        if (httpCode > 0) {
            String JSON_Data = http.getString();
            Serial.println("Weather API Response:");
            Serial.println(JSON_Data);
            DynamicJsonDocument doc(2048);
            deserializeJson(doc, JSON_Data);
            long sunrise = doc["sys"]["sunrise"];
            long sunset = doc["sys"]["sunset"];
            String sunrise_time = timeExtract(sunrise);
            String sunset_time = timeExtract(sunset);
            Serial.println("Sunrise Time (IST): " + sunrise_time);
            Serial.println("Sunset Time (IST): " + sunset_time);
            http.end();
            return {sunrise_time, sunset_time};
        } else {
            Serial.println("Error fetching weather data");
        }
        http.end();
    }
    return {"", ""};
}

// Function to fetch AQI data and extract overall_aqi
void getAQIData() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String fullAQIURL = aqiURL + "lat=" + String(lat) + "&lon=" + String(lon) + "&appid=" + ApiKey; 
        http.begin(fullAQIURL);
        int httpCode = http.GET();
        if (httpCode > 0) {
            String JSON_Data = http.getString();
            Serial.println("AQI API Response:");
            DynamicJsonDocument doc(512);
            deserializeJson(doc, JSON_Data);

            // Extracting overall_aqi from the response
            if (doc.containsKey("list") && doc["list"].size() > 0) {
                int overallAqiValue = doc["list"][0]["main"]["aqi"]; 
                Serial.print("Current Overall AQI: ");
                Serial.println(overallAqiValue); 
            } else {
                Serial.println("AQI data not available in the response.");
            }
        } else {
            Serial.println("Error fetching AQI data");
        }
        http.end();
    } else {
        Serial.println("WiFi disconnected");
    }
}

// Function to convert timestamp to formatted time
String timeExtract(long timestamp) {
    timestamp += 19800; 
    long hours = (timestamp % 86400L) / 3600;
    long minutes = (timestamp % 3600) / 60;
    long seconds = timestamp % 60;
    char timeStr[9];
    snprintf(timeStr, sizeof(timeStr), "%02ld:%02ld:%02ld", hours, minutes, seconds);
    return String(timeStr);
}

// Define HTTP POST endpoint for control
void handleJsonRequest() {
    if (server.hasArg("plain")) {
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, server.arg("plain"));
        
        if (error) {
            Serial.print("JSON Parsing Failed: ");
            Serial.println(error.c_str());
            server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            return;
        }

     
        if (doc.containsKey("alarm")) {
            bool alarm = doc["alarm"];
            sendCommandToArduino(alarm ? "ALARM ON" : "ALARM OFF");
            Serial.printf("Alarm set to %s\n", alarm ? "ON" : "OFF");
        }

        if (doc.containsKey("automode")) {
            automode = doc["automode"];
            sendCommandToArduino(automode ? "AUTOMODE ON" : "AUTOMODE OFF");
            Serial.printf("Automode set to %s\n", automode ? "ON" : "OFF");
        }

        // Check for the working object
        if (doc.containsKey("working")) {
            JsonObject working = doc["working"];
            
            if (working.containsKey("open")) {
                isopen = working["open"];
                sendCommandToArduino(isopen ? "OPEN ON" : "OPEN OFF");
                Serial.printf("Open mode set to %s\n", isopen ? "ON" : "OFF");
            }
            
            if (working.containsKey("close")) {
                isclose = working["close"];
                sendCommandToArduino(isclose ? "CLOSE ON" : "CLOSE OFF");
                Serial.printf("Close mode set to %s\n", isclose ? "ON" : "OFF");
            }

            if (working.containsKey("pause")) {
                pauseAction = working["pause"];
                sendCommandToArduino(pauseAction ? "PAUSE ON" : "PAUSE OFF");
                Serial.printf("Pause mode set to %s\n", pauseAction ? "ON" : "OFF");
            }
        }

        // Respond with success
        server.send(200, "application/json", "{\"status\":\"success\"}");
    } else {
        server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"No JSON data\"}");
    }
}

void setup() {
    // Start Serial
    Serial.begin(115200);

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    int attempts = 0;

    while (WiFi.status() != WL_CONNECTED && attempts < 10) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
        attempts++;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Failed to connect to WiFi. Restarting...");
        ESP.restart(); 
    } else {
        Serial.println("WiFi connected!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP()); 
    }

    // Define HTTP POST endpoint for control
    server.on("/control", HTTP_POST, handleJsonRequest);

    // Start the server
    server.begin();
    Serial.println("Server started!");
}

void loop() {
    server.handleClient(); 

    if(test) {
        getLocation(); 
        std::pair<String, String> sunrise_sunset = getWeatherData(); 
        getAQIData(); 
        
        test = 0; 
    }

    static unsigned long lastCheckTime = 0;

    if (millis() - lastCheckTime > 60000) { 
        lastCheckTime = millis();

        getLocation(); 
        std::pair<String, String> sunrise_sunset_updated = getWeatherData(); 
        getAQIData(); 

   delay(500); 
}
}