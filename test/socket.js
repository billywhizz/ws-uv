"use strict";

const net = require("net")
const stats = {
  send: 0,
  recv: 0
}
const client = net.connect({
  port: 8001
}, () => {
  const payload = new Buffer(parseInt(process.argv[2] || "1024"))
  client.write(payload)
  stats.send += payload.length
})
.on("data", (data) => {
  client.write(data)
  stats.recv += data.length
  stats.send += data.length
  //client.end()
})
.on("end", () => {

})

setInterval(() => {
  console.dir(stats)
  stats.send = stats.recv = 0
}, 1000)