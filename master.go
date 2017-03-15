package main;

import (
  "golang.org/x/net/websocket"
  "fmt"
  "log"
)

const (
  WS_URL = "ws://localhost:8989/client"
  WS_ORIGIN = "http://localhost/"
)

func main() {
  ws, err := websocket.Dial(WS_URL, "", WS_ORIGIN)
  if (err != nil) {
    panic(err)
  }

  request := []byte("{\"command\":\"get_current_challenge\",\"args\":{}}")
  _, err = ws.Write(request)
  if (err != nil) {
    panic(err)
  }

  var msg = make([]byte, 512)
  var n int
  if n, err = ws.Read(msg); err != nil {
    log.Fatal(err)
  }
  fmt.Printf("Received: %s.\n", msg[:n])
}
