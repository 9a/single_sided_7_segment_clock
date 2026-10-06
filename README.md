# Single Sided 7 segment clock
A pcb design for a 7 segment clock for single sided milled pcbs. Designed with KiCad 10. 

BOM (alternatives are also OK):
- 2x SN74HC595DR 
- 29x 150080RS75000
- 8x 10 Ohm 0805 (or 0603)

See schematic/placement picture for assembly. The other through holes need to be connected via jumper wires.

It is designed to be connected to a USB-C ESP8266 D1-mini with a one to one connection via jumper wires to the pins 5V GND D4 D3 D2. Since the pins expose the control pins of the shift registers any microcontroller can be adapted.

For use with ESP home in Homeassistant:
The provided esp_yaml.txt can be copied when creating the device, the cnc_display.h needs to be copied into the esphome directory.

![PCB](PXL_20260924_182005619.mp4_snapshot_00.08.969.jpg)

![finished_clock](PXL_20260914_172004473.PORTRAIT.jpg)
