#include <Arduino.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>
#include <Nextion.h>
#define nextion Serial2
Nextion myNextion(nextion, 9600);
#define DEBUG true
#define OLED false
#define NEXTION true
#if OLED == true
#include <Wire.h> 	// libreria para bus I2C
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//PANTALLA
#define ANCHO 128 	// reemplaza ocurrencia de ANCHO por 128
#define ALTO 64 	// reemplaza ocurrencia de ALTO por 64
#define OLED_RESET 4 	// necesario por la libreria pero no usado
Adafruit_SSD1306 oled(ANCHO, ALTO, &Wire, OLED_RESET); // crea objeto
#endif
//CONFIGURACION
//MODOS DE CONDUCCIÓN
const uint8_t FactorEco = 4;
const uint8_t FactorNormal = 2;
const uint8_t FactorSport = 1;
const uint8_t CE_PIN = 9; 	//Declaremos los pines CE y el CSN
const uint8_t CSN_PIN = 10;
byte RFPipe[5] ={0x60, 0x59, 0x70, 0x54, 0x01}; //Variable con la dirección del
canal por donde se va a transmitir
const int EstoyVivoPeriod = 100; 	//Periodo de envio de la señal Estoy Vivo (en
milisegundos)
const uint8_t ValueSteps = 5; //Incrementos en los valores leidos por los
potenciometros
const int8_t ValueZero = 6; 	//Valor a partir del cual la lectura analógica no se
considera 0 (incluido este valor).
const uint8_t PinNivelacion = A13; 	//Pin Analogico donde está conectado el
potenciometro para la nivelacion
const uint8_t PinGiro = A12; 	//Pin Analogico donde está conectado el
potenciometro para el giro
const uint8_t PinVelocidad = A11; 	//Pin Analogico donde está conectado el
potenciometro para la velocidad
const uint8_t PinAceleracion= A14; 	//Pin Analogico donde está conectado el
acelerador del motor (r.p.m)
const uint8_t PinBombAux= A6; 	//Queda por hacer


const uint8_t PinModoConduccion = A2; //Pin Analogico para el modo de conducción
(Eco, Normal o Sport)
const uint8_t PinModoLuces = A5; 	//Pin Analogico para leer el modo de las
luces
const uint8_t PinModoGiro = A1; 	//Pin Analogico para el modo de giro (normal,
cangrejo, super)
const uint8_t Bocina= 3; 	//Pin digital donde está conectado el cable
de control (modo PULL DOWN) de la bocina
const uint8_t Arranque= 5; 	//Pin digital donde está conectado el cable
de control (modo PULL DOWN) del arranque
const uint8_t InterruptorRF= 11; 	//Pin digital que desactiva todas las
funciones del mando inalámbrico salvo la seta de emergencia
const uint8_t SetaEmergencia= 12; 	//Pin digital Seta de Emergencia
const uint8_t NivAuto=7; 	//Futura nivelación automática del vehículo
const uint8_t Autonomo=8; 	//Pin digital para activar el modo
autónomo/manual
const uint8_t idModulo[4] = {0, 0, 0, 101};
const uint8_t idNivelacion = 5;
const uint8_t idVelocidad = 2;
const uint8_t idAceleracion= 10;
const uint8_t idGiro= 3;
const uint8_t idBocina= 102;
const uint8_t idArranque= 50;
const uint8_t idNivAuto= 51;
const uint8_t idAutonomo= 200;
const uint8_t idLuces= 100;
const uint8_t idBombaAux= 60;
//------------------------------------------
//DEFINICIONES
RF24 radio(CE_PIN, CSN_PIN); //creamos el objeto radio (NRF24L01)
int8_t ReadAnalogValue(uint8_t port, int portmin, int portmax, int8_t outmin,
int8_t outmax, uint8_t steps);
bool ReadDigitalValue(uint8_t port);
int8_t CalcularValorPasos(int8_t value, uint8_t steps);
void EnviarEstoyVivo();
void LeeEnviarValores();
void LeerEnviarValorNivelacion();
void LeerEnviarValorVelocidad();
void LeerEnviarValorGiro();
void LeerEnviarValorBocina();
void LeerEnviarValorArranque();
void LeerEnviarValorSetaEmergencia();
void LeerEnviarValorRF();
void LeerEnviarValorBombaAux();
void LeerEnviarValorNivAuto();
void LeerEnviarValorAceleracion();
void LeerEnviarValorAutonomo();
void LeerEnviarValorLuces();
void imprimeNextionGauge ();
void statusbattery ();
unsigned long lastTimeEstoyVivo = 0;
int8_t lastValueNivelacion = 0;
int8_t lastValueVelocidad = 0;


