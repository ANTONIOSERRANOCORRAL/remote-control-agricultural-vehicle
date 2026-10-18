#include <SPI.h>
#include <mcp2515.h>
struct can_frame canMsg;// Parece el identificador pero NOSE BIEN
MCP2515 mcp2515(10);
void setup() {
Serial.begin(115200);
mcp2515.reset();
mcp2515.setBitrate(CAN_500KBPS);
//mcp2515.setBitrate (CAN_250KBPS, MCP_16MHZ);
mcp2515.setNormalMode();
}
void loop() {
if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
Serial.print(canMsg.can_id, DEC); // print ID
Serial.print(" ");
// 	Serial.print(canMsg.can_dlc, DEC); // print DLC
// 	Serial.print(" ");
for (int i = 0; i<canMsg.can_dlc; i++) { // print the data
Serial.print(canMsg.data[i],DEC);
Serial.print(" ");
}
Serial.print(" 	");
}
else{
Serial.print(" 	No llega trama CAN 	");
}
delay(100);
}
