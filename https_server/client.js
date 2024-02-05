// Create a WebSocket object that connects to the server
var socket = new WebSocket("wss://localhost:443");

// Get the chatbox and message form elements by their id
var chatbox = document.getElementById("chatbox");
var messageform = document.getElementById("messageform");

// Add an event listener for the open event
socket.addEventListener("open", function(event) {
    // Display a message to the user
    chatbox.innerHTML += "<p>Connected to the server</p>";
});

// Add an event listener for the message event
socket.addEventListener("message", function(event) {
    // Append the message to the chatbox
    chatbox.innerHTML += "<p>" + event.data + "</p>";
});

// Add an event listener for the close event
socket.addEventListener("close", function(event) {
    // Display a message to the user
    chatbox.innerHTML += "<p>Disconnected from the server</p>";
});

// Add an event listener for the submit event
messageform.addEventListener("submit", function(event) {
    // Prevent the default behavior of the form
    event.preventDefault();
    // Get the value of the message input element
    var message = document.getElementById("messageinput").value;
    // Send the message to the server
    socket.send(message);
    // Clear the message input element
    document.getElementById("messageinput").value = "";
});