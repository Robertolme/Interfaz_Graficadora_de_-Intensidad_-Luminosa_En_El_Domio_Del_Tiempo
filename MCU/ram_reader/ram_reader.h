#ifndef RAM_READER_H
#define RAM_READER_H

#include <Arduino.h>
#include <SPI.h>

class RamReader {
public:
    RamReader(int8_t pinCS, SPIClass* spi = &SPI);
    void begin();
    bool isReady();
    uint8_t readByte(uint32_t addr);
    void readBlock(uint32_t addr, uint8_t* buffer, size_t len);
    void end();

private:
    void sendAddress(uint32_t addr);
    int8_t _cs;
    SPIClass* _spi;
    uint32_t _ramSize;
    bool _initialized;
    uint8_t  _addrBytes = 3; // 24 bits de dirección
    static constexpr uint8_t  CMD_READ = 0x03;
    static constexpr uint32_t SPI_CLOCK_HZ = 10'000'000; // 10 MHz
    static constexpr uint8_t  SPI_MODE = SPI_MODE0; // CPOL=0,
};

#endif // RAM_READER_H
