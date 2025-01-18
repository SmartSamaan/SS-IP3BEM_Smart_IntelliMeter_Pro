# SS-IP3BEM_Smart_IntelliMeter_Pro
Smart IntelliMeter Pro: WiFi Based Three-Phase Bidirectional Energy Metering Evaluation/Development Board <br />
Smart IntelliMeter Pro SS-IP3BEM is an industrial grade WiFi MCU based 3-phase bidirectional energy metering development/evaluation board with an onboard ESP32-WROOM-32UE WiFi/Bluetooth controller incorporating a RS-485 communication port. The board has the capability to interface with a character LCD (16x2, 20x2, 16x4, or 20x4) in 4-bit mode as well as a serial I2C display. The on-chip flash memory of the controller is used to store the energy variables as well as calibration constants. The main features of the board are:<br />
•	ESP32-WROOM-32UE controller 4MB Flash, 448kB ROM, 536kB SRAM<br />
•	WiFi protocol: 802.11b/g/n, Bluetooth v4.2 +EDR, Class 1, 2 and 3<br />
•	USB Type-C port for serial communication between the board and PC to access measurement data on the PC as well as programming of the MCU<br />
•	I2C port for serial interfacing of OLED displays (e.g., 0.96" or 1.3" OLED display or similar)<br />
•	4-bit interface for character LCDs (e.g., 16x2, 20x2, 16x4, 20x4 or similar)<br />
•	A JTAG interface for programming of the MCU using ESP-IDF<br />
•	Two pushbuttons for custom navigation of LCD display windows<br />
•	RS-485 communication port for industrial grade communication with other devices <br />
The board includes ADE7758 24-pin SOIC which can also communicate via SPI communication with any external microcontroller unit to get the energy measurements. The technical specifications of the ADE7758 are:<br />
•	Highly accurate; supports IEC 60687, IEC 61036, IEC 61268, IEC 62053-21, IEC 62053-22, and IEC 62053-23<br />
•	Compatible with 3-phase/3-wire, 3-phase/4-wire, and other 3-phase services<br />
•	Less than 0.1% active energy error over a dynamic range of 1000 to 1 at 25°C<br />
•	Supplies active/reactive/apparent energy, voltage RMS, current RMS, and sampled waveform data.<br />
•	Two pulse outputs, one for active power and the other selectable between reactive and apparent power with programmable frequency<br />
•	Digital power, phase, and RMS offset calibration.<br />
•	On-chip, user-programmable thresholds for line voltage SAG and overvoltage detections<br />
•	An on-chip, digital integrator enables direct interface-to-current sensors with di/dt output.<br />
•	A PGA in the current channel allows a direct interface to current transformers.<br />
•	A SPI-compatible serial interface with IRQ<br />
•	3-phase bidirectional energy measurement<br />

The Energy_Meter.zip folder contains the html and php files for sending data to the localhost webserver. For localhost running, install the platform like xampp first.
