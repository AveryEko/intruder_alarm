const correctPin = "123456";
let pendingAction = null;

const statusEl = document.getElementById("status");
const feedbackEl = document.getElementById("feedback");
const pinModal = document.getElementById("pinModal");
const pinInput = document.getElementById("pinInput");
const pinError = document.getElementById("pinError");

document.getElementById("armBtn").addEventListener("click", () => {
  pendingAction = "arm";
  openPinModal();
});

document.getElementById("disarmBtn").addEventListener("click", () => {
  pendingAction = "disarm";
  openPinModal();
});

document.getElementById("cancelBtn").addEventListener("click", () => {
  closePinModal();
  feedbackEl.textContent = "Action cancelled.";
});

document.getElementById("confirmBtn").addEventListener("click", async () => {
  const pin = pinInput.value.trim();
  if (pin.length !== 6 || !/^\d{6}$/.test(pin)) {
    pinError.textContent = "PIN must be exactly 6 digits.";
    return;
  }

  if (pin === correctPin) {
    try {
      const res = await fetch(`/${pendingAction}`, { method: "POST" });
      const message = await res.text();
      feedbackEl.textContent = message;
      closePinModal();
      fetchStatus();
    } catch (err) {
      feedbackEl.textContent = "Failed to send command.";
    }
  } else {
    pinError.textContent = "Incorrect PIN. Try again.";
  }
});

function openPinModal() {
  pinModal.style.display = "flex";
  pinInput.value = "";
  pinError.textContent = "";
  pinInput.focus();
}

function closePinModal() {
  pinModal.style.display = "none";
}

// Fetch current status from server
async function fetchStatus() {
  try {
    const res = await fetch("/status");
    const data = await res.json();
    statusEl.textContent = "System is " + data.status;
    statusEl.style.backgroundColor = data.status === "Armed" ? "#660000" :
                                     data.status === "Disarmed" ? "#004400" :
                                     "#cc0000";
  } catch {
    statusEl.textContent = "Unable to fetch system status.";
  }
}


fetchStatus();
setInterval(fetchStatus, 5000); // Refresh status every 5s

function openCameraModal() {
  const modal = document.getElementById("cameraModal");
  const iframe = document.getElementById("cameraFeed");
  iframe.src = "http://<ESP32-CAM-IP>"; // 🔁 Replace with actual ESP32-CAM IP
  modal.style.display = "flex";
}

function closeCameraModal() {
  const modal = document.getElementById("cameraModal");
  const iframe = document.getElementById("cameraFeed");
  iframe.src = ""; // Stop feed
  modal.style.display = "none";
}
