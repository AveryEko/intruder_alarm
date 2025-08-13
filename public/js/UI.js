const express = require('express');
const session = require('express-session');
const path = require('path');
const fetch = require('node-fetch'); // install using: npm i node-fetch@2

const app = express();
const port = 3000;

// PHP Backend URLs
const PHP_STATUS_URL = "https://fiot-intruder-alarm.infinityfree.me/server/status.php";
const PHP_ALERTS_URL = "https://fiot-intruder-alarm.infinityfree.me/server/alerts.php"; // optional
const PHP_SNAPSHOTS_URL = "https://fiot-intruder-alarm.infinityfree.me/server/snapshots.php"; // optional

// Sessions
app.use(session({
  secret: 'secret-key',
  resave: false,
  saveUninitialized: true
}));

app.use(express.urlencoded({ extended: true }));
app.use(express.json());

// Authentication middleware
app.use((req, res, next) => {
  const openRoutes = ['/', '/login', '/login.html'];
  const isAsset = req.path.endsWith('.css') || req.path.endsWith('.js') || req.path.endsWith('.png');
  if (openRoutes.includes(req.path) || isAsset || req.session.loggedIn) {
    return next();
  } else {
    return res.redirect('/login.html');
  }
});

// Static files
app.use(express.static(path.join(__dirname, 'public')));

// Routes
app.get('/login', (req, res) => {
  res.sendFile(path.join(__dirname, 'public/login.html'));
});

app.post('/login.html', (req, res) => {
  const { username, password } = req.body;
  if (username === 'admin' && password === '1234') {
    req.session.loggedIn = true;
    res.redirect('/UI.html');
  } else {
    res.send('Login failed. <a href="/login.html">Try again</a>');
  }
});

app.get('/logout', (req, res) => {
  req.session.destroy(() => {
    res.redirect('/login.html');
  });
});

// 👉 POST /arm-esp32 or /disarm-esp32
app.post('/arm-esp32', async (req, res) => {
  await updateBackendStatus({ esp32Armed: true });
  res.send("ESP32 system armed via web.");
});

app.post('/disarm-esp32', async (req, res) => {
  await updateBackendStatus({ esp32Armed: false });
  res.send("ESP32 system disarmed via web.");
});

// 👉 POST /arm-arduino or /disarm-arduino
app.post('/arm-arduino', async (req, res) => {
  await updateBackendStatus({ arduinoArmed: true });
  res.send("Arduino Uno system armed via web.");
});

app.post('/disarm-arduino', async (req, res) => {
  await updateBackendStatus({ arduinoArmed: false });
  res.send("Arduino Uno system disarmed via web.");
});

// 👉 GET /status (returns both ESP32 and Arduino status)
app.get('/status', async (req, res) => {
  try {
    const r = await fetch(PHP_STATUS_URL);
    const json = await r.json();
    res.json({
      esp32Armed: json.esp32Armed,
      arduinoArmed: json.arduinoArmed,
      esp32Status: json.esp32Status || "Unknown",
      arduinoStatus: json.arduinoStatus || "Unknown"
    });
  } catch {
    res.status(500).json({ esp32Armed: false, arduinoArmed: false, esp32Status: "Error", arduinoStatus: "Error" });
  }
});

// 👉 GET /alerts (optional)
app.get('/alerts', async (req, res) => {
  try {
    const r = await fetch(PHP_ALERTS_URL);
    const json = await r.json();
    res.json(json);
  } catch {
    res.status(500).json({ type: "none" });
  }
});

// 👉 GET /snapshots (optional)
app.get('/snapshots', async (req, res) => {
  try {
    const r = await fetch(PHP_SNAPSHOTS_URL);
    const json = await r.json();
    res.json(json);
  } catch {
    res.status(500).json([]);
  }
});

// 🔄 Update PHP backend status
async function updateBackendStatus(updates) {
  // updates = {esp32Armed: true/false, arduinoArmed: true/false}
  const params = new URLSearchParams();
  if ('esp32Armed' in updates) params.append('esp32Armed', updates.esp32Armed);
  if ('arduinoArmed' in updates) params.append('arduinoArmed', updates.arduinoArmed);

  await fetch(PHP_STATUS_URL, {
    method: "POST",
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body: params.toString()
  });
}

app.listen(port, () => {
  console.log(`✅ Server running at http://localhost:${port}`);
});