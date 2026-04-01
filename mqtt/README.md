# Local MQTT Broker

This repository now includes a local Mosquitto broker configuration for the Arduino modules.

## Broker settings

- Host: this machine's LAN IP address
- Port: `1883`
- Username: `lancehome`
- Password: `611632787d524d3f154f88e1`

## Start the broker

From the repository root:

```bash
./mqtt/start-local.sh
```

The broker listens on all interfaces on port `1883`.

## Update the Arduino EEPROM credentials

Edit `LanceHome/SetCredentials/SetCredentials.ino`:

```cpp
strncpy(cred.mqttUser, "lancehome", CRED_USERNAME_MAX - 1);
strncpy(cred.mqttPass, "611632787d524d3f154f88e1", CRED_PASSWORD_MAX - 1);
strncpy(cred.mqttServer, "YOUR_PC_LAN_IP", CRED_SERVER_MAX - 1);
strncpy(cred.mqttClientId, "LanceModule1", CRED_CLIENT_ID_MAX - 1);
```

Use the LAN IP of the computer running Mosquitto, not `127.0.0.1`.

## Quick test

In one terminal:

```bash
mosquitto_sub -h 127.0.0.1 -p 1883 -u lancehome -P 611632787d524d3f154f88e1 -t 'LanceControllino1/#' -v
```

In another terminal:

```bash
mosquitto_pub -h 127.0.0.1 -p 1883 -u lancehome -P 611632787d524d3f154f88e1 -t 'LanceControllino1/out' -m 'test'
```

## Run Automatically On Fedora Boot

This repo includes a `systemd` service file at `mqtt/arduinoautomation-mosquitto.service`.

Install and enable it:

```bash
sudo cp /home/lance/Repositories/ArduinoAutomation/mqtt/arduinoautomation-mosquitto.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now arduinoautomation-mosquitto.service
```

Check status:

```bash
sudo systemctl status arduinoautomation-mosquitto.service
sudo journalctl -u arduinoautomation-mosquitto.service -f
```

Notes:

- The service runs as user `lance`.
- It starts `/home/lance/Repositories/ArduinoAutomation/mqtt/start-local.sh`.
- If your repo path or Linux username changes, update the service file before copying it into `/etc/systemd/system/`.
