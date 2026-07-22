#include <Arduino.h>
#include <M5Unified.h>
#include <Preferences.h>
#include <esp_intr_alloc.h>
#include <usb/usb_host.h>

namespace {

enum class HostState : uint8_t {
    kUpstreamConnected,
    kReady,
    kStarting,
    kWaitingForDevice,
    kEnumerated,
    kFailed,
};

struct HostResult {
    uint16_t vid = 0;
    uint16_t pid = 0;
    uint8_t device_class = 0;
    uint8_t interfaces = 0;
    usb_speed_t speed = USB_SPEED_FULL;
};

struct DriverState {
    usb_host_client_handle_t client = nullptr;
    usb_device_handle_t device = nullptr;
    uint8_t address = 0;
    volatile uint32_t actions = 0;
};

constexpr uint32_t kActionOpen = 1u << 0;
constexpr uint32_t kActionRead = 1u << 1;
constexpr uint32_t kActionClose = 1u << 2;

SemaphoreHandle_t host_ready;
Preferences prefs;
portMUX_TYPE result_mux = portMUX_INITIALIZER_UNLOCKED;
volatile HostState host_state = HostState::kUpstreamConnected;
HostResult host_result;
char failure_reason[48] = "none";
bool host_started = false;
bool last_touch = false;

bool upstreamConnected(int16_t vbus_mv) {
    return vbus_mv > 3000 || static_cast<bool>(Serial);
}

void setFailure(const char* reason, esp_err_t error) {
    portENTER_CRITICAL(&result_mux);
    snprintf(failure_reason, sizeof(failure_reason), "%s:%s", reason,
             esp_err_to_name(error));
    host_state = HostState::kFailed;
    portEXIT_CRITICAL(&result_mux);
}

void saveResult(const HostResult& result) {
    prefs.putUShort("vid", result.vid);
    prefs.putUShort("pid", result.pid);
    prefs.putUChar("class", result.device_class);
    prefs.putUChar("ifaces", result.interfaces);
    prefs.putUChar("speed", static_cast<uint8_t>(result.speed));
    prefs.putBool("valid", true);
}

void clientEvent(const usb_host_client_event_msg_t* event, void* arg) {
    auto* driver = static_cast<DriverState*>(arg);
    if (event->event == USB_HOST_CLIENT_EVENT_NEW_DEV && driver->address == 0) {
        driver->address = event->new_dev.address;
        driver->actions |= kActionOpen;
    } else if (event->event == USB_HOST_CLIENT_EVENT_DEV_GONE && driver->device != nullptr) {
        driver->actions = kActionClose;
    }
}

void hostDaemonTask(void*) {
    const usb_host_config_t config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };
    const esp_err_t error = usb_host_install(&config);
    if (error != ESP_OK) {
        setFailure("usb_host_install", error);
        xSemaphoreGive(host_ready);
        vTaskDelete(nullptr);
    }

    host_state = HostState::kWaitingForDevice;
    xSemaphoreGive(host_ready);
    while (true) {
        uint32_t event_flags = 0;
        const esp_err_t event_error = usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
        if (event_error != ESP_OK) {
            setFailure("lib_events", event_error);
            vTaskDelete(nullptr);
        }
    }
}

void hostClientTask(void*) {
    xSemaphoreTake(host_ready, portMAX_DELAY);
    if (host_state == HostState::kFailed) {
        vTaskDelete(nullptr);
    }

    DriverState driver;
    const usb_host_client_config_t client_config = {
        .is_synchronous = false,
        .max_num_event_msg = 5,
        .async = {
            .client_event_callback = clientEvent,
            .callback_arg = &driver,
        },
    };
    esp_err_t error = usb_host_client_register(&client_config, &driver.client);
    if (error != ESP_OK) {
        setFailure("client_register", error);
        vTaskDelete(nullptr);
    }

    while (true) {
        if (driver.actions == 0) {
            error = usb_host_client_handle_events(driver.client, pdMS_TO_TICKS(100));
            if (error != ESP_OK && error != ESP_ERR_TIMEOUT) {
                setFailure("client_events", error);
                vTaskDelete(nullptr);
            }
            continue;
        }

        if (driver.actions & kActionOpen) {
            error = usb_host_device_open(driver.client, driver.address, &driver.device);
            driver.actions &= ~kActionOpen;
            if (error != ESP_OK) {
                setFailure("device_open", error);
                vTaskDelete(nullptr);
            }
            driver.actions |= kActionRead;
        }

        if (driver.actions & kActionRead) {
            usb_device_info_t info = {};
            const usb_device_desc_t* device_desc = nullptr;
            const usb_config_desc_t* config_desc = nullptr;
            error = usb_host_device_info(driver.device, &info);
            if (error == ESP_OK) {
                error = usb_host_get_device_descriptor(driver.device, &device_desc);
            }
            if (error == ESP_OK) {
                error = usb_host_get_active_config_descriptor(driver.device, &config_desc);
            }
            if (error != ESP_OK || device_desc == nullptr || config_desc == nullptr) {
                setFailure("read_descriptor", error);
                vTaskDelete(nullptr);
            }

            HostResult result;
            result.vid = device_desc->idVendor;
            result.pid = device_desc->idProduct;
            result.device_class = device_desc->bDeviceClass;
            result.interfaces = config_desc->bNumInterfaces;
            result.speed = info.speed;
            saveResult(result);

            portENTER_CRITICAL(&result_mux);
            host_result = result;
            host_state = HostState::kEnumerated;
            portEXIT_CRITICAL(&result_mux);

            driver.actions &= ~kActionRead;
        }

        if (driver.actions & kActionClose) {
            usb_host_device_close(driver.client, driver.device);
            driver.device = nullptr;
            driver.address = 0;
            driver.actions &= ~kActionClose;
        }
    }
}

