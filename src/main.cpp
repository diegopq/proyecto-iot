#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>



// Credenciales MQTT
const char *mqtt_server = "ioticos.org";
const int mqtt_port = 1883;
const char *mqtt_user = "98QpanPKd1m0wvi";
const char *mqtt_pass = "kzkzKzL2mHN6j1n";

// Credenciales de WiFi
const char* ssid     = "FAMPQ2.4GHz";
const char* password = "C1r1co/3shG6j07L";

//====== Tópicos ===========
const char *Raiz = "caw3AGrqrErOFYj";

//habitacion 1
//---Suscripciones
const char *Hab1_led_sub = "caw3AGrqrErOFYj/Hab1/Led";
const char *Hab1_vent_sub = "caw3AGrqrErOFYj/Hab1/Vent";
const char *Hab1_Auto_Cont_sub = "caw3AGrqrErOFYj/Hab1/Auto";
//----Publicaciones
const char *Hab1_tem_pub = "caw3AGrqrErOFYj/Hab1/Temp";

//habitacion 2
//---Suscripcion
const char *Hab2_led_sub = "caw3AGrqrErOFYj/Hab2/Led";
const char *Hab2_auto_led_sub = "caw3AGrqrErOFYj/Hab2/Auto";
//---Publicacion
const char *Hab2_Mov_pub = "caw3AGrqrErOFYj/Hab2/Mov";

//pasillo
//---Publicacion
const char *IntensidadLuz = "caw3AGrqrErOFYj/Pasillo/Intensidad";
const char *IntensidadLed = "caw3AGrqrErOFYj/Pasillo/Led";



// ==== Variables =====
WiFiClient espClient;
PubSubClient client(espClient);
int contconexion = 0;
char msg[25];
long initTime;


// Hab 1
int hab1_auto_state = 1;

// Hab 2
bool mov = LOW;
int hab2_auto_state = 1;

//Pasillo
const int freqPwm = 1000;
const int ledChannel = 0;
const int resolution = 8;
float ADC_LDR = 0.0;
int dutyCycle;

//====== prototipos de funcion =======
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void setup_wifi();
void Habitacion1();
void Habitacion1State(char* topic, String value);
void Habitacion2();
void Habitacion2State(char* topic, String value);
void SuscripcionHab1();
void SuscripcionHab2();
void Pasillo();

//===== Configuracion de pines ======
//hab 1
const int LEDHAB1 = 2;
const int LEDVENT = 5;
const int TEMPSENSORHAB1 =  4;

//hab 2
const int MOVSENSOR = 18;
const int MOVLEDSENSOR = 19; 

//pasillo
const int LDR_VAL = 39;
const int LED_LDR = 21;

// ====== Conf sensores =======
DHT dht1(TEMPSENSORHAB1, DHT11);


void setup() {
  Serial.begin(115200);
  initTime = millis();
  //configuracion de pines
  //hab1
  pinMode(LEDHAB1, OUTPUT);
  pinMode(LEDVENT, OUTPUT);
  pinMode(TEMPSENSORHAB1, INPUT);
  dht1.begin();
  digitalWrite(LEDHAB1, LOW);
  digitalWrite(LEDVENT, LOW);

  //hab2
  pinMode(MOVSENSOR, INPUT);
  pinMode(MOVLEDSENSOR, OUTPUT);

  //pasillo
  ledcSetup(ledChannel, freqPwm, resolution);
  ledcAttachPin(LED_LDR, ledChannel);

  setup_wifi();
  // Configuración conexión MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  //reconectar el wifi en caso de que se desconecte
  if(WiFi.status() != WL_CONNECTED){
    setup_wifi();
  }

  if (!client.connected()) {
    // Si no está conectado al broker se realiza la reconexión
    reconnect();
  }
  client.loop();
  if(millis() - initTime > 1500){
    initTime = millis();
    Habitacion1();
    Habitacion2();
    Pasillo();
  }
}


