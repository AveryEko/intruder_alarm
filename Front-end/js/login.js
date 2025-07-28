const users = [
  { username: "admin", password: "1234" },
  { username: "user1", password: "password" }
];

document.getElementById("loginForm").addEventListener("submit", function (e) {
  e.preventDefault();

  const username = document.getElementById("username").value.trim();
  const password = document.getElementById("password").value.trim();
  const message = document.getElementById("message");

  const user = users.find(u => u.username === username && u.password === password);

  if (user) {
    message.style.color = "lime";
    message.textContent = "Login successful!";
    setTimeout(() => {
      window.location.href = "UI.html"; //redirect to UI page
    }, 1000);
  } else {
    message.style.color = "red";
    message.textContent = "Invalid username or password";
  }
});

// Toggle password visibility
function togglePassword() {
  const passwordInput = document.getElementById("password");
  const type = passwordInput.getAttribute("type") === "password" ? "text" : "password";
  passwordInput.setAttribute("type", type);
}



