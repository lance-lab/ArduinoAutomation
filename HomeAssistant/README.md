# Home Assistant Stack

This folder runs three Docker containers:

- Home Assistant
- Mosquitto
- Cloudflare tunnel

## 1. Prepare Folders

From this folder:

```bash
cd /home/lance/Repositories/ArduinoAutomation/HomeAssistant
```

Create the runtime folders:

```bash
sudo mkdir -p /opt/homeassistant
sudo mkdir -p /opt/mosquitto/config /opt/mosquitto/data /opt/mosquitto/log
```

Copy the Home Assistant config:

```bash
sudo cp -r config/* /opt/homeassistant/
```

Copy the Mosquitto config:

```bash
sudo cp -r mosquitto/config/* /opt/mosquitto/config/
```

To generate or update the Mosquitto password file with Docker:

```bash
cd /home/lance/Repositories/ArduinoAutomation/HomeAssistant/mosquitto/config
docker run --rm -it \
  -v "$PWD":/mosquitto/config \
  eclipse-mosquitto:2 \
  mosquitto_passwd /mosquitto/config/password.txt lancehome
```

To create a brand-new password file instead, add `-c`:

```bash
docker run --rm -it \
  -v "$PWD":/mosquitto/config \
  eclipse-mosquitto:2 \
  mosquitto_passwd -c /mosquitto/config/password.txt lancehome
```

After changing `password.txt`, copy it again:

```bash
sudo cp password.txt /opt/mosquitto/config/password.txt
```

## 2. Create Docker Network

```bash
docker network create ha-net
```

If it already exists, continue.

## 3. Configure Cloudflare Token

Create `.env` in this folder:

```bash
nano .env
```

Add:

```env
CLOUDFLARED_TOKEN=your_cloudflare_tunnel_token
```

In Cloudflare Zero Trust, the tunnel service should point to:

```text
http://homeassistant:8123
```

## 4. Stop Native Mosquitto

Only Docker Mosquitto should use port `1883`.

```bash
sudo systemctl disable --now mosquitto.service
sudo systemctl disable --now arduinoautomation-mosquitto.service
```

Check:

```bash
sudo ss -ltnp | grep ':1883'
```

No output is OK before Docker starts. After Docker starts, it should show `docker-proxy`.

## 5. Start Docker Compose

```bash
docker compose up -d
```

Check:

```bash
docker compose ps
```

## 6. Configure Home Assistant MQTT

Open Home Assistant:

```text
http://<server-ip>:8123
```

MQTT settings:

```text
Broker: mosquitto
Port: 1883
Username: lancehome
Password: same password from mosquitto/config/password.txt
```

Use `mosquitto` for MQTT in this Compose setup because Home Assistant and Mosquitto are on the same Docker network.

## 7. Arduino MQTT Settings

Arduino/Controllino modules should use the server LAN IP:

```text
Broker: <server-ip>
Port: 1883
Username: lancehome
Password: same MQTT password
```

## Useful Checks

```bash
docker logs homeassistant
docker logs mosquitto
docker logs cloudflared
```

```bash
docker exec homeassistant nc -zv mosquitto 1883
docker exec cloudflared getent hosts homeassistant
```

```bash
sudo ss -ltnp | grep ':1883'
```
