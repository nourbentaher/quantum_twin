const net = require("net");
const WebSocket = require("ws");

const TCP_HOST = "192.168.1.42";
const TCP_PORT = 9090;

// WebSocket server for dashboard
const wss = new WebSocket.Server({ port: 8080 });

wss.on("connection", (ws) => {
    console.log("Dashboard connected");

    ws.on("message", (msg) => {
        const client = new net.Socket();

        client.connect(TCP_PORT, TCP_HOST, () => {
            client.write(msg.toString());
        });

        client.on("data", (data) => {
            ws.send(data.toString());
            client.destroy();
        });

        client.on("error", (err) => {
            ws.send("ERROR: " + err.message);
        });
    });
});

console.log("Bridge running on ws://localhost:8080");
