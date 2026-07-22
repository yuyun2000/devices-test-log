#include <Arduino.h>
#include <M5Unified.h>
#include <UNIT_UHF_RFID.h>

namespace {

constexpr uint8_t kEpcBank  = 0x01;
constexpr size_t kChunkSize = 4;

Unit_UHF_RFID uhf;

struct UartPins {
    const char* port;
    uint8_t rx;
    uint8_t tx;
};

constexpr UartPins kCandidates[] = {
    {"C", 13, 14},
    {"A", 33, 32},
    {"B", 36, 26},
};

UartPins active_pins = kCandidates[0];

void printHex(const uint8_t* data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        Serial.printf("%02X", data[i]);
    }
}

bool verifyEpcChunk(const CARD& card, uint16_t start_address, size_t epc_offset) {
    uint8_t actual[kChunkSize] = {};
    const bool read_ok = uhf.readCard(actual, sizeof(actual), kEpcBank, start_address);
    const bool match = read_ok && memcmp(actual, card.epc + epc_offset, sizeof(actual)) == 0;

    Serial.printf("UHF_ISSUE16_READ sa=%u read_ok=%u match=%u expected=", start_address, read_ok, match);
    printHex(card.epc + epc_offset, sizeof(actual));
    Serial.print(" actual=");
    printHex(actual, sizeof(actual));
    Serial.println();
    return match;
}

bool findUnit() {
    for (const auto& pins : kCandidates) {
        Serial2.end();
        delay(100);
        uhf.begin(&Serial2, 115200, pins.rx, pins.tx, false);
        delay(300);

        const String version = uhf.getVersion();
        Serial.printf("UHF_ISSUE16_PROBE port=%s rx=%u tx=%u version=%s\n", pins.port, pins.rx, pins.tx,
                      version.c_str());
        if (version != "ERROR") {
            active_pins = pins;
            uhf.begin(&Serial2, 115200, pins.rx, pins.tx, true);
            delay(200);
            return true;
        }
    }
    return false;
}

bool verifyNoTagTransport() {
    uint8_t data[kChunkSize] = {};
    constexpr uint16_t kDiagnosticAddress = 0x1234;
    const bool read_ok = uhf.readCard(data, sizeof(data), kEpcBank, kDiagnosticAddress);
    const bool module_error = !read_ok && uhf.buffer[0] == 0xBB && uhf.buffer[2] == 0xFF && uhf.buffer[5] == 0x09 &&
                              uhf.buffer[7] == 0x7E;
    Serial.printf("UHF_ISSUE16_NO_TAG_READ sa=0x%04X read_ok=%u module_error_09=%u\n", kDiagnosticAddress, read_ok,
                  module_error);
    return module_error;
}

bool runTest() {
    const uint8_t count = uhf.pollingOnce();
    Serial.printf("UHF_ISSUE16_POLL count=%u\n", count);
    if (count == 0) {
        if (verifyNoTagTransport()) {
            Serial.println("UHF_ISSUE16_TRANSPORT_PASS tag_present=0");
            return true;
        }
        return false;
    }

    Serial.print("UHF_ISSUE16_EPC value=");
    printHex(uhf.cards[0].epc, sizeof(uhf.cards[0].epc));
    Serial.println();

    if (!uhf.select(uhf.cards[0].epc)) {
        Serial.println("UHF_ISSUE16_SELECT_FAIL");
        return false;
    }

    bool pass = true;
    pass &= verifyEpcChunk(uhf.cards[0], 2, 0);
    pass &= verifyEpcChunk(uhf.cards[0], 4, 4);
    pass &= verifyEpcChunk(uhf.cards[0], 6, 8);
    if (pass) {
        Serial.println("UHF_ISSUE16_DATA_PASS");
        Serial.println("UHF_ISSUE16_TRANSPORT_PASS tag_present=1");
    }
    return pass;
}

}  // namespace

void setup() {
    auto cfg = M5.config();
    cfg.output_power = true;
    M5.begin(cfg);

    Serial.begin(115200);
    delay(1000);
    M5.Power.setExtOutput(true);
    Serial.printf("UHF_ISSUE16_BOOT board=%u\n", static_cast<unsigned>(M5.getBoard()));

    if (!findUnit()) {
        Serial.println("UHF_ISSUE16_UNIT_NOT_FOUND");
        while (true) {
            delay(1000);
        }
    }
    Serial.printf("UHF_ISSUE16_UNIT_READY port=%s rx=%u tx=%u\n", active_pins.port, active_pins.rx, active_pins.tx);
}

void loop() {
    if (runTest()) {
        Serial.println("UHF_ISSUE16_PASS");
        while (true) {
            delay(1000);
        }
    }

    Serial.println("UHF_ISSUE16_RETRY");
    delay(1000);
}
