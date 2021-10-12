# Arduino-GeishaBridge

Communication to and from Geisha (Panasonic WH-MDC05f3e5) using a WIFI Arduino esp8266 controller.
The controller will hack the serial communication between remote and heatpump, using this PCB 
(thanks to all who contributed):

https://github.com/pemue-git/pcb/tree/master/WH-MDC05F3E5_comm

Inspired by 
https://github.com/v-s-c-o-p-e/geisha_aquarea_panasonic_arduino_esp8266_proxy
and 
https://github.com/der-lolo/aquarea

The controller will read the values documented so far from the heatpump and publish these to
some MQTT server running in your network (in my case, it's an openhab installation)
I am using a homie library for esp8266:

https://github.com/homieiot/homie-esp8266

Also, new values from MQTT will be accepted und sent to the heatpump.
In my case, I use some Openhab-Things and Items to display and manipulate the geisha values. 
Openhab sends them to the controller via MQTT.

Use at own risk, you need to cut the wire between heatpump and remote. 
I cannot provide PCBs. These are simple and can manually be soldered on plain cards (see pics).

For me, it works also in case there is an Intesis-Home device installed. My cable setup is 
Remote -> IntesisHome -> PCB -> Heatpump. Haven't tried it yet without the Intesis Box. 

The timing on the serial bus is merely a guess. I do not have a logic analyzer. Sometimes a serial packet gets dropped, but 
that didn't do any bad up to now.

To get started, install the homie-esp8266 libs in your Arduino environment
and follow the docs, ie. https://homieiot.github.io/homie-esp8266/docs/stable/

The device can be configured like so: https://homieiot.github.io/homie-esp8266/docs/stable/configuration/json-configuration-file/

There is a JSON file which contains passwords, hostnames,... in "../src/data/homie/config.json". This file can be uploaded to the controller using this tool: https://github.com/esp8266/arduino-esp8266fs-plugin

The code is plain C, but because asychronous libs (homie, wifi and mqtt) are involved, it's not always straight forward.

I used a cheapo nodeMcu board (see pic) and installed the following board manager in arduino GUI:
ESP8266 Boards -> "NodeMCU 1.0 (ESP 12E Module)"

The provided binaries will obviously only work if you manage to upload a config.json file with your hostnames, passwords.... to your board.