//configuracion de wifi
void setup_wifi(){
  delay(20);
  // Conexión WIFI
  Serial.println();
  Serial.print("Conectando a ssid: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED and contconexion <20) { 
    ++contconexion;
    delay(1000);
    Serial.print(".");
  }
  if (contconexion <20) {
      Serial.println("");
      Serial.println("WiFi conectado");
      Serial.println(WiFi.localIP());
  }
  else { 
    contconexion = 0;
      Serial.println("");
      Serial.println("Error de conexion");
  }
}

// Conexión MQTT
void reconnect() {
  // Si no hay conexión se entra dentro del ciclo
  while (!client.connected()) {
    Serial.println("Intentando conexión Mqtt...");
    // Se crea un cliente ID
    String clientId = "ESP32Client";
    clientId += String(random(0xffff), HEX);
    // Se intenta realizar l aconexión conectar
    if (client.connect(clientId.c_str(),mqtt_user,mqtt_pass)) {
      // Si hay conexión se imprime el mensaje
      Serial.println("Conectado con el Broker!");
      //===== Habitacion 1 =======
      SuscripcionHab1();
      //===== Habitación 2 =======
      SuscripcionHab2();
    } else {
      // Si no hay conexión se imprime el mensaje, el estado de conexión
      // y se intenta nuevamente dentro de 5 segundo
      Serial.print("falló :( con error -> ");
      Serial.print(client.state());
      Serial.println(" Intentamos de nuevo en 5 segundos");
      delay(5000);
    }
  }
}

// Función para cuando se recibe un mensaje por parte del broker de un tópico al
// cual se está subscrito
void callback(char* topic, byte* payload, unsigned int length){
  String incoming = "";
  Serial.print("Mensaje recibido desde -> ");
  Serial.print(topic);
  Serial.println("");
  for (int i = 0; i < length; i++) {
    incoming += (char)payload[i];
  }
  incoming.trim();
  Serial.println("Mensaje -> " + incoming);

//topicos para la habitacion 1
  Habitacion1State(topic, incoming);
//topicos para la habitacion 2
  Habitacion2State(topic, incoming);
}

//realiza la suscripcion a los topicos de la habitacion 1
void SuscripcionHab1(){
  // Nos suscribimos al tópico Raíz/Hab1/Led
      if(client.subscribe(Hab1_led_sub)){
        Serial.print("Suscripcion -- ");
        Serial.println(Hab1_led_sub);
        Serial.println(" -- ok");
      }
      else{
        Serial.println("fallo Suscripciión a --");
        Serial.println(Hab1_led_sub);
      }
      // Nos suscribimos al tópico Raíz/Hab1/Vent
      if(client.subscribe(Hab1_vent_sub)){
        Serial.print("Suscripcion -- ");
        Serial.println(Hab1_vent_sub);
        Serial.println(" -- ok");
      }
      else{
        Serial.println("fallo Suscripciión a --");
        Serial.println(Hab1_vent_sub);
      }
      // Nos suscribimos al tópico Raíz/Hab1/Auto
      if(client.subscribe(Hab1_Auto_Cont_sub)){
        Serial.print("Suscripcion -- ");
        Serial.println(Hab1_Auto_Cont_sub);
        Serial.println(" -- ok");
      }
      else{
        Serial.println("fallo Suscripciión a --");
        Serial.println(Hab1_Auto_Cont_sub);
      }
}

//realiz la suscripcion a los topicos de la habitacion 2
void SuscripcionHab2(){
  // Nos suscribimos al tópico Raíz/Hab2/Led
      if(client.subscribe(Hab2_led_sub)){
        Serial.print("Suscripcion -- ");
        Serial.println(Hab2_led_sub);
        Serial.println(" -- ok");
      }
      else{
        Serial.println("fallo Suscripciión a --");
        Serial.println(Hab2_led_sub);
      }
      // Nos suscribimos al tópico Raíz/Hab2/Auto
      if(client.subscribe(Hab2_auto_led_sub)){
        Serial.print("Suscripcion -- ");
        Serial.println(Hab2_auto_led_sub);
        Serial.println(" -- ok");
      }
      else{
        Serial.println("fallo Suscripciión a --");
        Serial.println(Hab2_auto_led_sub);
      }
}