int8_t lastValueGiro = 0;
int8_t lastValueAceleracion =0;
uint8_t lastValueModoConduccion = 0;
bool lastValueBocina =false;
bool lastValueArranque =false;
bool lastValueSetaEmergencia= false;
bool lastValueRF= false;
bool lastValueNivAuto= false;
bool lastValueAutonomo= false;
uint8_t lastValueModoGiro = 0;
int lastvalueluces=0;
int8_t lastValueBombAux =0;
//----------------------------------------
void setup() {
//Tiempo para poder cargar el ARDUINO
//delay(2000);
//NEXTION
#if NEXTION == true
Serial.begin(9600);
myNextion.init();
delay(2500);
myNextion.sendCommand("page 1");
delay(2500);
myNextion.sendCommand("page 2");
//Inicializamos valores
myNextion.setComponentText("page2.t0","0");
myNextion.setComponentText("page2.t1","MANUAL");
myNextion.setComponentText("page2.t2","0");
myNextion.setPictureValue("page2.p0",3);
#endif
// //PANTALLA
#if OLED == true
Wire.begin(); 	// inicializa bus I2C
oled.begin(SSD1306_SWITCHCAPVCC, 0x3C); 	// inicializa pantalla con direccion
0x3C
#endif
//Configuramos el puerto serie si tenemos la depuracion activa
#if DEBUG == true
Serial.begin(115200);
#endif
radio.begin(); //inicializamos el NRF24L01
radio.openWritingPipe(RFPipe); //Abrimos un canal de escritura
lastTimeEstoyVivo = millis();
//Pines digitales


pinMode(Bocina,INPUT);
pinMode(Arranque,INPUT);
pinMode(SetaEmergencia,INPUT);
pinMode(InterruptorRF,INPUT);
pinMode(NivAuto,INPUT);
pinMode(Autonomo,INPUT);
LeeEnviarValores();
//Leer variables 1 vez para saber en que posición se quedaron (solo en
interruptores, los pulsadores vuelven a su posición)
//Leer todas las variables al principio
// LeerEnviarValorNivelacion();
// LeerEnviarValorVelocidad();
// LeerEnviarValorGiro();
// LeerEnviarValorBocina();
// LeerEnviarValorArranque();
// LeerEnviarValorSetaEmergencia();
// LeerEnviarValorRF();
// LeerEnviarValorNivAuto();
// LeerEnviarValorAceleracion();
// LeerEnviarValorAutonomo();
// LeerEnviarValorLuces();
// LeerEnviarValorBombaAux();
}
void loop(){
//Leemos los valores y enviamos los datos.
LeeEnviarValores();
//Enviamos la señal de Estoy Vivo.
EnviarEstoyVivo();
}
void LeeEnviarValores() {
//Leer RF
LeerEnviarValorRF();
//Seta de emergencia
LeerEnviarValorSetaEmergencia();
//Modo autónomo/manual
LeerEnviarValorAutonomo();
if (lastValueSetaEmergencia == false && lastValueRF == true && lastValueAutonomo
== false) {
//Nivelacion
LeerEnviarValorNivelacion();
//Velocidad
LeerEnviarValorVelocidad();
//Giro


LeerEnviarValorGiro();
//Aceleracion del motor
LeerEnviarValorAceleracion();
//Bocina
LeerEnviarValorBocina();
//Arranque
LeerEnviarValorArranque();
//Bomba auxiliar
LeerEnviarValorBombaAux();
//Nivelación Automática
LeerEnviarValorNivAuto();
//Luces
LeerEnviarValorLuces();
//Bomba Auxiliar
LeerEnviarValorBombaAux();
#if NEXTION ==true
//Gauge Nextion
imprimeNextionGauge();
statusbattery();
#endif
}
}
#if NEXTION == true
void imprimeNextionGauge() {
int leerPosPotenciometro= analogRead(A14);
int lecturaAnalogicaNextion= map(leerPosPotenciometro,0,1023,0,242); //PASAR A
GRADOS
if (lecturaAnalogicaNextion <20){
lecturaAnalogicaNextion+=315;
}
else{
lecturaAnalogicaNextion-=21;
}
myNextion.setComponentValue("page2.z0",lecturaAnalogicaNextion);
}
void statusbattery() {
float valueAnalogBattery=analogRead(A0);
float volt_battery = ((((valueAnalogBattery*5)/1023)*3.208)+1.35);
int volt_battery_porcent = (((volt_battery - 9)*100) / (12.2-9));
String datolecturaString_Bat =String(volt_battery_porcent);
myNextion.setComponentText("page2.t3",datolecturaString_Bat);
if (volt_battery_porcent>75 && volt_battery_porcent<=100){
myNextion.setPictureValue("page2.p6",21);
}
else if (volt_battery_porcent>35 && volt_battery_porcent<=75){


myNextion.setPictureValue("page2.p6",23);
}
else if (volt_battery_porcent>10 && volt_battery_porcent<=35){
myNextion.setPictureValue("page2.p6",22);
}
else {
myNextion.setPictureValue("page2.p6",24);
}
}
#endif
void LeerEnviarValorNivelacion() {
byte datos[12]; // Desde el byte 0 al 11
//Leemos el potenciometro de la nivelación
int8_t currentValueNivelacion = ReadAnalogValue(PinNivelacion, 0, 1023, -100,
100, ValueSteps);
#if DEBUG == true
Serial.println("Nivelacion :");
Serial.println(currentValueNivelacion);
#endif
if(currentValueNivelacion != lastValueNivelacion) { //Si el valor es distinto,
envia el nuevo valor y setea el valor actual como el ultimo.
#if NEXTION == true
String datolecturaString_Niv_2 =String(currentValueNivelacion);
myNextion.setComponentText("page2.t2",datolecturaString_Niv_2);
#endif
datos[0] = idModulo[0];
datos[1] = idModulo[1];
datos[2] = idModulo[2];
datos[3] = idModulo[3];
datos[4] = idNivelacion;
if (currentValueNivelacion < 0) {
//Si la nivelación es negativa se entiende que es el lado derecho el que debe
subir (pistones 2 y 4)
datos[5] = 0; //piston 1
datos[6] = -currentValueNivelacion; //piston 2
datos[7] = 0; //piston 3
datos[8] = -currentValueNivelacion; //piston 4
}
else {
//Si la nivelación es positiva se entiende que es el lado izquierdo el que
debe subir (pistones 1 y 3)
datos[5] = currentValueNivelacion; //piston 1
datos[6] = 0; //piston 2
datos[7] = currentValueNivelacion; //piston 3
datos[8] = 0; //piston 4
}
datos[9] = 0;
datos[10] = 0;


datos[11] = 0;
radio.write(datos, sizeof(datos)); //Se envian los datos por RF
lastValueNivelacion = currentValueNivelacion; //Se establece la nueva medicion
como la ultima.
}
}
void LeerEnviarValorVelocidad() {
byte datos[12]; // Desde el byte 0 al 11
//Leemos el potenciometro de velocidad
int8_t currentValueVelocidad = ReadAnalogValue(PinVelocidad, 0, 1023, -100, 100,
ValueSteps);
Serial.println(currentValueVelocidad);
#if DEBUG == true
Serial.println("Velocidad :");
Serial.println(currentValueVelocidad);
#endif
//Leemos el modo de conducción
int value = analogRead(PinModoConduccion);
uint8_t factor = 1;
if (value>=450 && value<=550) {
factor= FactorSport;
} else if (value>=850 && value<=950){
factor= FactorEco;
} else {
factor= FactorNormal;
}
if (lastValueModoConduccion!=factor) {
#if NEXTION == true
if (factor == FactorEco) {
myNextion.setPictureValue("page2.p4",13);
}
else if (factor == FactorSport) {
myNextion.setPictureValue("page2.p4",15);
}
else{
myNextion.setPictureValue("page2.p4",14);
}
#endif
#if OLED == true
if (factor == FactorEco) {
oled.clearDisplay(); 	// limpia pantalla
oled.setTextColor(WHITE); 	// establece color al unico
disponible (pantalla monocromo)


oled.setCursor(0, 0); 	// ubica cursor en inicio de
coordenadas 0,0
oled.setTextSize(1); 	// establece tamano de texto en 1
oled.print(" MODO DE CONDUCCION"); 	// escribe en pantalla el texto
oled.setCursor (10, 30); 	// ubica cursor en coordenas 10,30
oled.setTextSize(2); 	// establece tamano de texto en 2
oled.print(" ECO"); 	// escribe en pantalla el texto
oled.display(); 	// muestra en pantalla todo lo
establecido anteriormente
} else if (factor == FactorSport) {
oled.clearDisplay(); 	// limpia pantalla
oled.setTextColor(WHITE); 	// establece color al unico
disponible (pantalla monocromo)
oled.setCursor(0, 0); 	// ubica cursor en inicio de
coordenadas 0,0
oled.setTextSize(1); 	// establece tamano de texto en 1
oled.print(" MODO DE CONDUCCION"); 	// escribe en pantalla el texto
oled.setCursor (10, 30); 	// ubica cursor en coordenas 10,30
oled.setTextSize(2); 	// establece tamano de texto en 2
oled.print(" SPORT"); 	// escribe en pantalla el texto
oled.display(); 	// muestra en pantalla todo lo
establecido anteriormente
} else {
oled.clearDisplay(); 	// limpia pantalla
oled.setTextColor(WHITE); 	// establece color al unico
disponible (pantalla monocromo)
oled.setCursor(0, 0); 	// ubica cursor en inicio de
coordenadas 0,0
oled.setTextSize(1); 	// establece tamano de texto en 1
oled.print(" MODO DE CONDUCCION"); 	// escribe en pantalla el texto
oled.setCursor (10, 30); 	// ubica cursor en coordenas 10,30
oled.setTextSize(2); 	// establece tamano de texto en 2
oled.print(" NORMAL"); 	// escribe en pantalla el texto
oled.display(); 	// muestra en pantalla todo lo
establecido anteriormente
}
#endif
lastValueModoConduccion = factor;
}
currentValueVelocidad = currentValueVelocidad/factor;
#if NEXTION == true
String datolecturaString_Vel =String(currentValueVelocidad);
myNextion.setComponentText("page2.t0",datolecturaString_Vel);
#endif
if(currentValueVelocidad != lastValueVelocidad) { //Si el valor es distinto,
envia el nuevo valor y setea el valor actual como el ultimo.
datos[0] = idModulo[0];


datos[1] = idModulo[1];
datos[2] = idModulo[2];
datos[3] = idModulo[3];
datos[4] = idVelocidad;
datos[5] = currentValueVelocidad; //DELANTERA
datos[6] = currentValueVelocidad; //DELANTERA
datos[7] = 0; //TRASERA
datos[8] = 0; //TRASERA
datos[9] = 0;
datos[10] = 0;
datos[11] = 0;
radio.write(datos, sizeof(datos)); //Se envian los datos por RF
lastValueVelocidad = currentValueVelocidad; //Se establece la nueva medicion
como la ultima.
}
}
void LeerEnviarValorGiro() {
byte datos[12]; // Desde el byte 0 al 11
//Leemos el potenciometro de giro
int8_t currentValueGiro = ReadAnalogValue(PinGiro, 0, 1023, -100, 100,
ValueSteps);
#if DEBUG == true
Serial.println("Giro :");
Serial.println(currentValueGiro);
#endif
//Modo de giro
int currentAnalogModoGiro = analogRead(PinModoGiro);
uint8_t currentModoGiro;
if (currentAnalogModoGiro >= 450 && currentAnalogModoGiro <= 550){
currentModoGiro = 1;
} else if (currentAnalogModoGiro >= 850 && currentAnalogModoGiro <= 950) {
currentModoGiro = 0;
} else {
currentModoGiro = 2;
}
if (lastValueModoGiro != currentModoGiro || lastValueGiro != currentValueGiro) {
datos[0] = idModulo[0];
datos[1] = idModulo[1];


datos[2] = idModulo[2];
datos[3] = idModulo[3];
datos[4] = idGiro;
if (currentModoGiro == 1) {
//MODO CANGREJO
if (currentValueGiro < 10 and currentValueGiro > -10) {
//No gira debido a que el valor de giro no es el mínimo exigido (para
evitar perturbaciones)
datos[5] = 0;
datos[6] = 0;
datos[7] = 0;
datos[8] = 0;
} else {
//Proximamente solo habrá que controlar un pistón delantero y uno trasero
datos[5] = currentValueGiro;
datos[6] = currentValueGiro;
datos[7] = currentValueGiro;
datos[8] = currentValueGiro;
#if NEXTION == true
myNextion.setPictureValue("page2.p5",19);
#endif
}
} else if (currentModoGiro == 2) {
//MODO GIRO COMPLETO
if (currentValueGiro < 10 and currentValueGiro > -10) {
//No gira debido a que el valor de giro no es el mínimo exigido (para
evitar perturbaciones)
datos[5] = 0;
datos[6] = 0;
datos[7] = 0;
datos[8] = 0;
} else {
//Proximamente solo habrá que controlar un pistón delantero y uno trasero
datos[5] = currentValueGiro;
datos[6] = currentValueGiro;
datos[7] = -currentValueGiro;
datos[8] = -currentValueGiro;
#if NEXTION == true
myNextion.setPictureValue("page2.p5",18);
#endif
}


} else {
//MODO NORMAL
if (currentValueGiro < 10 and currentValueGiro > -10) {
//No gira debido a que el valor de giro no es el mínimo exigido (para
evitar perturbaciones)
datos[5] = 0;
datos[6] = 0;
datos[7] = 0;
datos[8] = 0;
} else {
//Proximamente solo habrá que controlar un pistón delantero y uno trasero
datos[5] = currentValueGiro;
datos[6] = currentValueGiro;
datos[7] = 0;
datos[8] = 0;
#if NEXTION == true
myNextion.setPictureValue("page2.p5",17);
#endif
}
}
datos[9] = 0;
datos[10] = 0;
datos[11] = 0;
radio.write(datos, sizeof(datos)); //Se envian los datos por RF
lastValueModoGiro = currentModoGiro;
lastValueGiro = currentValueGiro;
}
}
void LeerEnviarValorAceleracion() {
byte datos[12]; // Desde el byte 0 al 11
//Leemos el potenciometro de aceleración del motor
int8_t currentValueAceleracion = ReadAnalogValue(PinAceleracion, 0, 1023, 0, 100,
ValueSteps);
if(currentValueAceleracion != lastValueAceleracion) { //Si el valor es distinto,
envia el nuevo valor y setea el valor actual como el ultimo.
datos[0] = idModulo[0];
datos[1] = idModulo[1];
datos[2] = idModulo[2];


datos[3] = idModulo[3];
datos[4] = idAceleracion;
datos[5] = currentValueAceleracion;
datos[6] = 0;
datos[7] = 0;
datos[8] = 0;
datos[9] = 0;
datos[10] = 0;
datos[11] = 0;
radio.write(datos, sizeof(datos)); //Se envian los datos por RF
lastValueAceleracion = currentValueAceleracion; //Se establece la nueva
medicion como la ultima.
}
}
void LeerEnviarValorLuces() {
byte datos[12]; // Desde el byte 0 al 11
int value = analogRead(PinModoLuces);
int lucesapagadas=11;
int lucesdelanteras=12;
int lucestodas=13;
if (value>=450 && value<=550){
if(lucesapagadas != lastvalueluces){
#if NEXTION ==true
myNextion.setPictureValue("page2.p3",10);
#endif
datos[0] = idModulo[0];
datos[1] = idModulo[1];
datos[2] = idModulo[2];
datos[3] = idModulo[3];
datos[4] = idLuces;
datos[5] = 255;
datos[6] = 255;
datos[7] = 255;
datos[8] = 255;
datos[9] = 0;
datos[10] = 0;
datos[11] = 0;
radio.write(datos, sizeof(datos)); //Se envian los datos por RF
// oled.clearDisplay();
// oled.setTextColor(WHITE);
// oled.setCursor(0, 0);
// oled.setTextSize(1);
// oled.print(" 	LUCES");
// oled.setCursor (10, 30);
// oled.setTextSize(2);
// oled.print("APAGADAS");
// oled.display();
// delay(500);


// oled.clearDisplay();
lastvalueluces = lucesapagadas;
}
}
else if (value>=850 && value<=950){
if(lucestodas != lastvalueluces){
#if NEXTION ==true
myNextion.setPictureValue("page2.p3",16);
#endif
datos[0] = idModulo[0];
datos[1] = idModulo[1];
datos[2] = idModulo[2];
datos[3] = idModulo[3];
datos[4] = idLuces;
datos[5] = 0;
datos[6] = 0;
datos[7] = 0;
datos[8] = 0;
datos[9] = 0;
datos[10] = 0;
datos[11] = 0;
radio.write(datos, sizeof(datos)); //Se envian los datos por RF
// oled.clearDisplay();
// oled.setTextColor(WHITE);
// oled.setCursor(0, 0);
// oled.setTextSize(1);
// oled.print(" 	LUCES");
// oled.setCursor (10, 30);
// oled.setTextSize(2);
// oled.print(" TODAS");
// oled.display();
// delay(500);
// oled.clearDisplay();
}
lastvalueluces=lucestodas;
}
else {
if(lucesdelanteras != lastvalueluces){
#if NEXTION ==true
myNextion.setPictureValue("page2.p3",11);
#endif
datos[0] = idModulo[0];
datos[1] = idModulo[1];
datos[2] = idModulo[2];
datos[3] = idModulo[3];
datos[4] = idLuces;
datos[5] = 255;
datos[6] = 255;
datos[7] = 0;
datos[8] = 0;
datos[9] = 0;
datos[10] = 0;


datos[11] = 0;
radio.write(datos, sizeof(datos)); //Se envian los datos por RF
// oled.clearDisplay();
// oled.setTextColor(WHITE);
// oled.setCursor(0, 0);
// oled.setTextSize(1);
// oled.print(" 	LUCES");
// oled.setCursor (10, 30);
// oled.setTextSize(2);
// oled.print("DELANTERA");
// oled.display();
// delay(500);
// oled.clearDisplay();
}
lastvalueluces=lucesdelanteras;
}
}
void LeerEnviarValorBocina() {
byte datos[12]; // Desde el byte 0 al 11
//Leemos el pulsador
bool sinobocina = ReadDigitalValue(Bocina);
#if DEBUG == true
Serial.println("Bocina :");
Serial.println(sinobocina);
#endif
if(sinobocina != lastValueBocina) { //Si el valor es distinto, envia el nuevo
valor y setea el valor actual como el ultimo.
datos[0] = idModulo[0];
datos[1] = idModulo[1];
datos[2] = idModulo[2];
datos[3] = idModulo[3];
datos[4] = idBocina;
if(sinobocina == true){ //PULSADOR NA
//Si la bocina está pulsada
datos[5] = 255;
#if NEXTION == true
myNextion.setPictureValue("page2.p0",4);
#endif
#if OLED == true
oled.clearDisplay(); 	// limpia pantalla
oled.setTextColor(WHITE); 	// establece color al unico disponible
(pantalla monocromo)
oled.setCursor(0, 0); 	// ubica cursor en inicio de coordenadas 0,0
oled.setTextSize(1); 	// establece tamano de texto en 1
oled.print(" 	ACTIVACION"); // escribe en pantalla el texto
oled.setCursor (10, 30); 	// ubica cursor en coordenas 10,30
oled.setTextSize(2); 	// establece tamano de texto en 2


oled.print(" BOCINA"); // escribe en pantalla el texto
oled.display(); 	// muestra en pantalla todo lo establecido
anteriormente
delay(300);
oled.clearDisplay(); 	// limpia pantalla
#endif
} else {
//Si la bocina NO está pulsada
datos[5] = 0;
#if NEXTION == true
myNextion.setPictureValue("page2.p0",3);
#endif
}
datos[6] = 0;
datos[7] = 0;
datos[8] = 0;
datos[9] = 0;
datos[10] = 0;
datos[11] = 0;
radio.write(datos, sizeof(datos)); //Se envian los datos por RF
lastValueBocina = sinobocina; //Se establece la nueva medicion como la ultima.
}
}
void LeerEnviarValorArranque() {
byte datos[12]; // Desde el byte 0 al 11
//Leemos el pulsador
bool sinoarranque = ReadDigitalValue(Arranque);
Serial.println("Arranque: ");
Serial.println(sinoarranque);
if(sinoarranque != lastValueArranque) { //Si el valor es distinto, envia el
nuevo valor y setea el valor actual como el ultimo.
datos[0] = idModulo[0];
datos[1] = idModulo[1];
datos[2] = idModulo[2];
datos[3] = idModulo[3];
datos[4] = idArranque;
if(sinoarranque==true){
datos[5] = 0;
datos[6] = 0;
datos[7] = 0;
datos[8] = 0;
}
if(sinoarranque==false){
datos[5] = 255;


datos[6] = 0;
datos[7] = 0;
datos[8] = 0;
// oled.clearDisplay(); 	// limpia pantalla
// oled.setTextColor(WHITE); 	// establece color al unico disponible
(pantalla monocromo)
// oled.setCursor(0, 0); 	// ubica cursor en inicio de coordenadas 0,0
// oled.setTextSize(1); 	// establece tamano de texto en 1
// oled.print(" 	ACTIVACION"); // escribe en pantalla el texto
// oled.setCursor (10, 30); 	// ubica cursor en coordenas 10,30
// oled.setTextSize(2); 	// establece tamano de texto en 2
// oled.print(" ARRANQUE"); // escribe en pantalla el texto
// oled.display(); 	// muestra en pantalla todo lo establecido
anteriormente
// delay(1200);
// oled.clearDisplay(); 	// limpia pantalla
}
datos[9] = 0;
datos[10] = 0;
datos[11] = 0;
radio.write(datos, sizeof(datos)); //Se envian los datos por RF
lastValueArranque = sinoarranque; //Se establece la nueva medicion como la
ultima.
}
}
void LeerEnviarValorBombaAux() {
byte datos[12]; // Desde el byte 0 al 11
//Leemos el potenciometro de Bomba Auxiliar
int8_t currentValueBombAux = ReadAnalogValue(PinBombAux, 0, 1023, 0, 100,
ValueSteps);
if(currentValueBombAux != lastValueBombAux) { //Si el valor es distinto, envia
el nuevo valor y setea el valor actual como el ultimo.
datos[0] = idModulo[0];
datos[1] = idModulo[1];
datos[2] = idModulo[2];
datos[3] = idModulo[3];
datos[4] = idBombaAux;
datos[5] = currentValueBombAux;
datos[6] = 0;
datos[7] = 0;
datos[8] = 0;
datos[9] = 0;
datos[10] = 0;
datos[11] = 0;
#if NEXTION == true
String datolecturaString_Niv =String(currentValueBombAux);
myNextion.setComponentText("page0.t10",datolecturaString_Niv);
#endif
radio.write(datos, sizeof(datos)); //Se envian los datos por RF
lastValueBombAux = currentValueBombAux; //Se establece la nueva medicion como
la ultima.


}
}
void LeerEnviarValorNivAuto() {
byte datos[12]; // Desde el byte 0 al 11
//Leemos el pulsador
bool sinoNivAuto = ReadDigitalValue(NivAuto);
#if DEBUG == true
Serial.println("Nivelacion Auto: ");
Serial.println(sinoNivAuto);
#endif
if(sinoNivAuto != lastValueNivAuto) { //Si el valor es distinto, envia el nuevo
valor y setea el valor actual como el ultimo.
datos[0] = idModulo[0];
datos[1] = idModulo[1];
datos[2] = idModulo[2];
datos[3] = idModulo[3];
datos[4] = idNivAuto;
if(sinoNivAuto==true){
datos[5] = 0;
#if NEXTION == true
myNextion.setComponentText("page2.t1","MANUAL");
#endif
#if OLED == true
oled.clearDisplay();
oled.setTextColor(WHITE);
oled.setCursor(0, 0);
oled.setTextSize(1);
oled.print(" 	NIVELACION");
oled.setCursor (5, 30);
oled.setTextSize(2);
oled.print("AUTOMATICA");
oled.display();
delay(500);
oled.clearDisplay();
#endif
} else {
datos[5] = 255;
#if NEXTION == true
myNextion.setComponentText("page2.t1","AUTO");
#endif
#if OLED == true


oled.clearDisplay();
oled.setTextColor(WHITE);
oled.setCursor(0, 0);
oled.setTextSize(1);
oled.print(" 	NIVELACION");
oled.setCursor (5, 30);
oled.setTextSize(2);
oled.print("MANUAL");
oled.display();
delay(500);
oled.clearDisplay();
#endif
}
datos[6] = 0;
datos[7] = 0;
datos[8] = 0;
datos[9] = 0;
datos[10] = 0;
datos[11] = 0;
radio.write(datos, sizeof(datos)); //Se envian los datos por RF
lastValueNivAuto = sinoNivAuto; //Se establece la nueva medicion como la
ultima.
}
}
void LeerEnviarValorSetaEmergencia() {
byte datos[12]; // Desde el byte 0 al 11
//Leemos el interruptor
bool sinosetaemergencia = ReadDigitalValue(SetaEmergencia);
#if DEBUG == true
Serial.println("Seta de emergencia: ");
Serial.println(sinosetaemergencia);
#endif
if(sinosetaemergencia != lastValueSetaEmergencia) { //Si el valor es distinto,
envia el nuevo valor y setea el valor actual como el ultimo.
datos[0] = 255;
datos[1] = 255;
datos[2] = 255;
datos[3] = 255;
if(sinosetaemergencia == false) {
datos[4] = 0;
#if NEXTION == true
myNextion.sendCommand("page 2");
//Al desactivar la SETA DE EMERGENCIA volvemos a las variables mas usadas
myNextion.setPictureValue("page2.p0",3);
myNextion.setPictureValue("page2.p3",16);


myNextion.setComponentText("page2.t2","0");
myNextion.setComponentText("page2.t1","MANUAL");
myNextion.setPictureValue("page2.p1",6);
#endif
} else {
datos[4] = 255;
#if NEXTION == true
myNextion.sendCommand("page 3");
#endif
#if OLED == true
oled.clearDisplay();
oled.setTextColor(WHITE);
oled.setCursor(0, 0);
oled.setTextSize(1);
oled.print("---------------------");
oled.setCursor (10, 25);
oled.setTextSize(2);
oled.print(" WARNING");
oled.display();
#endif
}
datos[5] = 0;
datos[6] = 0;
datos[7] = 0;
datos[8] = 0;
datos[9] = 0;
datos[10] = 0;
datos[11] = 0;
radio.write(datos, sizeof(datos)); //Se envian los datos por RF
lastValueSetaEmergencia = sinosetaemergencia; //Se establece la nueva medicion
como la ultima.
}
}
void LeerEnviarValorRF() {
byte datos[12]; // Desde el byte 0 al 11
//Leemos el interruptor
bool sinoRF = ReadDigitalValue(InterruptorRF);
#if DEBUG == true
Serial.println("RF :");
Serial.println(sinoRF);
#endif
if(sinoRF != lastValueRF) { //Si el valor es distinto, envia el nuevo valor y
setea el valor actual como el ultimo.
datos[0] = 255;
datos[1] = 0;


datos[2] = 0;
datos[3] = 2;
if(sinoRF==true) {datos[4] = 0;
#if NEXTION == true
myNextion.setPictureValue("page2.p1",6);
#endif
}
else {datos[4] = 255;
#if NEXTION == true
myNextion.setPictureValue("page2.p1",7);
#endif
}
datos[5] = 0;
datos[6] = 0;
datos[7] = 0;
datos[8] = 0;
datos[9] = 0;
datos[10] = 0;
datos[11] = 0;
radio.write(datos, sizeof(datos)); //Se envian los datos por RF
lastValueRF = sinoRF; //Se establece la nueva medicion como la ultima.
#if OLED == true
oled.clearDisplay();
oled.setTextColor(WHITE);
oled.setCursor(4,4);
oled.setTextSize(4);
oled.print(" UCO ");
oled.display();
#endif
}
}
void LeerEnviarValorAutonomo() {
byte datos[12]; // Desde el byte 0 al 11
//Leemos el pulsador
bool sinoAutonomo = ReadDigitalValue(Autonomo);
#if DEBUG == true
Serial.println("Autonomo: ");
Serial.println(sinoAutonomo);
#endif
if(sinoAutonomo != lastValueAutonomo) { //Si el valor es distinto, envia el
nuevo valor y setea el valor actual como el ultimo.
datos[0] = idModulo[0];
datos[1] = idModulo[1];
datos[2] = idModulo[2];
datos[3] = idModulo[3];
datos[4] = idAutonomo;


if(sinoAutonomo == true){
datos[5] = 255;
#if NEXTION == true
myNextion.setPictureValue("page2.p2",8);
#endif
#if OLED == true
oled.clearDisplay();
oled.setTextColor(WHITE);
oled.setCursor(0, 0);
oled.setTextSize(1);
oled.print(" 	MODO");
oled.setCursor (5, 30);
oled.setTextSize(2);
oled.print("AUTONOMO ");
oled.display();
#endif
} else {
datos[5] = 0;
#if NEXTION == true
myNextion.setPictureValue("page2.p2",12);
#endif
#if OLED == true
oled.clearDisplay();
oled.setTextColor(WHITE);
oled.setCursor(0, 0);
oled.setTextSize(1);
oled.print(" 	MODO");
oled.setCursor (5, 30);
oled.setTextSize(2);
oled.print(" MANUAL ");
oled.display();
delay(500);
oled.clearDisplay();
#endif
}
datos[6] = 0;
datos[7] = 0;
datos[8] = 0;
datos[9] = 0;
datos[10] = 0;
datos[11] = 0;
radio.write(datos, sizeof(datos)); //Se envian los datos por RF
lastValueAutonomo = sinoAutonomo; //Se establece la nueva medicion como la
ultima.
}
}


