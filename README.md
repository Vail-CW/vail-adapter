---
Author: neale(original author) and Brett KE9BOS (Current Author)
Email: ke9bos@pigletradio.org
License: MIT
---

# Vail Adapter: Morse Code Key/Paddle to USB

![Vail adapter, assembled and connected](doc/vail-adapter-v2.jpg)


# Features

* Lets you key even if you move focus to another window
* Works with [Vail](https://vail.woozle.org/)
* Works with [VBand](https://hamradio.solutions/vband/), but the window has to remain focused
* Optional sidetone generator, which helps with latency
* Implements all nine keyer modes from Vail, in the adapter, so you lunatics can try to key at 50WPM with no latency issues
* Plays received signals in the adapter, so you can turn off your computer speaker
* Free firmware updates for life
* Can be wired up in about 5 minutes

[Vail Adapter benefits video](https://www.youtube.com/watch?v=XQ-mwdyLkOY) (4:46)

# Bill of Materials for PCB version
* Seeed Studio XIAO SAMD21 (Available from [Amazon](https://www.amazon.com/gp/product/B08CN5YSQF?smid=A2OY3Y9CEYQQ5W) or [Adafruit QT Py](https://www.adafruit.com/product/4600))
* PCB can be ordered from (JLCPCB or PCBWAY) yourself or from Brett KE9BOS as a bare PCB, kit with all parts needed, or assembled - email ke9bos@pigletradio.org
* Buzzer Speaker
* You need one of each of the below for V1.1 pcb(purple), or only the first one if you have a v1 pcb(black):
  * [PCB Mount TRS connector](https://a.co/d/bLaRwym)
  * [Switched PCB Mount Aux Jack](https://www.amazon.com/dp/B07WR748JS)
 
# Bill of Materials if you don't want to use a PCB
* Seeed Studio XIAO SAMD21 (Available from [Amazon](https://www.amazon.com/gp/product/B08CN5YSQF?smid=A2OY3Y9CEYQQ5W) or [Adafruit QT Py](https://www.adafruit.com/product/4600))
* Buzzer Speaker
* [Panel Aux Jack](https://www.amazon.com/dp/B01C3RFHDC)

# Setting Up

* [Easy Setup](doc/easy-install.md)
* [Advanced Setup](doc/advanced-install.md)


# Future Work

Things I plan to add:

* [x] PCB to ease assembly and make a more robust shippable product
* [ ] Advanced PCB with capacitive touch built into pcb and enclosure, as well as radio passthrough
* [ ] Wireless Vail Adapter


# Contributing
To contribute to this project please contact ke9bos@pigletradio.org


