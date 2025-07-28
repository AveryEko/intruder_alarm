const express = require('express');
const session = require('express-session');
const path = require('path');
const fetch = require('node-fetch'); // install using: npm i node-fetch@2

const app = express();
const port = 3000;

// Firebase Realtime Database
const FIREBASE_URL = "https://your-project-id.firebaseio.com/intruderStatus.json";
const FIREBASE_SECRET = "YOUR_DATABASE_SECRET";

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

// 👉 POST /arm or /disarm
app.post('/arm', async (req, res) => {
  await updateStatus("Armed");
  res.send("System armed via web.");
});

app.post('/disarm', async (req, res) => {
  await updateStatus("Disarmed");
  res.send("System disarmed via web.");
});

// 👉 GET /status
app.get('/status', async (req, res) => {
  try {
    const r = await fetch(`${FIREBASE_URL}?auth=${FIREBASE_SECRET}`);
    const json = await r.json();
    res.json({ status: json.status || "Unknown" });
  } catch {
    res.status(500).json({ status: "Error" });
  }
});

// 🔄 Update Firebase
async function updateStatus(status) {
  await fetch(`${FIREBASE_URL}?auth=${FIREBASE_SECRET}`, {
    method: "PUT",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ status })
  });
}

app.listen(port, () => {
  console.log(`✅ Server running at http://localhost:${port}`);
});
