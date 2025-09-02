#include "ram_reader/ram_reader.cpp"
#include "uart_comm/uart_comm.cpp"
#include "display/display.cpp"
#include "peripherals/peripherals.cpp"
#include "pds/pds.cpp"
#include "net/net.cpp" 

RamReader ram(5); // CS en GPIO 5
UartComm uartComm;
Display display;
Peripherals peripherals;
Pds pds;
Net net;

void setup() {
  Serial.begin(9600);
  ram.begin();
}

void loop() {
  if (ram.isReady()) {
    uint8_t buf[16];
    ram.readBlock(0x0000, buf, sizeof(buf));
    Serial.print("Bytes: ");
    for (uint8_t b : buf) {
      if (b < 16) Serial.print('0');
      Serial.print(b, HEX);
      Serial.print(' ');
    }
    Serial.println();
    delay(1000);
  }
}
