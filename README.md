# hw-lilygo-t3s3-lr1121 — LilyGo T3-S3 (LR1121) board HAL

**hw-lilygo-t3s3-lr1121** is the board-support straddle for the **LilyGo T3-S3**
(LoRa32) with a Semtech **LR1121** — the dual-band (sub-GHz and 2.4 GHz) LR11x0
part. The board is an ESP32-S3 LoRa node built on the **ESP32-S3-WROOM-1(U)**
(ESP32-S3FH4R2: 4 MB flash, 2 MB **quad** PSRAM), with the radio on its own SPI
bus and a microSD slot on a second one. It makes the board usable by an
application: it owns the LoRa chip-select park and publishes the board's pin
map and hardware tuning as Kconfig. Board reference:
<https://wiki.lilygo.cc/products/t3-series/t3-s3/>.

It is a **non-buildable** component — it decides nothing about what the device
*does*. A buildable assembler (`reticulous/reticulous`) adds it and inherits the
board: `spangap build reticulous/reticulous --with spangap/hw-lilygo-t3s3-lr1121`. The
mesh stack, the IP/web platform, `app_main`, the partition layout, the update
story and the browser SPA all come from the buildable and its other straddles —
not from here.

It brings up **LoRa + microSD + the 0.96" SSD1306 OLED**, the latter as a paged
status display via [tinylcd](../tinylcd) (staged by this board through
`additional_installs`, pins fed through a gated `kconfig:` group); the BOOT
button (GPIO 0) advances the page.

## Which T3-S3

The T3-S3 ships with several radios on the same PCB, and each has its own
straddle because the radio driver differs. This one is the **LR1121**; the
SX1262 board is [hw-lilygo-t3s3-sx1262](../hw-lilygo-t3s3-sx1262), whose README
also covers the SX127x and SX1280 variants. Against the SX1262 board, only the
radio's wiring changes: the same SPI, CS, RST and BUSY pins, but the IRQ on
**GPIO 36** (the chip's DIO9), a **3.0 V** TCXO rather than 1.8 V, and an
antenna switch the chip drives from its own **DIO5/DIO6** rather than an
SX126x's DIO2.

## ⚠️ Verify before trusting

The pin map below comes from LilyGo's T3-S3 wiki and the `tlora_t3s3_v1`
Meshtastic variant's LR1121 block (pins, TCXO voltage and RF-switch table),
**not** from a board in hand. Nobody has run reticulous on this board yet.
Confirm against your unit before an RF or partition run:

- **Radio variant.** `detect_hw()` (below) halts any T3-S3 whose radio does not
  answer as an LR1121.
- **RF switch.** DIO5 high on receive, DIO6 high on sub-GHz transmit, both low
  in standby and on 2.4 GHz transmit. A wrong table shows as a radio that comes
  up and transmits but is deaf, or one whose transmissions never leave the
  board.
- **Flash size.** 4 MB / 2 MB quad PSRAM, as on the SX1262 board: the firmware
  floor leaves only 256 KB for `/state` and no room for an A/B OTA pair.

## What it does, and how it fits

The board contributes one hook that the buildable's generated init dispatcher
calls. There is nothing to call by hand: if the straddle is in the build, the
board comes up automatically.

| Hook | Band | Present when | Brings up |
|---|---|---|---|
| `T3s3Lr1121Board::onStart` | start | always | LoRa chip-select park HIGH |

`onStart` runs in the `start:` band, **before** `spangapInit()`. It parks the
LR1121's CS line HIGH so the radio does not drive MISO before `loraInit()` (in
[iface-lora](../iface-lora)) claims the pin. There is no `init:`-band companion
— the OLED UI is [tinylcd](../tinylcd)'s own service. The microSD card is
mounted by `spangapInit()`'s `fs_mount_sd()` (it sits on its own SPI bus, so its
CS needs no pre-park). The board has no gated peripheral power rail.

## Board identity (`detect_hw`)

`esp-idf/src/detect.cpp` returns `"hw-lilygo-t3s3-lr1121"` when the hardware
under the firmware is this board, and NULL when it is not. It asks for 4 MB of
flash, then the OLED acking on 18/17, and then the radio must read as an
**LR1121** (GetVersion device byte 0x03) — the same PCB carrying an SX1262
answers NULL and belongs to the SX1262 straddle. Passive reads only; no rail is
driven.

