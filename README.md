# ArduinoAutomation

This repository has two main parts:

- `HomeAssistant/` - Docker setup for Home Assistant, Mosquitto, and Cloudflare tunnel
- `LanceHome/` - Arduino/Controllino firmware, libraries, and module configuration

Start with the nested README files:

- [HomeAssistant setup](/home/lance/Repositories/ArduinoAutomation/HomeAssistant/README.md)
- [LanceHome setup](/home/lance/Repositories/ArduinoAutomation/LanceHome/README.md)

Recommended order:

1. Set up Home Assistant, Mosquitto, and Cloudflare from `HomeAssistant/README.md`.
2. Configure the Arduino/Controllino modules from `LanceHome/README.md`.
3. Make sure the MQTT broker IP, username, password, and topics match between both sides.

The root README is only an index. Keep detailed installation steps in the folder-specific README files.
