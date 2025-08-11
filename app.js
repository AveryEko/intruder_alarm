const express = require('express');
const session = require('express-session');
const path = require('path');

const app = express();
const port = 3000;

let alarmStatus = 'Disarmed';

// Sessions
app.use(session({
  secret: 'secret-key',
  resave: false,
  saveUninitialized: true
}));

app.use(express.urlencoded({ extended: true }));
app.use(express.json());

// Serve static files from public directory
app.use(express.static(path.join(__dirname, 'public')));

// Auth middleware
app.use((req, res, next) => {
  const openRoutes = ['/', '/login', '/login.html'];
  const isPublicAsset = req.path.startsWith('/css') || 
                       req.path.startsWith('/js') || 
                       req.path.endsWith('.png') || 
                       req.path.endsWith('.ico') ||
                       req.path.endsWith('.jpg') ||
                       req.path.endsWith('.jpeg');

  if (openRoutes.includes(req.path) || isPublicAsset || req.session.loggedIn) {
    return next();
  } else {
    return res.redirect('/login.html');
  }
});

// Root route - redirect to login
app.get('/', (req, res) => {
  if (req.session.loggedIn) {
    res.redirect('/UI.html');
  } else {
    res.redirect('/login.html');
  }
});

// Login routes
app.get('/login', (req, res) => {
  res.sendFile(path.join(__dirname, 'public', 'login.html'));
});

app.get('/login.html', (req, res) => {
  res.sendFile(path.join(__dirname, 'public', 'login.html'));
});

// Main UI page
app.get('/UI.html', (req, res) => {
  if (req.session.loggedIn) {
    res.sendFile(path.join(__dirname, 'public', 'UI.html'));
  } else {
    res.redirect('/login.html');
  }
});

// Login POST
app.post('/login', (req, res) => {
  const { username, password } = req.body;
  if (username === 'admin' && password === '1234') {
    req.session.loggedIn = true;
    res.redirect('/UI.html');
  } else {
    res.send('Login failed. <a href="/login.html">Try again</a>');
  }
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

// API Routes for alarm system
app.get('/status', (req, res) => {
  res.json({ status: alarmStatus });
});

app.post('/arm', (req, res) => {
  alarmStatus = 'Armed';
  res.json({ success: true, message: "System successfully armed.", status: alarmStatus });
});

app.post('/disarm', (req, res) => {
  alarmStatus = 'Disarmed';
  res.json({ success: true, message: "System successfully disarmed.", status: alarmStatus });
});

app.post('/verify-pin', (req, res) => {
  const { pin } = req.body;
  const correctPin = '123456';
  
  if (pin === correctPin) {
    res.json({ success: true });
  } else {
    res.json({ success: false, message: 'Invalid PIN' });
  }
});

// Start server
app.listen(port, () => {
  console.log(`🚀 Intruder Alarm System running at http://localhost:${port}`);
  console.log(`📱 Access the interface: http://localhost:${port}/UI.html`);
  console.log(`🔑 Login: admin / 1234`);
});