void drawStatus() {
    HostState state;
    HostResult result;
    char reason[sizeof(failure_reason)];
    portENTER_CRITICAL(&result_mux);
    state = host_state;
    result = host_result;
    memcpy(reason, failure_reason, sizeof(reason));
    portEXIT_CRITICAL(&result_mux);

    M5.Display.fillScreen(BLACK);
    M5.Display.setTextColor(WHITE);
    M5.Display.setTextSize(2);
    M5.Display.setCursor(8, 10);
    M5.Display.println("CoreS3 USB Host");
    M5.Display.println();

    switch (state) {
        case HostState::kUpstreamConnected:
            M5.Display.println("Upstream USB detected");
            M5.Display.println("Host power is OFF");
            M5.Display.println();
            M5.Display.println("Unplug test cable,");
            M5.Display.println("attach OTG device,");
            M5.Display.println("then tap screen.");
            break;
        case HostState::kReady:
            M5.Display.println("Safe to start host");
            M5.Display.println("Tap screen");
            break;
        case HostState::kStarting:
            M5.Display.println("Starting host...");
            break;
        case HostState::kWaitingForDevice:
            M5.Display.println("VBUS ON");
            M5.Display.println("Waiting for USB device");
            break;
        case HostState::kEnumerated:
            M5.Display.setTextColor(GREEN);
            M5.Display.println("ENUMERATION PASS");
            M5.Display.printf("VID %04X  PID %04X\n", result.vid, result.pid);
            M5.Display.printf("Class %02X  IF %u\n", result.device_class,
                              result.interfaces);
            M5.Display.printf("Speed %s\n",
                              result.speed == USB_SPEED_LOW ? "low" : "full");
            break;
        case HostState::kFailed:
            M5.Display.setTextColor(RED);
            M5.Display.println("HOST FAIL");
            M5.Display.println(reason);
            break;
    }
}

void startHost() {
    prefs.putBool("valid", false);
    host_state = HostState::kStarting;
    drawStatus();

    M5.Power.setUsbOutput(true);
    delay(150);
    if (!M5.Power.getUsbOutput()) {
        setFailure("usb_power", ESP_FAIL);
        return;
    }

    host_ready = xSemaphoreCreateBinary();
    if (host_ready == nullptr) {
        setFailure("semaphore", ESP_ERR_NO_MEM);
        return;
    }

    host_started = true;
    xTaskCreatePinnedToCore(hostDaemonTask, "usb-host-daemon", 4096, nullptr, 2,
                            nullptr, 0);
    xTaskCreatePinnedToCore(hostClientTask, "usb-host-client", 6144, nullptr, 3,
                            nullptr, 1);
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(2500);
    Serial.println("ISSUE71_BOOT");
    auto config = M5.config();
    M5.begin(config);
    prefs.begin("issue71", false);
    M5.Power.setUsbOutput(false);
    delay(50);
    Serial.println("ISSUE71_BOARD_READY");

    if (prefs.getBool("valid", false)) {
        Serial.printf(
            "ISSUE71_REPLAY_PASS vid=0x%04X pid=0x%04X class=0x%02X interfaces=%u speed=%u\n",
            prefs.getUShort("vid"), prefs.getUShort("pid"),
            prefs.getUChar("class"), prefs.getUChar("ifaces"),
            prefs.getUChar("speed"));
    }

    const int16_t vbus_mv = M5.Power.getVBUSVoltage();
    const bool upstream_cdc = static_cast<bool>(Serial);
    if (upstreamConnected(vbus_mv)) {
        host_state = HostState::kUpstreamConnected;
        Serial.printf(
            "ISSUE71_SAFE_WAIT upstream_vbus_mv=%d upstream_cdc=%d usb_output=%d\n",
            vbus_mv, upstream_cdc, M5.Power.getUsbOutput());
    } else {
        host_state = HostState::kReady;
        Serial.printf("ISSUE71_READY upstream_vbus_mv=%d upstream_cdc=%d\n",
                      vbus_mv, upstream_cdc);
    }
    drawStatus();
}

void loop() {
    M5.update();
    if (!host_started && host_state != HostState::kFailed) {
        const int16_t vbus_mv = M5.Power.getVBUSVoltage();
        const bool upstream_connected = upstreamConnected(vbus_mv);
        if (!upstream_connected && host_state == HostState::kUpstreamConnected) {
            host_state = HostState::kReady;
            drawStatus();
        } else if (upstream_connected && host_state == HostState::kReady) {
            host_state = HostState::kUpstreamConnected;
            drawStatus();
        }

        const bool touched = M5.Touch.getCount() > 0;
        if (host_state == HostState::kReady && touched && !last_touch) {
            startHost();
        }
        last_touch = touched;
    }

    static HostState last_drawn = HostState::kFailed;
    if (host_state != last_drawn) {
        last_drawn = host_state;
        drawStatus();
    }
    delay(50);
}
