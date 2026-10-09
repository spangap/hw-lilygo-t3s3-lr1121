# hw-lilygo-t3s3-lr1121 — INTERNALS

Maintainer reference for the LilyGo T3-S3 (LR1121) board HAL. For what the
straddle is and how to build with it, read [README.md](README.md) first.

## 1. Bring-up: the chip-select park

The only firmware this straddle owns is `T3s3Lr1121Board::onStart`
([esp-idf/src/t3s3lr1121.cpp](esp-idf/src/t3s3lr1121.cpp)). It configures the
LR1121's chip-select (`CONFIG_LORA0_CS_PIN`, GPIO 7) as an output and drives it
HIGH — deselected — so the radio does not drive MISO before `loraInit()` (in
[iface-lora](../iface-lora)) claims the SPI pins. It runs in the `start:` band,
before `spangapInit()`, and is guarded by `#if defined(CONFIG_LORA0_CS_PIN)` so
the source compiles when the board is staged without iface-lora.

The board has no peripheral power rail. The microSD card sits on its own SPI bus
(host 3: SCK 14 / MOSI 11 / MISO 2, CS 13) and its CS is claimed by
`fs_mount_sd()`, so it needs no pre-park; the radio is on host 2 (SCK 5 / MOSI 6
/ MISO 3, CS 7). Keep them on distinct hosts — the two buses share no pins.

## 2. The radio's RF switch

The LR1121 has no DIO2-style single switch line. Its front end is steered by the
chip's own DIOs from a table the chip applies on every mode change. On this
board DIO5 selects the receive path and DIO6 the sub-GHz transmit path; neither
is high in standby or for a 2.4 GHz transmit. The table is board data, so it
lives in `straddle.yaml` as iface-lora's `CONFIG_LORA0_LR_RFSW_*` masks, and
iface-lora programs it after every `begin()` (whose reset clears it). Only
`_IDLE`, `_RX`, `_TX` and `_TX_HF` mean anything on an LR11x0: it has one
receive row for both bands, so `_RX_HF` is left at 0.

The IRQ is the chip's DIO9, fixed on this part, wired to GPIO 36 — which is what
`CONFIG_LORA0_DIO1_PIN` names. `CONFIG_LORA0_LR_IRQ_DIO` is an LR2021 setting
and stays at its default.

The TCXO runs from the chip's DIO3 at **3.0 V**, not the 1.8 V of the SX1262
T3-S3. If the voltage were wrong, iface-lora's TCXO fallback would retry on a
bare crystal and warn naming `CONFIG_LORA0_TCXO_MV`.

## 3. Everything else is Kconfig VALUES, not sources

A board straddle is **non-buildable**: a `sdkconfig.defaults` here would be
ignored under `--with`. So the hardware profile lives entirely in
`straddle.yaml`'s `kconfig:` block: memory (`CONFIG_ESPTOOLPY_FLASHSIZE_4MB`,
`CONFIG_SPIRAM_MODE_QUAD`, `CONFIG_SPANGAP_MAX_FIRMWARE_KB=3840`), the
`CONFIG_LORA*` radio pins and flags, the `CONFIG_SPANGAP_SDCARD*` pins and the
`CONFIG_TINYLCD_*` pins. This board defines no new symbols; it only supplies
values.

## 4. Sourcing & pitfalls

The pin map, TCXO voltage and RF-switch table come from the `tlora_t3s3_v1`
Meshtastic variant's LR1121 block, which matches LilyGo's own examples, **not**
from a board in hand. Things to confirm on real hardware:

- **RF switch** — a wrong table gives a radio that initialises cleanly and is
  deaf, or transmits into nothing. Receive first, then transmit, at short range.
- **2.4 GHz** — untested; the 2.4 GHz transmit row is all-low per the reference
  table, so the HF path is not switched by DIO5/DIO6.
- **4 MB flash is tight** — the same floor as the SX1262 board: `/state` gets
  256 KB and there is no A/B OTA pair.
- **Internal-DRAM headroom** — quad PSRAM plus Wi-Fi/lwIP plus the SD bounce
  buffer all draw on the internal pool; watch for `ESP_ERR_NO_MEM` under a busy
  Wi-Fi + SD workload.
