"use strict";

var WebSocket = require("ws")

WebSocket.prototype.sendJSON = function (payload) {
  return this.send(JSON.stringify(payload), { mask: false })
}

function connect() {

  var ws = new WebSocket("ws://127.0.0.1/", {
    protocolVersion: 8,
    protocol: "producer-client"
  })

  ws.on("open", () => {
    console.log("open")
    ws.sendJSON(process.memoryUsage())
  })

  ws.on("error", (err) => {
    console.error(err)
  })

  ws.on("close", () => {
    console.log("close")
    setTimeout(connect, 1000)
  })

  ws.on("message", (message) => {
    console.log("message")
    console.dir(message)
    //ws.sendJSON(message)
  })

}

connect()