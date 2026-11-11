ESPHome component to handle an i80 NV3041A panel.
---

**Current status: Alpha (Display protocol to code, esphome touchscreen ok**

For now, ESPHome mipi display extensions don't handle i80 (intel 8080 / 8 bits parallel) protocol.

Unfortunately, the display driver is not available in i80 (only in quad SPI) in the LovyanGLX library neither
and the "IM" pins hardcode the protocol to use (to GND) on the controller flat cable (to inspect with good eyes ;).

The factory SDK is available and should be usable, but with bulk data init in Hexadecimal to import.

Made for my own use with an inverter and maybe some BMS, this CAN/RS485 panel board is based on a ESP32 S3 (16MB)
with a display NV3041A. Full ref on the PCB is PanelLan ZX4D30CE08S-V12 but its sold also as
WT32S3-43H on alibaba. It may be seen with the 4827 suffix (on the box) for the resolution 480x272.
WT prefix is for Wireless-Tag, visible also on the ESP shield on the PCB.

For sure it allow also bluetooth and wifi and a few external gpios,
it's sold with 2 sets of connectors, one for CAN/RS485/RS232 and the other to debug.

This allow to use the integrated touchscreen CST3240 to create custom buttons and to command
the pwm backlight from ESPHome, to avoid backlight when unused...

The panel also has a buzzer which can do alarms, its configured in the sample for the touches.

Sample yaml is based on JK BMS modbus project (syssi) and show the display components integration to query a few entities.
It doesn't require any other component from his repository, modbus is a standard ESPHome component.

<img width="484" height="355" alt="PCB-Picture" src="https://github.com/user-attachments/assets/eccf1b4d-2b50-4b09-8e9c-263c05c333b7" />


Links
-----

Datasheets and documents :

- https://fccid.io/2BCBK-ZX4D30CE08S
- https://img03.71360.com/w3/4e1e0o/20240731/32602985e837b5826126e4c7d6cd2b52.pdf
- https://en.wireless-tag.com/product-item-49.html
- https://techinfodepot.shoutwiki.com/wiki/PanelLan_SC07
- https://github.com/lovyan03/LovyanGFX
- https://components.espressif.com/components/espressif/esp_lcd_panel_io_additions/versions/1.0.1~1/readme
- https://github.com/syssi/esphome-jk-bms
- https://doc.panel-tag.cn/ESP32-S3/index.html
- https://github.com/smartpanle/QMSD-ESP32-BSP
- https://gitee.com/qiming-zhixian/qmsd-esp32-bsp
- https://admin.osptek.com/uploads/NV_3041_A_Datasheet_V1_2_20221011_686486a221.pdf
- https://www.buydisplay.com/download/ic/CST3240_Application_Manual.pdf
- https://www.buydisplay.com/download/ic/CST3240_Datasheet.pdf

