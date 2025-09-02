#include "ram_reader.h"

RamReader::RamReader(int8_t pinCS, SPIClass* spi)
    : _cs(pinCS), _spi(spi), _ramSize(1024), _initialized(false) {}

void RamReader::begin() {
    pinMode(_cs, OUTPUT);
    digitalWrite(_cs, HIGH);

    _spi->begin(); // pines por defecto

    _initialized = true;
}

bool RamReader::isReady() {
    return _initialized;
}

void RamReader::sendAddress(uint32_t addr) {
    // Envía _addrBytes más significativos primero (big-endian)
    for (int8_t i = _addrBytes - 1; i >= 0; --i) {
        _spi->transfer((addr >> (8 * i)) & 0xFF);
    }
}

uint8_t RamReader::readByte(uint32_t addr) {
    if (!_initialized) return 0;

    // Si limitaste tamaño, evita overflow
    if (_ramSize && addr >= _ramSize) return 0;

    uint8_t data = 0;

    _spi->beginTransaction(SPISettings(SPI_CLOCK_HZ, MSBFIRST, SPI_MODE));
    digitalWrite(_cs, LOW);

    _spi->transfer(CMD_READ);
    sendAddress(addr);
    // Si usaras FAST_READ (0x0B) necesitarías: _spi->transfer(0x00); // dummy

    data = _spi->transfer(0x00); // reloj para recibir un byte

    digitalWrite(_cs, HIGH);
    _spi->endTransaction();

    return data;
}

void RamReader::readBlock(uint32_t addr, uint8_t* buffer, size_t len) {
    if (!_initialized || buffer == nullptr || len == 0) return;

    // Seguridad por tamaño conocido
    if (_ramSize) {
        if (addr >= _ramSize) return;
        uint32_t maxlen = _ramSize - addr;
        if (len > maxlen) len = maxlen;
    }

    _spi->beginTransaction(SPISettings(SPI_CLOCK_HZ, MSBFIRST, SPI_MODE));
    digitalWrite(_cs, LOW);

    _spi->transfer(CMD_READ);
    sendAddress(addr);
    // Para FAST_READ (0x0B): _spi->transfer(0x00); // 1 dummy byte

    // Lectura en ráfaga
#if defined(ARDUINO_ARCH_ESP32)
    // En ESP32 hay transferBytes(). Usamos un buffer de ceros para clockear.
    static uint8_t zeros[64] = {0};
    size_t remaining = len;
    while (remaining) {
        size_t n = (remaining > sizeof(zeros)) ? sizeof(zeros) : remaining;
        _spi->transferBytes(zeros, buffer, n);
        buffer     += n;
        remaining  -= n;
    }
#else
    // Compatible con cualquier core Arduino
    for (size_t i = 0; i < len; ++i) {
        buffer[i] = _spi->transfer(0x00);
    }
#endif

    digitalWrite(_cs, HIGH);
    _spi->endTransaction();
}

void RamReader::end() {
    _spi->end();
    _initialized = false;
}