//leer datos del sensor de temperatura y publica el dato
void Habitacion1(){
    float temp = dht1.readTemperature();
    while(isnan(temp)){
      Serial.println("Lectura fallida");
      delay(1000);
      temp = dht1.readTemperature();
    }
  //se prende el led de acuerdo a la temperatura si el control automatico esta activado
  if(hab1_auto_state == 1){
    if(temp >= 30.0){
      digitalWrite(LEDVENT, HIGH);
    }
    else{
      digitalWrite(LEDVENT, LOW);
    }
  }
    //publicacion de los datos de la habiatacion 1
    String strTemp = String(temp);
    strTemp.toCharArray(msg,25);
    client.publish(Hab1_tem_pub, msg);
  }

//aplica los cambios de acuerdo a lo recibido en los topicos suscritos
void Habitacion1State(char* topic, String value){
  if (strcmp(topic,Hab1_led_sub)==0)
  {
    //activar o desactivar el led de la habitacion 1
    int NUMERO = value.toInt();
    if(NUMERO==1)
    {
      digitalWrite(LEDHAB1, HIGH);
    }
    else
    {
      digitalWrite(LEDHAB1, LOW);
    }
  }
  else if(strcmp(topic, Hab1_Auto_Cont_sub) == 0){
    //activar o desactivar el control automatico del ventilador
    int num = value.toInt();
    if(num == 1){
      hab1_auto_state = 1;
    }
    else{
      hab1_auto_state = 0;
    }
  }
  else if(strcmp(topic, Hab1_vent_sub) == 0){
    int num = value.toInt();
    //solo si el control automatico esta desactivado se puede activar
    //manualmente el ventilador
    if(hab1_auto_state == 0){
      if(num == 1){
        digitalWrite(LEDVENT, HIGH);

      }
      else{
        digitalWrite(LEDVENT, LOW);
      }
    }
  }

}

//leer datos del sensor de movimiento y publica el dato
void Habitacion2(){
    mov = digitalRead(MOVSENSOR);
    if(hab2_auto_state == 1){
      if(mov == HIGH){
        //publica si el sensor detecta movimiento
        digitalWrite(MOVLEDSENSOR, HIGH);
        String strTemp = String(1);
        strTemp.toCharArray(msg,25);
        client.publish(Hab2_Mov_pub, msg);
      }
      else{
        digitalWrite(MOVLEDSENSOR, LOW);
        String strTemp = String(0);
        strTemp.toCharArray(msg,25);
        client.publish(Hab2_Mov_pub, msg);
      }
    }
    else{
      String strTemp = String(0);
      strTemp.toCharArray(msg,25);
      client.publish(Hab2_Mov_pub, msg);
    }

}

//aplica los cambios de acuerdo a lo recibido en los topicos suscritos
void Habitacion2State(char* topic, String value){
  if(strcmp(topic,Hab2_auto_led_sub)==0)
  {
    int num = value.toInt();
    //activa o desactiva el control automatico de led de habitacion 2
    if(num==1)
    {
      hab2_auto_state = 1;
      
    }
    else
    {
      hab2_auto_state = 0;
    }
  }
  else if(strcmp(topic, Hab2_led_sub) == 0){
    int num = value.toInt();
    if(hab2_auto_state == 0){
      if(num == 1){
        digitalWrite(MOVLEDSENSOR, HIGH);
      }
      else{
        digitalWrite(MOVLEDSENSOR, LOW);
      }
    }
  }
}

//lee datos de la resistencia y los publica tambien se modifica la intensidad del led
void Pasillo(){
  ADC_LDR = analogRead(LDR_VAL);
  float percent = ADC_LDR / 4095.0;
  dutyCycle = (255 * (1 - percent ));
  ledcWrite(ledChannel, dutyCycle);
  //publica los valores de la resistencia
  String intensidad = String(ADC_LDR);
  intensidad.toCharArray(msg,25);
  client.publish(IntensidadLuz, msg);
  //publica los valores del led
  String led = String(dutyCycle);
  led.toCharArray(msg, 25);
  client.publish(IntensidadLed, msg);
}