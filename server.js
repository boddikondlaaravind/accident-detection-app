/**
 * RapidAid IoT Relay Server
 * ──────────────────────────
 * A tiny bridge between physical hardware (ESP32, Arduino, a panic
 * button, etc.) and the RapidAid webpage. The webpage can't accept an
 * incoming connection from your device directly, so your device POSTs
 * here, and the webpage polls here.
 *
 * Routes:
 *   POST /trigger?device=<userId>&key=<deviceKey>   — hardware calls this
 *   GET  /status?device=<userId>&key=<deviceKey>    — webpage polls this
 *   POST /ack?device=<userId>&key=<deviceKey>       — webpage calls this
 *                                                      after it fires SOS,
 *                                                      so the flag clears
 *
 * <userId> and <deviceKey> come from the RapidAid app's
 * Profile → 📡 IoT Device screen — copy them into your device's code.
 *
 * DEPLOY (pick one, all have free tiers):
 *   Render.com   → New Web Service → connect this folder → Node
 *   Railway.app  → New Project → Deploy from folder
 *   Glitch.com   → Import this folder, it starts automatically
 *   Or run locally for testing:  npm install && node server.js
 *
 * Once deployed, copy the server's public URL (e.g.
 * https://rapidaid-relay.onrender.com) into the RapidAid webpage's
 * IOT_ENDPOINT constant near the top of the <script> section.
 */

const express = require('express');
const app = express();
app.use(express.json());

// In-memory store: { "userId:key": { triggered: bool, lastSeen: number } }
const devices = {};

function keyFor(req) {
  const { device, key } = req.query;
  if (!device || !key) return null;
  return `${device}:${key}`;
}

// Hardware calls this when it detects a crash / panic press
app.post('/trigger', (req, res) => {
  const k = keyFor(req);
  if (!k) return res.status(400).json({ error: 'missing device or key' });
  devices[k] = { triggered: true, lastSeen: Date.now() };
  console.log(`🚨 Trigger received from ${req.query.device}`);
  res.json({ ok: true });
});

// Webpage polls this every few seconds
app.get('/status', (req, res) => {
  const k = keyFor(req);
  if (!k) return res.status(400).json({ error: 'missing device or key' });
  const entry = devices[k];
  res.json({ triggered: !!(entry && entry.triggered) });
});

// Webpage calls this after it has fired the SOS, to reset the flag
app.post('/ack', (req, res) => {
  const k = keyFor(req);
  if (!k) return res.status(400).json({ error: 'missing device or key' });
  if (devices[k]) devices[k].triggered = false;
  res.json({ ok: true });
});

// Simple health check
app.get('/', (req, res) => res.send('RapidAid IoT relay is running.'));

const PORT = process.env.PORT || 3000;
app.listen(PORT, () => console.log(`RapidAid relay listening on port ${PORT}`));
