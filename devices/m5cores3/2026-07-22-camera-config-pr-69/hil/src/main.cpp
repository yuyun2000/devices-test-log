#include <Arduino.h>
#include <M5CoreS3.h>
#include <esp_camera.h>

#ifndef PR69_VARIANT
#define PR69_VARIANT 0
#endif

namespace {

constexpr int kFramesToCheck = 4;

#if PR69_VARIANT == 2
camera_config_t custom_config;
#endif

uint32_t fnv1a(const uint8_t* data, size_t length) {
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < length; ++i) {
        hash ^= data[i];
        hash *= 16777619u;
    }
    return hash;
}

const char* variantName() {
#if PR69_VARIANT == 0
    return "main_baseline";
#elif PR69_VARIANT == 1
    return "pr69_default";
#else
    return "pr69_custom";
#endif
}

void showResult(const char* status, int frames, int width, int height) {
    CoreS3.Display.fillScreen(status[0] == 'P' ? DARKGREEN : MAROON);
    CoreS3.Display.setTextColor(WHITE);
    CoreS3.Display.setTextSize(2);
    CoreS3.Display.setCursor(8, 12);
    CoreS3.Display.printf("PR #69 %s\n\n%s\n\nframes=%d\nsize=%dx%d\n",
                          variantName(), status, frames, width, height);
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(1200);

    auto cfg = M5.config();
    CoreS3.begin(cfg);

    bool init_ok = false;
    int expected_width = 320;
    int expected_height = 240;
    pixformat_t expected_format = PIXFORMAT_RGB565;

#if PR69_VARIANT == 2
    custom_config = *CoreS3.Camera.config;
    custom_config.pixel_format = PIXFORMAT_GRAYSCALE;
    custom_config.frame_size = FRAMESIZE_VGA;
    custom_config.fb_count = 2;
    expected_width = 640;
    expected_height = 480;
    expected_format = PIXFORMAT_GRAYSCALE;
    init_ok = CoreS3.Camera.begin(&custom_config);
    const bool pointer_ok = CoreS3.Camera.config == &custom_config;
#else
    init_ok = CoreS3.Camera.begin();
    const bool pointer_ok = true;
#endif

    Serial.printf("PR69_INIT variant=%s init=%d config_pointer=%d\n",
                  variantName(), init_ok, pointer_ok);

    if (!init_ok || !pointer_ok) {
        Serial.printf("PR69_RESULT variant=%s status=FAIL reason=init\n", variantName());
        showResult("FAIL", 0, 0, 0);
        return;
    }

    bool frame_shape_ok = true;
    bool hash_nonzero = true;
    int captured = 0;
    int last_width = 0;
    int last_height = 0;

    for (int i = 0; i < kFramesToCheck; ++i) {
        if (!CoreS3.Camera.get()) {
            Serial.printf("PR69_FRAME index=%d status=GET_FAIL\n", i);
            frame_shape_ok = false;
            delay(100);
            continue;
        }

        camera_fb_t* frame = CoreS3.Camera.fb;
        const uint32_t hash = fnv1a(frame->buf, frame->len);
        const bool shape_ok = frame->width == expected_width &&
                              frame->height == expected_height &&
                              frame->format == expected_format &&
                              frame->len > 0;
        Serial.printf(
            "PR69_FRAME index=%d width=%u height=%u format=%d len=%u fnv1a=%08lX shape=%d\n",
            i,
            static_cast<unsigned>(frame->width),
            static_cast<unsigned>(frame->height),
            static_cast<int>(frame->format),
            static_cast<unsigned>(frame->len),
            static_cast<unsigned long>(hash),
            shape_ok);

        last_width = frame->width;
        last_height = frame->height;
        frame_shape_ok = frame_shape_ok && shape_ok;
        hash_nonzero = hash_nonzero && hash != 0;
        ++captured;
        CoreS3.Camera.free();
        delay(80);
    }

    const bool pass = captured == kFramesToCheck && frame_shape_ok && hash_nonzero;
    Serial.printf(
        "PR69_RESULT variant=%s status=%s frames=%d expected=%dx%d format=%d\n",
        variantName(), pass ? "PASS" : "FAIL", captured, expected_width,
        expected_height, static_cast<int>(expected_format));
    showResult(pass ? "PASS" : "FAIL", captured, last_width, last_height);
}

void loop() {
    delay(1000);
}
