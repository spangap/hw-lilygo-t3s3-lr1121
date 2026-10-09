/**
 * t3s3lr1121.h — LilyGo T3-S3 (LR1121) board support for reticulous.
 *
 * The T3-S3 (LoRa32) is an ESP32-S3 LoRa node built on the ESP32-S3-WROOM-1(U)
 * (ESP32-S3FH4R2: 4 MB flash, 2 MB *quad* PSRAM). This straddle targets the
 * LR1121 (dual-band LR11x0) radio variant, on its own SPI bus, plus the board's
 * microSD slot on a second SPI bus. The 0.96" SSD1306 OLED is wired as a paged
 * status display via spangap/tinylcd (pins published in straddle.yaml). See
 * t3s3lr1121.cpp for the implementation and the board reference:
 * https://wiki.lilygo.cc/products/t3-series/t3-s3/
 *
 * What this module provides:
 *   - The always-on board bring-up entry point T3s3Lr1121Board::onStart().
 *
 * The board has no gated peripheral power rail, so there is no board-owned
 * power pin here. The LR1121's pins (NSS/SCK/MOSI/MISO/RST/BUSY/IRQ), TCXO and
 * DIO5/DIO6 RF-switch table belong to iface-lora's CONFIG_LORA* knobs, and the
 * microSD pins to spangap-core's CONFIG_SPANGAP_SDCARD_* knobs — both set as
 * board VALUES in this straddle's `kconfig:` block.
 */
#pragma once

#include "sdkconfig.h"
#include "service.h"

#define BOARD_NAME              "LilyGo T3-S3 (LR1121)"

/**
 * Board bring-up, as a registered Service. T3s3Lr1121Board::onStart is the
 * always-on hardware bring-up: it parks the LoRa radio's CS line HIGH so the
 * LR1121 doesn't drive MISO before the LoRa interface claims it. It runs in the
 * start band, before spangapInit() (and so before fs_mount_sd() touches the
 * separate SD bus). There is no onInit companion — the OLED UI is tinylcd's own
 * service, not a board hook.
 */
class T3s3Lr1121Board : public Service {
public:
    void onStart() override;
};
