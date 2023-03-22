#include <ESP8266WiFi.h>
#include "Timer.h"

///////////////Parameters/////////////////
// WIFI params
const char* ssid = "iPhoneA";
const char* password = "Rtzq-467";

// CSE params
// Set your Static IP address

const char* host = "172.20.10.6";
const int httpPort = 8080;

// AE params
const int aePort   = 80;
const char* origin   = "Cae_device1";
///////////////////////////////////////////

Timer t;

WiFiServer server(aePort);
int led = D6;                // the pin that the LED is atteched to
int sensor = D5;              // the pin that the sensor is atteched to
int state = LOW;             // by default, no motion detected
int val = 0;                 // variable to store the sensor status (value)
float sensorValue;

void setup() {

  Serial.begin(9600);
  delay(10);

  // Configure pins for LED control
  pinMode(led, OUTPUT);      // initalize LED as an output
  pinMode(sensor, INPUT);    // initialize sensor as an input
  Serial.begin(9600);        // initialize serial
  digitalWrite(led, 0);

  Serial.println();
  Serial.println();

  // Connect to WIFI network
  Serial.print("Connecting to ");
  Serial.println(ssid);
 
  WiFi.persistent(false);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Start HTTP server
  server.begin();
  Serial.println("Server started");

  // Create AE resource
  String resulat = send("/server",2,"{\"m2m:ae\":{\"rn\":\"MotionSensor\",\"api\":\"MotionSensor.company.com\",\"rr\":\"true\",\"poa\":[\"http://"+WiFi.localIP().toString()+":"+aePort+"\"]}}");
  
  if(resulat=="HTTP/1.1 201 Created"){
    // Create Container resource
    send("/server/MotionSensor",3,"{\"m2m:cnt\":{\"rn\":\"SensorMovement\"}}");

    // Create ContentInstance resource
    send("/server/MotionSensor/SensorMovement",4,"{\"m2m:cin\":{\"con\":\"0\"}}");
    
  }

  t.every(1000*5, push);
}

// Method in charge of receiving event from the CSE
void loop(){
  t.update();
  // Check if a client is connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  
  // Wait until the client sends some data
  Serial.println("new client");
  while(!client.available()){
    delay(1);
  }
  
  // Read the request
  String req = client.readString();
  Serial.println(req);
  client.flush();

  // Switch the LED state according to the received value
  if (req.indexOf("ON") != -1){
    digitalWrite(led, 1);
  }else if (req.indexOf("OFF") != -1){
    digitalWrite(led, 0);
  }else{
    Serial.println("invalid request");
    client.stop();
    return;
  }
  
  client.flush();

  // Send HTTP response to the client
  String s = "HTTP/1.1 200 OK\r\n";
  client.print(s);
  delay(1);
  Serial.println("Client disonnected");
 
}


// Method in charge of sending request to the CSE
String send(String url,int ty, String rep) {

  // Connect to the CSE address
  Serial.print("connecting to ");
  Serial.println(host);
  WiFiClient client;
 
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return "error";
  }

  
  // prepare the HTTP request
  String req = String()+"POST " + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "X-M2M-Origin: " + origin + "\r\n" +
               "Content-Type: application/json;ty="+ty+"\r\n" +
               "Content-Length: "+ rep.length()+"\r\n"
               "Connection: close\r\n\n" + 
               rep;

  Serial.println(req+"\n");

  // Send the HTTP request
  client.print(req);
               
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return "error";
    }
  }

  // Read the HTTP response
  String res="";
  if(client.available()){
    res = client.readStringUntil('\r');
    Serial.print(res);
  }
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  
  Serial.println();
  Serial.println("closing connection");
  Serial.println();
  return res;
}

void push(){
  val = digitalRead(sensor);   // read sensor value
  if (val == HIGH) {           // check if the sensor is HIGH
      Serial.println("Motion detected!"); 
       String data = String()+"{\"m2m:cin\":{\"con\":\""+val+"\"}}";
      send("/server/MotionSensor/SensorMovement",4,data);
    }
   
  else {
      delay(500);            
      if (state == HIGH){
        Serial.println("Motion stopped!");
    }
  }
  
 

}


