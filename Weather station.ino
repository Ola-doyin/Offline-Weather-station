
#include <ESP8266WiFi.h>
//#include <WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

const char* ssid     = "Weather Station";
const char* password = "wstation12";

#define DHTPIN 5     // Digital pin connected to the DHT sensor
#define DHTTYPE    DHT11     // DHT 11

DHT dht(DHTPIN, DHTTYPE);

//Variables
int channel_no = 5, max_con = 5;
float temp = 0.0;
float hum = 0.0;
float co = 0.0;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

unsigned long previousMillis = 0;    // will store last time server was updated

const long interval = 1000;  //Update server every 10seconds

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .ws-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>Weather Station</h2>
  <p>
    <span class="ws-labels">CO conc</span>
    <span id="cogas">%CO%</span>
    <sup class="units">ppm</sup>
  </p>
  <p>
    <span class="ws-labels">Temperature</span> 
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <span class="ws-labels">Humidity</span>
    <span id="humidity">%HUMIDITY%</span>
    <sup class="units">%</sup>
  </p>
</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 1000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 1000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("cogas").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/cogas", true);
  xhttp.send();
}, 1000 ) ;
</script>
</html>)rawliteral";

// Replaces placeholder with Sensor values
String processor(const String& var){
  //Serial.println(var);
  if(var == "TEMPERATURE"){
    return String(temp);
  }
  else if(var == "HUMIDITY"){
    return String(hum);
  }
  else if(var == "CO"){
    return String(co);
  }
  return String();
}


void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  dht.begin();

  delay(5000);
  Serial.println("\nStarting Hotspot…");
  WiFi.softAP(ssid, password, channel_no, false, max_con);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("IP address: ");
  Serial.println(IP);

  // Print ESP8266 Local IP Address
//  Serial.print("Local IP address: ");
//  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(temp).c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(hum).c_str());
  });
  server.on("/cogas", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(co).c_str());
  });

  // Start server
  server.begin();
}


float calculatePPM(){
  float sensor = 0;
  float sensorArray[5] = {};
  for (int i = 0; i<5; i++){    
     sensorArray[i] = analogRead(A0);
     sensor += sensorArray[i];
     //Serial.println(sensor);
     delay(250);
  }
  sensor = sensor/5;
  float lgPPM, PPM;
  float Vrl = (float)sensor * 5/1024;
  float ratio = (5 - Vrl)/Vrl;
  lgPPM = (-1.4931*log(ratio)) + 4.6518;
  float calppm = exp(lgPPM);
  //ppm = pow((0.04435 * ratio), (1/(-0.66975)));
  PPM = map(calppm, 0, 800, 0, 230.2);
  return PPM;
}

void loop(){  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {    
    previousMillis = currentMillis; //last time you updated the sensor values    

    //DHT sensor
    float newtemp = dht.readTemperature(); //new temperature    
    delay(2000);
    if (isnan(newtemp)) {
      Serial.println("Failed to read temperature from sensor!");
    }
    else {
      temp = newtemp;
      Serial.print("Temperature: ");
      Serial.println(temp);
    }
   
    float newhum = dht.readHumidity();  //new humidity
    delay(2000);
    if (isnan(newhum)) {
      Serial.println("Failed to read humidity from sensor!");
    }
    else {
      hum = newhum;
      Serial.print("Humidity: ");
      Serial.println(hum);
    }

    //MQ-7 sensor
    float newco = calculatePPM();  //new CO
     if (isnan(newco)) {
      Serial.println("Failed to read CO from sensor!");
    }
    else {
      co = newco;
      Serial.print("CO: ");
      Serial.println(co);
    }
  }
}
