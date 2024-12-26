#include <IRremote.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>

//Put your WiFi Credentials here
const char* ssid = "yourssid";
const char* password = "26/12/2024Win";

//URL Endpoint for the API
String URL = "https://api.openweathermap.org/data/2.5/weather?";
String ApiKey = "1bbaa94749e0f616bab4f0b192077fb1";
String CurrTimeURL = "https://worldtimeapi.org/api/timezone/Etc/UTC";

// Replace with your location Credentials
String lat = "21.250000";
String lon = "81.629997";

int tempInput = 7;          // Temperature input
int IrInput = 5;             // IR sensor input
int clockWise = 3;           // Motor pin for clock-wise -> Open curtain
int antiClockWise = 4;       // Motor pin for anti-clock-wise -> Close curtain
bool autoMode = true;            // Automatic mode: true -> auto open and close
int lengthOfCurtain = 150;   // User-defined curtain rod length
float lengthCovered = 10;    // Length covered by the device
// lengthCovered ==10 -> curtain is closed
// lengthCovered ==lengthOfCurtain-10 -> curtain is open

// For LCD display
LiquidCrystal_I2C lcdDisplay(0x27, 16, 2);

// for temperature sensor 
OneWire oneWire(tempInput);
DallasTemperature sensors(&oneWire);

// For IR sensor (updated IRremote structure)
IRrecv irrecv(IrInput);
decode_results IRresults;  // Struct for IR data (use decode_results)

void setup()
{
  sensors.begin();
  Serial.begin(9600);

  // Defining motor pins
  pinMode(clockWise, OUTPUT);
  pinMode(antiClockWise, OUTPUT);
  pinMode(IrInput, INPUT);

  // LCD setup
  lcdDisplay.init();
  lcdDisplay.backlight();
  lcdDisplay.display();

  // IR sensor setup
  IrReceiver.begin(IrInput);
}

void loop()
{
  // Read temperature
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);
  printTemperatureValue(temperature);

  // Automatic curtain control
  if (autoMode){
    if (temperature >= 15 && temperature <= 30) {
      commandToMotor("Open");
    }
    else {
      commandToMotor("Close");
    }
  }

  // Check for IR remote signals
  if (IrReceiver.decode()) {
    autoMode=false;
    digitalWrite(clockWise, LOW);
    digitalWrite(antiClockWise, LOW);
    unsigned long IRcode = IrReceiver.decodedIRData.decodedRawData;
    IRControllerFunction(IRcode);
    Serial.println(IRcode);
    IrReceiver.resume();
  }

  lengthConstraint();

  // length controll 
  if(digitalRead(clockWise)==HIGH){
    Serial.println("Clockwise");
    lengthCovered-=5;
  }
  if(digitalRead(antiClockWise)==HIGH){
    lengthCovered+=5;
    Serial.println("Anti-clockwise");
  }
  
  //length covered check

  delay(100);
}

// Function to print temperature on LCD
void printTemperatureValue(float val)
{
  lcdDisplay.clear();
  lcdDisplay.setCursor(0, 0);
  lcdDisplay.print("Temp: ");
  lcdDisplay.print(val);
  lcdDisplay.print(" C");
  lcdDisplay.setCursor(0, 1);
  lcdDisplay.print("Len : ");
  lcdDisplay.print(lengthCovered);
}

// Function to control DC motor
void commandToMotor(String command)
{
  //clockwise
  if (command == "Open"){
    digitalWrite(clockWise, HIGH);
    digitalWrite(antiClockWise, LOW);
    lengthConstraint();
  }
  //anti-clockwise
  else if (command == "Close"){
    digitalWrite(clockWise, LOW);
    digitalWrite(antiClockWise, HIGH);
    lengthConstraint();
  }
  //stop
  else{
    digitalWrite(clockWise, LOW);
    digitalWrite(antiClockWise, LOW);
  }
}

// Function to measure length covered
void lengthConstraint()
{
  if((lengthCovered<=10) && digitalRead(clockWise)==HIGH){
    Serial.println("completely opened");
    digitalWrite(clockWise, LOW);
    digitalWrite(antiClockWise, LOW);
  }
  if((lengthCovered>=(lengthOfCurtain-10)) && digitalRead(antiClockWise)==HIGH){
    Serial.println("completely closed");
    digitalWrite(clockWise, LOW);
    digitalWrite(antiClockWise, LOW);
  }
}

// Function to handle IR remote control
void IRControllerFunction(unsigned long code)
{
  // Replace these values with your IR remote button codes
  unsigned long close_code=4211392256;
  unsigned long open_code=4177968896;
  unsigned long stop_code=4194680576;
  unsigned long AutoOperation_mode_code=4278238976;
  if (code ==open_code){
    commandToMotor("Open");
  }
  else if (code == close_code){
    commandToMotor("Close");
  }
  else if (code == stop_code){
    commandToMotor("Stop");
  }
  else if (code == AutoOperation_mode_code){
    autoMode=true;
  }
}
