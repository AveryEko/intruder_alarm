// Node.js server to receive alerts from Arduino (ESP-01)
// Install Express first: npm install express

const express = require('express');
const app = express();
const port = 3000; // Use 80 if you want it public without a port number

// CORS Middleware (for allowing frontend access if needed)
app.use((req, res, next) => {
  res.setHeader('Access-Control-Allow-Origin', '*');
  next();
});

// Simple GET endpoint that Arduino will hit on intruder detection
app.get('/alert', (req, res) => {
  console.log('⚠️ Intruder alert received from Arduino!');
  // You can save to a file, log, or trigger further actions
  res.send('Alert received');
});

// Optional: Frontend can fetch this to get latest status
let lastAlertTime = null;
app.get('/status', (req, res) => {
  res.json({ lastAlert: lastAlertTime });
});

// Start server
app.listen(port, () => {
  console.log(`✅ Alert server running at http://localhost:${port}`);
});
