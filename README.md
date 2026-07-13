# RapidAid IoT Relay

Bridges standalone hardware (ESP32 crash sensor, panic button, etc.) to the
RapidAid webpage, so a physical device — not just a phone tap — can fire
someone's SOS.

## Why this exists

The RapidAid webpage is a static page with no server of its own. A physical
device on the other side of the internet has no way to "call into" a
browser directly. This tiny server sits in between:

```
ESP32 (detects crash) → POST /trigger → this relay → RapidAid webpage polls /status → fires SOS
```

## 1. Deploy the relay (pick one — all free)

**Render.com** (easiest)
1. Push this folder to a GitHub repo (or use Render's "public Git repo" import).
2. Render → New → Web Service → connect the repo.
3. Build command: `npm install` · Start command: `npm start`
4. Deploy. Copy the resulting URL, e.g. `https://rapidaid-relay.onrender.com`

**Railway.app** or **Glitch.com** work the same way — connect this folder,
it'll detect `package.json` and run `npm start` automatically.

**Local testing** (before deploying anywhere):
```
npm install
node server.js
```
It listens on `http://localhost:3000`.

## 2. Point the webpage at your relay

Open `rapidaid.html`, find this line near the top of the `<script>` section:
```js
const IOT_ENDPOINT = "";
```
Set it to your deployed relay's URL:
```js
const IOT_ENDPOINT = "https://rapidaid-relay.onrender.com";
```

## 3. Get your device ID + key

In the RapidAid app: **Profile → 📡 IoT Device → Copy Device Link**.
The copied link looks like:
```
https://yoursite.com/rapidaid.html?device_trigger=user_1234567890&key=a1b2c3d4
```
The two values after `device_trigger=` and `key=` are what your hardware needs.

## 4. Flash your hardware

Open `esp32-crash-detector.ino` in the Arduino IDE, fill in:
- Your WiFi name/password
- Your relay's URL (from step 1)
- Your device ID + key (from step 3)

It supports two trigger sources out of the box: a manual panic button, and
automatic crash detection via an MPU6050 accelerometer (impact threshold is
tunable — start conservative and test).

Don't have that exact hardware? Any device that can make an HTTP POST to
```
POST <your-relay-url>/trigger?device=<id>&key=<key>
```
will work the same way — the .ino file is just one example.

## How the flow works end to end

1. Hardware detects a crash / button press → POSTs to `/trigger`
2. Relay marks that device as `triggered: true`
3. The webpage (if that user is logged in on it) polls `/status` every 5s and sees it
4. Webpage fires SOS automatically, then POSTs `/ack` to clear the flag
5. Relay resets to `triggered: false`, ready for the next event

## Notes

- This relay stores state in memory only — if it restarts, pending
  triggers are lost (fine for this use case; hardware will just re-trigger
  on the next event).
- Add HTTPS (Render/Railway give you this for free automatically) — never
  run this over plain HTTP for a safety device.
- Want multiple devices per person, or persistent storage? This is a
  deliberately minimal starting point — happy to extend it.
