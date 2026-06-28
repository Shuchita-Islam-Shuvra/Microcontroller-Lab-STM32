# Assignment 3 — BME280/BMP280 Environmental Sensor Interface

**Course:** CSE 2206 — Microcontroller and Embedded System
**Platform:** STM32F446RE Nucleo-64
**IDE:** STM32CubeIDE (Bare-Metal & HAL)
**Terminal:** PuTTY @ 115200 baud

---

## 📋 Overview

This assignment interfaces the STM32F446RE with a BMP280/BME280 environmental sensor using two communication protocols:

- **Part A:** SPI (4-wire) — Temperature & Pressure
- **Part B:** I2C (2-wire) — Temperature & Pressure

Both implementations are done in **Bare-Metal (register-level)** and **HAL (CubeIDE)**.

---

## 📁 Folder Structure

```
Assignment 3/
├── Part A SPI no hall/    → SPI Bare-Metal implementation
├── Part A SPI hall/       → SPI HAL implementation
├── Part B I2C no hall/    → I2C Bare-Metal implementation
└── Part B I2C hall/       → I2C HAL implementation
```

---

## 🔌 Hardware Connections

### Part A — SPI (SPI1)

| Signal | STM32 Pin | Sensor Pin |
|--------|-----------|------------|
| SCK    | PA5 (D13) | SCL/SCK    |
| MISO   | PA6 (D12) | SDO        |
| MOSI   | PA7 (D11) | SDA/SDI    |
| CS     | PA4 (D10) | CSB        |
| VCC    | 3.3V      | VCC        |
| GND    | GND       | GND        |

### Part B — I2C (I2C1)

| Signal | STM32 Pin    | Sensor Pin |
|--------|--------------|------------|
| SCL    | PB6 (D10)    | SCL        |
| SDA    | PB7 (CN7-21) | SDA        |
| CSB    | 3.3V         | CSB (I2C mode select) |
| SDO    | GND          | SDO (address = 0x76)  |
| VCC    | 3.3V         | VCC        |
| GND    | GND          | GND        |

---

## ⚙️ Peripheral Configuration

| Peripheral | Setting |
|------------|---------|
| System Clock | 180 MHz (HSI + PLL) |
| USART2 | 115200 baud, 8N1 |
| TIM6 | PSC=8999, ARR=9999 → 1 second interrupt |
| SPI1 | Master, Mode 0 (CPOL=0, CPHA=0), 8-bit, MSB first |
| I2C1 | Standard Mode, 100 kHz |

---

## 🌡️ Sensor Notes

The sensor used in this lab identified as **BMP280** (Chip ID: `0x58`), not BME280 (`0x60`). The BMP280 provides **Temperature** and **Pressure** only — it does not have a humidity sensor. All readings and verification tests reflect this.

---

## 🖥️ Sample UART Output

### Part A — SPI
```
========================================
BME280 via SPI1 HAL -- CSE 2206 Lab A
========================================
[A1] ChipID=0x58 (expect 0x60)
BMP280 detected.
Calibration loaded OK.
Starting readings...

[A3] Tick:1
[SPI-HAL] Temp:28.45C/83.21F Pres:1009.45hPa
[A4] Plausibility PASS
----------------------------------------
```

### Part B — I2C
```
========================================
BMP280 via I2C HAL -- CSE 2206 Lab B
========================================
[B1] ChipID=0x58 (expect 0x60 or 0x58)
BMP280 detected.
[B2] I2C ACK OK
Calibration loaded OK.
Starting readings...

[B3] Tick:1
[I2C-HAL] Temp:28.46C/83.23F Pres:1009.42hPa
[B4] Plausibility PASS
----------------------------------------
```

---

## ✅ Verification Tests

| Test | Description | Status |
|------|-------------|--------|
| A1 / B1 | Chip ID check | ✅ Pass (0x58 — BMP280) |
| A2 / B2 | UART loopback / I2C ACK | ✅ Pass |
| A3 / B3 | TIM6 1-second heartbeat | ✅ Pass |
| A4 / B4 | Sensor plausibility check | ✅ Pass |
| A5 / B5 | HAL vs Bare-Metal comparison | ✅ See table below |

### Test A5 — SPI: HAL vs Bare-Metal

| Reading # | BM Temp (°C) | HAL Temp (°C) | Diff | Pass (≤0.1°C)? |
|-----------|--------------|----------------|------|-----------------|
| 1 | — | — | — | — |
| 2 | — | — | — | — |
| 3 | — | — | — | — |

### Test B5 — I2C: HAL vs Bare-Metal

| Reading # | BM Temp (°C) | HAL Temp (°C) | Diff | Pass (≤0.1°C)? |
|-----------|--------------|----------------|------|-----------------|
| 1 | — | — | — | — |
| 2 | — | — | — | — |
| 3 | — | — | — | — |

---

## 📊 Final Comparison: SPI vs I2C

| Aspect | SPI | I2C |
|--------|-----|-----|
| Wires to sensor | 4 (MOSI, MISO, SCK, CS) | 2 (SDA, SCL) |
| STM32 Peripheral | SPI1 | I2C1 |
| Max Bus Speed | Up to 22.5 MHz (fPCLK/8) | 100 kHz (Standard mode) |
| Pull-up Resistors | Not required | Required (handled internally via PUPDR) |
| CS Pin Required | Yes (PA4) | No — address-based (0x76) |
| Multiple Slaves | One CS per slave | Address-based sharing |
| Your Temp (°C) | — | — |
| Your Pressure (hPa) | — | — |
| Readings Match? | Yes / No (within 0.1°C) | |

---

## 💬 Discussion Questions

**1. When would you choose SPI over I2C in a real product?**

SPI is preferable when high data throughput is required (e.g., displays, ADCs, flash memory) and pin count is not a constraint. Its full-duplex nature and higher clock speeds make it ideal for single-sensor, performance-critical applications. I2C is better suited for systems with multiple low-speed peripherals sharing limited GPIO pins, such as sensor hubs on compact PCBs, since it only needs two wires regardless of the number of devices on the bus.

**2. Why is the firmware compensation code identical in both parts?**

The compensation formulas operate on raw ADC values that originate from the same physical sensing element inside the BMP280/BME280 chip. SPI and I2C are only the *transport mechanisms* used to move register data between the STM32 and the sensor — they do not alter the underlying measurement or its digital representation. Since the calibration coefficients and raw ADC format are identical regardless of bus protocol, the compensation math (converting raw counts to °C/hPa) remains the same in both implementations.

---

## 🛠️ Tools Used

- STM32CubeIDE 2.1.0
- STM32CubeMX (for HAL code generation)
- PuTTY (UART terminal, 115200 baud)
- GitHub (version control)

---

## 👤 Author

Lab Assignment 3 — CSE 2206, Department of CSE, University of Dhaka
