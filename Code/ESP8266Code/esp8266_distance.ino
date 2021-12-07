#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include <Ultrasonic.h>
#include <PubSubClient.h>


WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
int delayTrue = 0;

const char* ssid="ORBI2709";
const char* password="ORBI2709!";
const char* mqtt_server = "mqtt.eclipse.org";
int LED=2;
int websockMillis=50;

const int trigPin = 12;
const int echoPin = 14;

long duration;
float distance;

unsigned long startMillis;  //some global variables available anywhere in the program
unsigned long currentMillis;
const unsigned long period = 500;  //the value is a number of mill

ESP8266WebServer server(80);
WebSocketsServer webSocket=WebSocketsServer(88);
String webSite,JSONtxt;


const char webSiteCont[] PROGMEM = 
R"=====(
<!DOCTYPE HTML>
<HTML>
<META name='viewport' content='width=device-width, initial-scale=1'>
<style>
#dynRectangle{
  width:0px;
  height:12px;
  top: 9px;
  background-color: red;
  z-index: -1;
  margin-top:8px;
}
#rectangle{
  width: 1401px;
  
  background-image: 
    linear-gradient(90deg, 
      rgba(73, 73, 73, 0.5) 0%, 
      rgba(73, 73, 73, 0.5) 1%, 
      transparent 1%
    ),linear-gradient(180deg, 
      #ffffff 50%, 
      transparent 50%
    ), 
    linear-gradient(90deg, 
      transparent 50%, 
      rgba(73, 73, 73, 0.5) 50%, 
      rgba(73, 73, 73, 0.5) 52%, 
      transparent 52%
    ), 
    linear-gradient(180deg, 
      #ffffff 70%, 
      transparent 70%
    ), 
    linear-gradient(90deg, 
      transparent 10%,
      rgba(73, 73, 73, 0.4) 10%, 
      rgba(73, 73, 73, 0.4) 12%, 
      transparent 12%, 
      
      transparent 20%,
      rgba(73, 73, 73, 0.4) 20%, 
      rgba(73, 73, 73, 0.4) 22%, 
      transparent 22%, 
      
      transparent 30%, 
      rgba(73, 73, 73, 0.4) 30%,
      rgba(73, 73, 73, 0.4) 32%, 
      transparent 32%, 
      
      transparent 40%, 
      rgba(73, 73, 73, 0.4) 40%, 
      rgba(73, 73, 73, 0.4) 42%, 
      transparent 42%, 
      
      transparent 60%, 
      rgba(73, 73, 73, 0.4) 60%, 
      rgba(73, 73, 73, 0.4) 62%, 
      transparent 62%, 
      
      transparent 70%, 
      rgba(73, 73, 73, 0.4) 70%, 
      rgba(73, 73, 73, 0.4) 72%, 
      transparent 72%, 
      
      transparent 80%, 
      rgba(73, 73, 73, 0.4) 80%, 
      rgba(73, 73, 73, 0.4) 82%, 
      transparent 82%, 
      
      transparent 90%, 
      rgba(73, 73, 73, 0.4) 90%, 
      rgba(73, 73, 73, 0.4) 92%, 
      transparent 92%
    );
  
  
  background-size: 140px 40px;
  min-height: 40px;
  
  /* only needed for labels */
  white-space:nowrap;
  font-size:0;
  margin:auto;
  margin-top:200px;
  padding:0;
}
label {
  font-size:16px;
  padding-top:2px;
  display:inline-block;
  width:140px;
  text-indent:3px;
}
</style>
<body>
<div id="rectangle">
  <label>0</label>
  <label>10</label>
  <label>20</label>
  <label>30</label>
  <label>40</label>
  <label>50</label>
  <label>60</label>
  <label>70</label>
  <label>80</label>
  <label>90</label>
  <label>100</label>
  <div id="dynRectangle"></div>
</div>
  
</body>
<SCRIPT>
  var a = 0;
  InitWebSocket()
  function InitWebSocket()
  {
    websock = new WebSocket('ws://'+window.location.hostname+':88/');
    websock.onmessage=function(evt)
    {
       JSONobj = JSON.parse(evt.data);
       var dist = parseInt(JSONobj.Distance);
       document.getElementById("dynRectangle").style.width = dist+"px";
    } // end of onmessage
      
  } // end of InıtWebSocket
</SCRIPT>
</HTML>
)=====";

void setup_wifi() {
   delay(100);
  // We start by connecting to a WiFi network
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) 
    {
      delay(500);
      Serial.print(".");
    }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void WebSite(){

  server.send(400,"text/html",webSiteCont);
  
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    //if you MQTT broker has clientID,username and password
    //please change following line to    if (client.connect(clientId,userName,passWord))
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
     //once connected to MQTT broker, subscribe command 
     //payload commands are as follows: 1 to get temp
     //0 is to get humidity, and 2 to delay data sent to broker  
      client.subscribe("OsoyooSensorCommand");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 6 seconds before retrying
      delay(6000);
    }
  }
} //end reconnect()

void setup() {
  Serial.begin(115200); // Starts the serial communication
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input

  WiFi.begin(ssid,password);
  while(WiFi.status() != WL_CONNECTED)
  {
    Serial.println(".");
    delay(500);  
  }
  WiFi.mode(WIFI_STA);
  Serial.println(" Start ESP ");
  Serial.println(WiFi.localIP());
  server.on("/",WebSite);
  server.begin();
  webSocket.begin();
  startMillis = millis();  //initial start time 
}

void loop() {
  webSocket.loop();
  server.handleClient();

  currentMillis = millis();  //get the current "time" (actually the number of milliseconds since the program started)
  if (currentMillis - startMillis >= period)  //test whether the period has elapsed
  {
    // Clears the trigPin
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    // Sets the trigPin on HIGH state for 10 micro seconds
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    // Reads the echoPin, returns the sound wave travel time in microseconds
    duration = pulseIn(echoPin, HIGH);
    
    // Calculate the distance
    distance= duration*0.034/2;
    
    // Prints the distance on the Serial Monitor
    Serial.print("Distance : ");
    Serial.println(distance);
    
    if(distance > 100)
      distance = 0;
    distance=distance*10;
    client.publish("OsoyooSensorData", distance);

    startMillis = currentMillis;  //IMPORTANT to save the start time of the current LED state.
  }
  String distaceStr = String(distance);
  JSONtxt = "{\"Distance\":\""+distaceStr+"\"}";
  webSocket.broadcastTXT(JSONtxt);
  
    
}