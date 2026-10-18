#include <Arduino.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <printf.h>
#include <RF24.h>
#include <RF24_config.h>
#include <mcp2515.h>
//CONFIGURACION
#define DEBUG true
const uint8_t CE_PIN = 9; 	//Declaremos los pines CE y el CSN
const uint8_t CSN_PIN = 10;
const uint8_t PinCS_MCP2515 = 8;
const uint8_t PinEnable = 6;
byte RFPipe[5] ={0x60, 0x59, 0x70, 0x54, 0x01}; //Variable con la dirección del
canal por donde se va a transmitir
const int TimeOutEstoyVivo = 1000;
const int periodNoErrors = 250;
const CAN_SPEED canspeed = CAN_500KBPS;
//-------------------------------
//DEFINICIONES
RF24 radio(CE_PIN, CSN_PIN); //creamos el objeto radio (NRF24L01)
MCP2515 mcp2515(PinCS_MCP2515);
unsigned long LastTimeEstoyVivo = 0;
bool errorEstoyVivoTimeLimit = false;
bool errorSetaEmergencia = false;
bool errorControlDesactivado = false;
unsigned long LastTimeNoErrors = 0;
void SendCAN(byte * data);
void EnviarErrores();
void EnviarErrorEstoyVivo();
void EnviarErrorControlDesactivado();
void EnviarErrorSetaEmergencia();
void EnviarNoErrores();
bool CheckTimeEstoyVivo();
bool CheckEnable();
void LeerDatosRF();
void LeerDatosCAN();
void SplitULong(unsigned long value, uint8_t *b0, uint8_t *b1, uint8_t *b2, uint8_t
*b3);
void EnviarIdPipe();


