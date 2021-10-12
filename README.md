# Arduino-GeishaBridge

Communication to Geisha (Panasonic WH-MDC05f3e5) vis WIFO Arduino esp8266 Controller.
The Controller will hack the serial Communication between Remote and Heatpump, using this PCB:

https://github.com/pemue-git/pcb/tree/master/WH-MDC05F3E5_comm

Inspired by 
https://github.com/v-s-c-o-p-e/geisha_aquarea_panasonic_arduino_esp8266_proxy
and 
https://github.com/der-lolo/aquarea

The Controller will read the Values documented so far from the heatpump and publish these values to
a MQTT Server using the homie convention. 

https://github.com/homieiot/homie-esp8266

Also, new values from MQTT will be accepted und sent to the geisha heatpump.
In my case, I use Openhab to display and manipulate the geisha values via MQTT.

Use at own risk. I cannot provide PCBs. These are simple an can manually soldered on a prototype-like PCB.

