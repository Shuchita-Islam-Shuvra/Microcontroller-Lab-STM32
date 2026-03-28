# 🔬 Microcontroller Lab — STM32 (NUCLEO-F446RE)

A complete collection of STM32 microcontroller lab experiments using both HAL (Hardware Abstraction Layer) and Bare-Metal (No HAL) programming approaches with the NUCLEO-F446RE development board.


## 📋 Table of Contents

- [Prerequisites](#prerequisites)
- [Lab Overview](#lab-overview)
- [Project Structure](#project-structure)
- [HAL Programming Workflow](#hal-programming-workflow)
- [Bare-Metal Programming Workflow](#bare-metal-programming-workflow)
- [Project Workspace Location](#project-workspace-location)
- [Notes](#notes)
- [License](#license)

---

## 🛠️ Prerequisites

Make sure the following tools are installed before getting started **(version 2.1.0)**:

- STM32CubeMX
- STM32CubeIDE
- Hardware: NUCLEO-F446RE development board
- Cable: USB Type-A to Mini-B
- Operating System: Windows 11



## 🧪 Lab Overview

| Lab | Title | HAL | Bare-Metal |
|-----|-------|-----|------------|
| Lab 1 | LED Blink | ✅ Lab-1B | ✅ Lab-1A |
| Lab 2 | UART Communication | ✅ Lab-2B | ✅ Lab-2A |
| Lab 3 | External Interrupt | ✅ Lab-3B | ✅ Lab-3A |
| Lab 4 | UART Receive Interrupt | ✅ Lab-4B | ✅ Lab-4A |
| Lab 5 | UART DMA Transfer | ✅ Lab-5B | ✅ Lab-5A |

---

## 📁 Project Structure

```
Microcontroller-Lab-STM32/
├── Lab-1A LED BLINK Bare metal/
├── Lab-1B LED BLINK hall/
├── Lab-2A UART no hall/
├── Lab-2B UART hall/
├── Lab-3A interrupt no hall/
├── Lab-3B interrupt hall/
├── Lab-4A Uart receive interrupt no hall/
├── Lab-4B UART receive interrupt hall/
├── Lab-5A UART DMA no hall/
└── Lab-5B UART DMA Hall/
```

---

## ⚙️ HAL Programming Workflow

HAL programming uses **STM32CubeMX** to auto-generate initialization code, which is then imported into STM32CubeIDE.

### Step 1 — Generate Code with STM32CubeMX

```
STM32CubeMX
  └── From New Project
        └── Start My Project from MCU
              └── ACCESS TO MCU SELECTOR
                    └── Commercial Part Number: STM32F446RET6TR
                          └── Start Project
                                └── Initialize all peripherals with default mode? → Yes
```

### Step 2 — Configure the Project Manager

```
Project Manager
  ├── Project Name     → LAB_1B <your-project-name>
  ├── Project Location → C:\Users\USER\STM32CubeIDE\workspace_2.1.0\
  └── Toolchain / IDE  → STM32CubeIDE
        └── GENERATE CODE
```

| Prompt | Action |
|--------|--------|
| Project Manager Settings Popup | Click Yes |
| Notification | Select Don't ask me again |
| Licence Agreement | Click Agree → Finish |
| Windows Security Alert | Click Allow |
| Success Dialog | Click Close |

### Step 3 — Import Project into STM32CubeIDE

```
STM32CubeIDE
  └── File
        └── Open Projects from File System
              └── Import Source → Select project folder → Finish
```

---

## 🔩 Bare-Metal Programming Workflow

Bare-metal programming gives **full control over hardware** without abstraction layers. Projects are created directly in STM32CubeIDE.

### Step 1 — Create New Project

```
STM32CubeIDE
  └── File
        └── STM32 Project Create/Import
              └── Create New STM32 Project
                    └── STM32CubeIDE Empty Project
                          └── Next
                                └── MCU/MPU Selector → STM32F446RETx
                                      └── Next
                                            └── Project Name → LAB_1A
                                                  └── Finish
```

### Step 2 — Setup Drivers



## 📂 Project Workspace Location

By default, STM32CubeIDE projects are saved at:

C:\Users\USER\STM32CubeIDE\workspace_2.1.0\

## 📌 Notes

- Replace `<your-project-name>` with your actual project name.
- **HAL workflow** is recommended for beginners — auto-generates peripheral configuration code.
- **Bare-Metal workflow** is for advanced users who want full hardware control.
- All experiments are based on the **STM32F446RE** microcontroller.

---

## 📄 License

This project is licensed under the MIT License.
See the LICENSE file for details.ShareContent
