const express = require('express');
const session = require('express-session');
const path = require('path');

const app = express();
const port = 3000;

let alarmStatus = 'Disarmed'; // 🔒 shared system state

// Sessions
app.use(session({
  secret: 'secret-key',
  resave: false,
  saveUninitialized: true
}));

app.use(express.urlencoded({ extended: true }));
app.use(express.json());

// Auth middleware
app.use((req, res, next) => {
  const openRoutes = ['/', '/login', '/login.html'];
  const isPublicAsset = req.path.endsWith('.css') || req.path.endsWith('.js') || req.path.endsWith('.png') || req.path.endsWith('.ico');

  if (openRoutes.includes(req.path) || isPublicAsset || req.session.loggedIn) {
    return next();
  } else {
    return res.redirect('/login.html');
  }
});

// Serve frontend
app.use(express.static(path.join(__dirname, 'public')));

// Login
app.get('/login', (req, res) => {
  res.sendFile(path.join(__dirname, 'public', 'login.html'));
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

// Logout
app.get('/logout', (req, res) => {
  req.session.destroy(() => {
    res.redirect('/login.html');
  });
});

// GET current alarm status
app.get('/status', (req, res) => {
  res.json({ status: alarmStatus });
});

// Arm system
app.post('/arm', (req, res) => {
  alarmStatus = 'Armed';
  res.send("System successfully armed.");
});

// Disarm system
app.post('/disarm', (req, res) => {
  alarmStatus = 'Disarmed';
  res.send("System successfully disarmed.");
});

app.listen(port, () => {
  console.log(`✅ Server running at http://localhost:${port}`);
});