spangap-core calls it before the first `onStart()` and **halts the device
awake** when the answer disagrees with the board this image was built for. The
confirmed answer is published as `sys.hw` and announced on the console as
`build: hw hw-lilygo-t3s3-lr1121`. flashmon's standalone detector carries a
hand-kept copy of the same function, renamed `detect_hw_lilygo_t3s3_lr1121`;
change one, change the other. See
[spangap-core/docs/init.md](../spangap-core/docs/init.md) and
[flashmon/docs/detect.md](../flashmon/docs/detect.md).

## Hardware & pin map

LilyGo **T3-S3** — ESP32-S3-WROOM-1(U) module (4 MB flash, 2 MB **quad** PSRAM,
selected via `CONFIG_SPIRAM_MODE_QUAD`). Two independent SPI buses: the LR1121
on **SPI host 2** (FSPI) and the microSD on **SPI host 3** (HSPI).

### LoRa LR1121 (owned by iface-lora, pins published here)

| Signal | GPIO | | Signal | GPIO |
|---|---|---|---|---|
| NSS / CS | 7 | | RST | 8 |
| SCK | 5 | | BUSY | 34 |
| MOSI | 6 | | IRQ (chip DIO9) | 36 |
| MISO | 3 | | | |

One radio (`CONFIG_LORA_COUNT=1`), `CONFIG_LORA0_RADIO_LR1121=y`, TCXO at 3.0 V
(`CONFIG_LORA0_TCXO_MV=3000`). The antenna switch is the chip's own RF-switch
table, set from iface-lora's `CONFIG_LORA0_LR_RFSW_*` masks (bit 0 = DIO5):

| Mode | Mask | DIO5 | DIO6 |
|---|---|---|---|
| standby | `0x00` | low | low |
| receive (both bands) | `0x01` | high | low |
| sub-GHz transmit | `0x02` | low | high |
| 2.4 GHz transmit | `0x00` | low | low |

The band follows the configured frequency: iface-lora begins the LR1121 on its
2.4 GHz front end when the carrier is above 2 GHz, and the transmit ceiling
then comes from `CONFIG_LORA_TX_POWER_MAX_HF` (default 12 dBm) instead of the
sub-GHz 22 dBm.

### microSD (owned by spangap-core, pins published here)

| Signal | GPIO |
|---|---|
| CS | 13 |
| SCK | 14 |
| MOSI | 11 |
| MISO | 2 |

On its own SPI bus (**host 3**), separate from the radio's. Mounted at boot when
`CONFIG_SPANGAP_SDCARD=y`.

### OLED + page button (owned by tinylcd, pins published here)

| Signal | GPIO | Notes |
|---|---|---|
| OLED SDA / SCL | 18 / 17 | 0.96" SSD1306 128×64, address 0x3C, no reset line |
| page button (BOOT) | 0 | click = next status page; 500 ms hold = screen off; press wakes a dark screen |

### Memory / flash (published from `kconfig:`)

| Key | Value | Why |
|---|---|---|
| `CONFIG_ESPTOOLPY_FLASHSIZE_4MB` | `y` | 4 MB flash (ESP32-S3FH4R2) |
| `CONFIG_SPANGAP_MAX_FIRMWARE_KB` | `3840` | state floor at 3.75 MB: the reticulous binary fits the `app` slot alongside the ~704 KB `fixed` partition; `/state` keeps the remaining 256 KB, enough because bulk data belongs on the microSD card. Without it `app` eats all 4 MB and there is **no `/state`** |
| `CONFIG_SPIRAM_MODE_QUAD` | `y` | the S3FH4R2 carries 2 MB PSRAM in **quad** mode, not octal |

## Storage variables

This board defines no storage keys of its own. Runtime LoRa parameters live at
`s.lora.*` ([iface-lora](../iface-lora)); the SD mount is owned by
[spangap-core](../spangap-core).

## Dependencies

- [spangap-core](../spangap-core) — base runtime (storage, log, CLI, fs incl. the
  SD mount, ITS).
- [iface-lora](../iface-lora) — owns the LR1121 radio engine and programs its
  RF-switch table; this board parks its CS and supplies its pins via Kconfig.
- [tinylcd](../tinylcd) — staged by this board (`additional_installs`); owns
  the OLED paged UI and the page button, pins supplied via Kconfig.

## Read next

- [INTERNALS.md](INTERNALS.md) — the CS-park bring-up, the radio's RF switch,
  and the board pitfalls.
