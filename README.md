# Motor Tester

## Functional Requirements: CYD Motor Control & RPM Sensing

This document outlines the functional requirements for a system utilizing the **Cheap Yellow Display (ESP32-2432S028R)** to control a DC motor via PWM and measure its rotational speed using an LED gate (optical) sensor.

### 1. Requirement Summary Table

| ID | Category | Requirement | Description |
|:---|:---|:---|:---|
| FR-01 | Motor Control | PWM Signal Generation | The system shall generate a PWM signal via ESP32 GPIO to regulate effective motor voltage. |
| FR-02 | Motor Control | Minimum Start Threshold | The system shall only activate the motor if the PWM duty cycle exceeds the 'stiction' threshold (e.g., ~30%). |
| FR-03 | Motor Control | Soft-Start / Kickstart | The system shall apply a momentary high-voltage pulse (Kickstart) when transitioning from 0 to 1 to overcome static friction. |
| FR-04 | Motor Control | Linear PWM Mapping | The system shall map UI slider percentage (0-100%) to the hardware PWM range (0-255 or 0-1023). |
| FR-05 | Sensing | Optical Interrupt Detection | The system shall detect state changes from the LED gate sensor using hardware interrupts. |
| FR-06 | Sensing | RPM Calculation Logic | The system shall calculate RPM based on the time delta between consecutive sensor pulses. |
| FR-07 | Sensing | Real-Time Display Refresh | The system shall refresh the RPM numerical data on the CYD screen every 500ms. |
| FR-08 | Analysis | Input-Feedback Correlation | The system shall allow monitoring of the actual RPM vs. the commanded PWM (Activation Line). |
| FR-09 | UI/UX | Touch Control Interface | The system shall provide a touch-sensitive slider on the 320x240 TFT to adjust motor power. |
| FR-10 | UI/UX | Visual Speed Feedback | The system shall display the current RPM alongside a visual progress bar or graph. |
| FR-11 | Safety | Emergency Shutdown | The system shall provide a dedicated on-screen 'STOP' button to immediately zero the PWM output. |
| FR-12 | Safety | Stall Protection | The system shall disable PWM output if no pulses are detected for 2 seconds while power is commanded. |

### 2. Detailed Activation Logic (RPM vs. PWM)

#### 2.1 Activation Line Characteristics
The system follows a specific activation curve where the motor remains stationary until a specific voltage threshold is reached.

* **Dead Zone:** PWM 0 to ~75 (0V - 1.5V). Motor is inactive or humming.
* **Activation Point:** PWM ~80. The "Stall Voltage" is overcome, and rotation begins.
* **Operating Range:** PWM 80 to 255. Linear increase in RPM relative to duty cycle.

#### 2.2 Mathematical Model
The software uses hardware interrupts to ensure accuracy:
- **Time Delta ($dt$):** Time in microseconds between two LED gate interruptions.
- **RPM Formula:** $RPM = \frac{60,000,000}{dt \times \text{pulses\_per\_rev}}$

### 3. Hardware Interface Mapping (CYD Specific)

| Component | ESP32 Pin (GPIO) | Mode |
|:---|:---|:---|
| **Motor PWM** | GPIO 22 or 27 | Output (LEDC) |
| **LED Gate Sensor** | GPIO 21 | Input (Internal Pull-up) |
| **TFT Display** | Standard SPI | Output |
| **Touch Controller** | Standard SPI | Input |

---
*Generated for project development in VS Code with ESP32 Arduino Core.*