//-----------------------------------
void setup() {
#if DEBUG == true
Serial.begin(115200);
#endif
//Inicializamos el módulo de RF
radio.begin();
radio.openReadingPipe(1, RFPipe);
radio.startListening();
//Inicializamos el modulo CAN
mcp2515.reset();
mcp2515.setBitrate(canspeed);
mcp2515.setNormalMode();
pinMode(PinEnable, INPUT);
LastTimeEstoyVivo = millis();
}
void loop() {
//Se leen y procesan los datos que existan disponibles por el canal de RF
LeerDatosRF();
//Leemos los datos disponibles en CAN y los procesamos
LeerDatosCAN();
//Se comprueba el tiempo desde la ultima señal EstoyVivo y en caso necesario envia
un error.
if (!CheckTimeEstoyVivo()) errorEstoyVivoTimeLimit = true;
else errorEstoyVivoTimeLimit = false;
//Se envian los errores si los hay
EnviarErrores();
}
void LeerDatosRF() {
uint8_t datos[12];
if (radio.available() ) { 	//Comprobamos si hay datos que leer
radio.read(datos,sizeof(datos)); //Leemos los datos disponibles
if (datos[0] != 255) { 	//Si el primer dato es distinto a 255 se trata
de una trama CAN y debe ser enviada a traves del módulo CAN.
//Solo enviamos ordenes si el modulo está habilitado


if (CheckEnable()) SendCAN(datos);
}
else {
//Si el primer datos es 255 se trata de unos datos internos y deben ser
procesados según corresponda.
if (datos[0] == 255 && datos [1] == 0 && datos[2] == 0 && datos[3] == 1) {
//Es la señal de Estoy Vivo
LastTimeEstoyVivo = millis(); //Establecemos la hora actual como la ultima
a la que se recibio la señal
}
else if (datos[0] == 255 && datos [1] == 0 && datos[2] == 0 && datos[3] == 2)
{
//Activacion / Desactivacion control inalambrico
if (datos[4] == 255) errorControlDesactivado = true;
else errorControlDesactivado = false;
}
else if (datos[0] == 255 && datos [1] == 255 && datos[2] == 255 && datos[3] ==
255) {
//Seta de emergencia
if (datos[4] == 255) errorSetaEmergencia = true;
else errorSetaEmergencia = false;
}
}
}
}
void LeerDatosCAN() {
struct can_frame canMsg;
//Comprobamos si existen mensajes que leer
if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
//Comprobamos si es una solicitud de alguna trama
if(canMsg.can_id == 201) {
byte array_id[4];
array_id[0] = canMsg.data[0];
array_id[1] = canMsg.data[1];
array_id[2] = canMsg.data[2];
array_id[3] = canMsg.data[3];
unsigned long id_solicitud =* (unsigned long*)(&array_id);
switch (id_solicitud)
{
case 221: //Es una solicitud para enviar el idPipe del receptor
EnviarIdPipe();
break;
default:


break;
}
}
}
}
void EnviarErrores() {
//Si hay error en la seta de emergencia (está presionada) se envia por CAN
if (errorSetaEmergencia) {
EnviarErrorSetaEmergencia();
}
//Si hay error en la activación del control inalambrico (está desactivado) se
envia por CAN
if (errorControlDesactivado) {
EnviarErrorControlDesactivado();
}
//Si se ha superado el tiempo limite de espera para recibir una señal de Estoy
Vivo, se envia por CAN
if (errorEstoyVivoTimeLimit) {
EnviarErrorEstoyVivo();
}
//Si no hay errores se envia por CAN para indicar que el modulo receptor funciona
correctamente
if (!errorControlDesactivado && !errorEstoyVivoTimeLimit && !errorSetaEmergencia)
{
//Comprobamos el tiempo desde el ultimo envio. Solo se envia cada cierto periodo
para evitar saturar el canal.
if ((millis() - LastTimeNoErrors) >= periodNoErrors) {
EnviarNoErrores();
LastTimeNoErrors = millis();
}
}
}
void SendCAN(byte * data) {
struct can_frame canMsg;
byte array_id[4];
array_id[0] = data[3];
array_id[1] = data[2];
array_id[2] = data[1];
array_id[3] = data[0];


canMsg.can_id =* (unsigned long*)(&array_id);
#if DEBUG == true
Serial.print(" 	");
Serial.print(data[0]);
Serial.print(" 	");
Serial.print(data[1]);
Serial.print(" 	");
Serial.print(data[2]);
Serial.print(" 	");
Serial.print(data[3]);
Serial.print(" 	");
Serial.print(data[4]);
Serial.print(" 	");
Serial.print(data[5]);
Serial.print(" 	");
Serial.print(data[6]);
Serial.print(" 	");
Serial.print(data[7]);
Serial.print(" 	");
Serial.print(data[8]);
Serial.print(" 	");
Serial.print(data[9]);
Serial.print(" 	");
Serial.print(data[10]);
Serial.print(" 	");
Serial.print(data[11]);
Serial.println(" 	");
#endif
canMsg.can_dlc = 8;
canMsg.data[0] = data[4];
canMsg.data[1] = data[5];
canMsg.data[2] = data[6];
canMsg.data[3] = data[7];
canMsg.data[4] = data[8];
canMsg.data[5] = data[9];
canMsg.data[6] = data[10];
canMsg.data[7] = data[11];
mcp2515.sendMessage(&canMsg);
}
bool CheckTimeEstoyVivo() {
if ((millis() - LastTimeEstoyVivo)<= TimeOutEstoyVivo) {
return true;
}
else {
return false;
}
}
bool CheckEnable() {
if (digitalRead(PinEnable)) return true;


else return false;
}
void EnviarIdPipe() {
//Trama CAN
struct can_frame canMsg;
canMsg.can_id = 221;
canMsg.can_dlc = 8;
canMsg.data[0] = RFPipe[0];
canMsg.data[1] = RFPipe[1];
canMsg.data[2] = RFPipe[2];
canMsg.data[3] = RFPipe[3];
canMsg.data[4] = RFPipe[4];
canMsg.data[5] = 0;
canMsg.data[6] = 0;
canMsg.data[7] = 0;
mcp2515.sendMessage(&canMsg);
}
void EnviarNoErrores() {
//Trama CAN
struct can_frame canMsg;
canMsg.can_id = 11;
canMsg.can_dlc = 8;
canMsg.data[0] = 0;
canMsg.data[1] = 0;
canMsg.data[2] = 0;
canMsg.data[3] = 0;
canMsg.data[4] = 0;
canMsg.data[5] = 0;
canMsg.data[6] = 0;
canMsg.data[7] = 0;
mcp2515.sendMessage(&canMsg);
}
void EnviarErrorSetaEmergencia() {
//Trama CAN
struct can_frame canMsg;
canMsg.can_id = 11;
canMsg.can_dlc = 8;
canMsg.data[0] = 255;
canMsg.data[1] = 0;
canMsg.data[2] = 0;
canMsg.data[3] = 0;
canMsg.data[4] = 0;
canMsg.data[5] = 0;
canMsg.data[6] = 0;
canMsg.data[7] = 0;
mcp2515.sendMessage(&canMsg);


}
void EnviarErrorControlDesactivado() {
//Trama CAN
struct can_frame canMsg;
canMsg.can_id = 11;
canMsg.can_dlc = 8;
canMsg.data[0] = 2;
canMsg.data[1] = 255;
canMsg.data[2] = 0;
canMsg.data[3] = 0;
canMsg.data[4] = 0;
canMsg.data[5] = 0;
canMsg.data[6] = 0;
canMsg.data[7] = 0;
mcp2515.sendMessage(&canMsg);
}
void EnviarErrorEstoyVivo() {
//Bytes correspondientes al tiempo limite de espera.
uint8_t b0,b1,b2,b3;
SplitULong(TimeOutEstoyVivo, &b0, &b1, &b2, &b3);
//Trama CAN
struct can_frame canMsg;
canMsg.can_id = 11;
canMsg.can_dlc = 8;
canMsg.data[0] = 1;
canMsg.data[1] = b0;
canMsg.data[2] = b1;
canMsg.data[3] = b2;
canMsg.data[4] = b3;
canMsg.data[5] = 0;
canMsg.data[6] = 0;
canMsg.data[7] = 0;
mcp2515.sendMessage(&canMsg);
}
void SplitULong(unsigned long value, uint8_t *b0, uint8_t *b1, uint8_t *b2, uint8_t
*b3) {
uint8_t len = sizeof(unsigned long);
uint8_t data[len];
memcpy(data, &value, len);
*b0 = data[0];
*b1 = data[1];
*b2 = data[2];
*b3 = data[3];
}
