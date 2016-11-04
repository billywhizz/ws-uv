var createSocket = require("./build/Release/tiny")

function onConnect(fd) {
  console.log("onConnect: " + fd)
}

function onData(fd, len) {
  console.log("onData: " + fd + ", len: " + len)
}

function onClose(fd) {
  console.log("onClose: " + fd)
}

var sock = createSocket()
var r = sock.listen(8001, onConnect, onData, onClose)