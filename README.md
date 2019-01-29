# Oregon Scientific sensor logger

An Arduino sketch to gather temperature and humidy from an old [Oregon Scientific](http://global.oregonscientific.com/) sensor.

These sensors use the [Manchester code](https://en.wikipedia.org/wiki/Manchester_code) to transmit data to weather stations. This kind of data encoding was and still is widely used.

## Parts
- Arduino [Nano](https://store.arduino.cc/arduino-nano) or [Uno](https://store.arduino.cc/arduino-uno-rev3), or any other cheap clone
- 433Mhz receiver like this one [Amazon](https://www.amazon.fr/WINGONEER-433Mhz-Superheterodyne-récepteur-Arduino/dp/B06XHJMC82) [AliExpress](https://fr.aliexpress.com/item/NEW-RXB6-433Mhz-Superheterodyne-Wireless-Receiver-Module-for-Arduino-ARM-AVR/32808930551.html) (RXB6 superheterodyne)

## Library and software
- [Arduino-Oregon-Library](https://github.com/Mickaelh51/Arduino-Oregon-Library) : library used to decode the radio signal
- [PlatformIO](https://platformio.org) or [Arduino IDE](https://www.arduino.cc/en/main/software)
- [Fritzing part](http://forum.fritzing.org/t/diode-keine-led-bauform-0805-gesucht/2216/34) for the RXB6 module

## Sensors and messages

 * Oregon RTGR328N
    - `3CCx` (with _x_=_9_,_A_,_B_,_C_,_D_) : Outside Temp-Hygro (known as `0xACC` in RFLink plugins).
    - `3EA8` or `3EC8` : Date & Time (`0xAEC` or `0xAEA`)
 * Oregon THGR228N or equivalent
    - `02D1` : Outside Temp-Hygro  (`0x1A2D`)

See [decode.h](include/decode.h) or below for details.

I guess that some authors haven't noticed that the first nibble `A` is not a part of the message, or just ignore this fact for convenience.

*Nota*

Bytes are received in 2 nibbles, LSB first, then MSB.

In lack of official documentation or clear and verified sources, I cannot know how Oregon Scientific sensors and stations really identify messages: 2 nibbles (`0` and `2`) or 4 nibbles, and with which order (`0x12D0` or `0x02D1`) ? That suppositions... I prefer the last one, because BCD numbers are reversed, the checksum is. So the ID should be too.

### Message with temperature and humidity

Desc | Content
---- | -------
Bytes received | `DA CC 43 D9 16 08 80 83 64 A0`
Nibbles | `A DCC3 4 9D 6 1800 83 8 46 0A`
Decoded | `channel=4 temp=8.1°C hum=38% bat=low`

Nibble | Value
------ | -----
   0   | always 0xA (1010 in binary), kind of sync or preamble
  4-1  | message ID
   5   | channel
  7-6  | rolling code : a code to identify the sensor, change after many resets
   8   | battery state, bit 3 seems to indicate a low level battery
  12-9 | temperature in BCD, value in Celsius degrees
 14-13 | percent of humidity
   15  | unknown
 17-16 | checksum (nibbles 1 to 15)
 19-18 | unknown trailing, not always present or misunderstanding of protocol

### Message with date time


Desc | Content
---- | -------
Bytes received | `8AEC43D97644318212917177CA`
Nibbles | `A 8CE3 4 9D 6 74 41 32 82 1 1 91 7 77 AC`
Decoded | `2019/01/28 23:14:47`

Nibble | Value
------ | -----
   0   | always 0xA (1010 in binary), kind of sync or preamble
  4-1  | message ID
   5   | channel
  7-6  | rolling code : a code to identify the sensor, change after many resets
   8   | probably sync state, bit 1 (value 2) indicates a valid date
  14-9 | HH:MM:SS in BCD
 16-15 | day (1-31)
   17  | month (1-12)
   18  | day of week (0=sunday)
 20-10 | year
   21  | unknown
 23-22 | checksum (nibbles 1 to 21)
 25-24 | unknown trailing, not always present or misunderstanding of protocol

## Wiring

![breadboard](oregon_bb.png)

## Links

* [RFLink](http://www.rflink.nl/blog2/) closed source RF gateway
* [RFLink](https://github.com/cwesystems/RFLink) fork of rflink when it was open source (there are may other forks on GitHub).
* [Oregon Scientific RF Protocol Description](http://wmrx00.sourceforge.net/Arduino/OregonScientific-RF-Protocols.pdf) :
* [Decoding the Oregon Scientific V2 protocol](https://jeelabs.net/projects/cafe/wiki/Decoding_the_Oregon_Scientific_V2_protocol)
* [Oregon Scientific sensors with Raspberry PI](https://www.disk91.com/2013/technology/hardware/oregon-scientific-sensors-with-raspberry-pi/)
* and so on (use google)
