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

document.getElementById("confirmBtn").addEventListener("click", () => {
  const pin = pinInput.value.trim();
  if (pin.length !== 6 || !/^\d{6}$/.test(pin)) {
    pinError.textContent = "PIN must be exactly 6 digits.";
    return;
  }

  if (pin === correctPin) {
    if (pendingAction === "arm") {
      statusEl.textContent = "System is Armed";
      statusEl.style.backgroundColor = "#660000";
      feedbackEl.textContent = "System successfully armed.";
    } else {
      statusEl.textContent = "System is Disarmed";
      statusEl.style.backgroundColor = "#004400";
      feedbackEl.textContent = "System successfully disarmed.";
    }
    closePinModal();
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
