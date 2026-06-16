# 🌱 Smart Irrigation System

This repository contains the code and resources for my **Smart Irrigation System** project – an automated soil-moisture-based watering controller built using embedded hardware and real-time sensor monitoring.

This system automatically waters plants/farm soil when moisture levels drop below a threshold to ensure healthy hydration while saving water. It’s ideal for gardens, small farms, or remote irrigation setups. :contentReference[oaicite:1]{index=1}

---

## 📌 Overview

A *Smart Irrigation System* continuously monitors soil moisture and activates a water pump only when necessary, optimizing water usage and minimizing manual intervention. :contentReference[oaicite:2]{index=2}

This project uses embedded firmware to:
- Read soil moisture data
- Activate/deactivate irrigation pumps accordingly
- Improve resource efficiency and reduce water waste

---

## 🧩 System Features

✅ **Automated watering** based on real soil moisture levels  
✅ **Efficient water usage** to conserve resources  
✅ **Simple embedded design** for educational and practical use  
✅ Ready for **expansion with IoT/cloud features** :contentReference[oaicite:3]{index=3}

---

## 🛠️ Components (Typical / Example)

> *(Customize this list based on your actual build)*

| Component | Purpose |
|-----------|---------|
| Microcontroller (e.g., ESP32 / Arduino) | Core processing & I/O control |
| Soil Moisture Sensor | Measures volumetric water content |
| Relay Module | Controls pump switching |
| Water Pump | Performs irrigation |
| Power Supply | Provides stable voltage |

---

## 🚀 How It Works

1. Sensor continuously reads soil moisture level.
2. Microcontroller interprets sensor data.
3. If moisture < threshold → **Pump turns ON**.
4. If moisture ≥ threshold → **Pump turns OFF**.

This logic ensures just-in-time irrigation and prevents **overwatering or dryness**. :contentReference[oaicite:4]{index=4}

---

## 🧪 Getting Started

### 📋 Prerequisites

Install the following on your computer:

- PlatformIO / Arduino IDE
- USB drivers for your microcontroller
- Required sensor libraries (e.g., for soil moisture)

### 🚧 Build & Upload

1. Clone this repository  
   ```bash
   git clone https://github.com/PrashilLamichhane/Smart_Irrigation_system.git