int8_t ReadAnalogValue(uint8_t port, int portmin, int portmax, int8_t outmin,
int8_t outmax, uint8_t steps) {
int valor = analogRead(port); //Leemos el puerto analogico
int8_t conv = map(valor, portmin, portmax, outmin, outmax); //Convertimos el
valor leido al rango que necesitamos
if (conv < ValueZero && conv>-ValueZero){ 	//Si el valor se encuentra entre
+/- ValueZero, el valor leido del puerto analógico se considera cero.
conv = 0;
}
conv = CalcularValorPasos(conv, steps); //Convertimos el valor leido al salto
más proximo según la configuración.
return conv;// En este tipo de funciones si retornamos
}
bool ReadDigitalValue(uint8_t port) {
int valor = digitalRead(port); //Leemos el puerto digital
if (valor>0) return true;
else return false;
}
int8_t CalcularValorPasos(int8_t value, uint8_t steps) {
int8_t value1 = round(value / steps);
return (value1 * steps);
}
void EnviarEstoyVivo() {
if ((millis()-lastTimeEstoyVivo) >= EstoyVivoPeriod) { 	//Comprobamos el tiempo
para determinar si es necesario enviar la señal de ESTOY VIVO
byte datos[12] = {255, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0};
radio.write(datos, sizeof(datos));
lastTimeEstoyVivo = millis();
}
}
