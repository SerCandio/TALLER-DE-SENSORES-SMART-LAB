#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

#include <DHT.h>
#include <DHT_U.h>

//Definir el tipo de sensor
#define DHTTYPE           DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

//Definir el pin digital del sensor
#define DHTPIN                  14

//Objeto de la libreria- clase del sensor
DHT dht(DHTPIN, DHTTYPE);

#define WIFI_TIMEOUT_CONNECTION           10000UL   //Timeout de conexion WiFi en 'ms'

#define SSID                              "ROSENFELD-2.4G"
#define PASSWORD                          "A8s1L2d3jZcKg96"

/*
#define SSID                              "HUAWEI Y6 2019"
#define PASSWORD                          "0a6837df87e"
*/

//Objeto WiFi Client
WiFiClient client;

//Objeto HTTPCliente
HTTPClient http;

#define TASK1_POLLRATE_MS       10000UL  //Tarea de leer sensor DHT11 y enviar (POST) data al servidor...
#define TASK2_POLLRATE_MS       2000UL  //Tarea de leer data del servidor y mostrar en placa (GET)

#define NUMBER_OF_TASKS         2     //Cantidad de tareas del sistema

//Definir variables de temporizacion de las tareas
static uint32_t timePrev[NUMBER_OF_TASKS]={0};

void setup() {
  //Inicalizar pines I/O , LED incorporado en placa como salida e iniciar en bajo
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  //Iniciar puerto serie
  Serial.begin(9600);

  //Inicializar sensor DHT
  dht.begin();

  //Configurar WiFi como STA: Unirse al AP
  setup_wifi();

  //Iniciar la temporizacion de tareas
  Task_Init();

}

void loop() {
  // put your main code here, to run REPETITIVA:

  //Tarea TASK que se ejecuta cada 'TASK1_POLLRATE_MS'
  if((millis() - timePrev[0])>=TASK1_POLLRATE_MS){
    timePrev[0]=millis();

  //Tomar muestras de temperatura y humedad
  float t=dht.readTemperature();
  float h=dht.readHumidity();

  //Imprimir data en puerto CONSOLA
  Serial.print("\r\nTemperature [°C]: "),
  Serial.print(t, 2);
  Serial.println("");
  Serial.print("\r\nHumidity [%RH]: "),
  Serial.print(h, 2);
  Serial.println("");

  //Crear el documento en formato JSON
  StaticJsonDocument<256> doc;

  //Añadir valores al JSON
  doc["temperature"]=t;
  doc["humidity"]=h;

  //Generar la Trama JSON
  String payloadUPLINK;
  serializeJson(doc, payloadUPLINK); //Serializar documento JSON en dato 'String'
  Serial.println("\r\nJSON payload UPLINK [TX]: ");
  Serial.println(payloadUPLINK);
  Serial.println("");

  //Hasta aqui se imprime..
  /*

  */

 //ARMAR una PETICION HTTP desde el cliente
  http.begin(client, "http://industrial.api.ubidots.com/api/v1.6/devices/nodemcu-esp8266-ubidots/");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Auth-Token", "BBUS-FCNCDjvXGpmc8nQ8vlgTqNgFvdcRqj");

  //ENVIAR LA PETICION + PAYLOAD DE UPLINK
  int httpCode=http.POST(payloadUPLINK);

  //Evaluar el codigo de respuesta desde el servidor (debe ser 200 OK)
  Serial.print("\r\nHTTP Response Code(from server): ");
  Serial.println(httpCode, DEC);
  Serial.println("");

  delay(300);

  if(httpCode>0){
    String payloadDOWNLINK=http.getString();
    Serial.println("\r\nJSON payload DOWNLINK(data sent-status) [RX]: ");
    Serial.println(payloadDOWNLINK);
  }

}

//////////////////////////////////////////////////////////////////////////////////////////////////////

//Tarea TASK que se ejecuta cada 'TASK2_POLLRATE_MS'
if((millis() - timePrev[1])>=TASK2_POLLRATE_MS)
{
   timePrev[1]=millis();

   //ARMAR una PETICION HTTP desde el cliente
  http.begin(client, "http://industrial.api.ubidots.com/api/v1.6/devices/nodemcu-esp8266-ubidots/pushbutton/lv");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Auth-Token", "BBUS-FCNCDjvXGpmc8nQ8vlgTqNgFvdcRqj");

   //Llamada al metodo GET
  int httpCode=http.GET();

  //Evaluar el codigo de respuesta desde el servidor
  Serial.print("HTTP Response Code(from server): ");
  Serial.println(httpCode, DEC);
  Serial.println("");

  delay(300);
  
  //Imprimir el contenido 
  if(httpCode>0){
    String payloadDOWNLINK=http.getString();
    Serial.println("\r\nPayload DOWNLINK(data-received status) [RX]: ");
    Serial.println(payloadDOWNLINK);

    //Tomar una decision acorde el payload
    if(payloadDOWNLINK.compareTo("0.0")){
       digitalWrite(LED_BUILTIN, LOW);
    }

    if(payloadDOWNLINK.compareTo("1.0")){
       digitalWrite(LED_BUILTIN, HIGH);
    }

  }


}



}


void setup_wifi(void){
    //Conectarse a la red WiFi
    WiFi.begin(SSID, PASSWORD);

    //Imprimir info al usuario
    Serial.print("\r\nConnecting to WiFi");

    unsigned long start=millis();

    while( (WiFi.status()!=WL_CONNECTED) && (millis() - start)<=WIFI_TIMEOUT_CONNECTION  )
    {
      delay(300);
      Serial.print(".");

    }

    if(WiFi.status()!=WL_CONNECTED)
    {
      Serial.println("\r\nCould not connect to AP, try again !!");
      return;
    }

    Serial.print("\r\nConnected to Access Point: ");
    Serial.println(SSID);
    Serial.print("Local IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC Address(esp8266): ");
    Serial.println(WiFi.macAddress());
    Serial.println("");
}

void Task_Init(void){
    //Iniciar el arreglo de temporizacion de tareas
    for(int i=0; i<NUMBER_OF_TASKS; i++){
      timePrev[i]=millis();
    }
}