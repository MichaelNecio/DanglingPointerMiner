#include <uWs/Hub.h>

int main() {
  uWS::Hub ws;

  ws.onConnection([](uWS::WebSocket<uWS::CLIENT> s, uWS::HttpRequest) {
    s.send("{\"command\": \"get_current_challenge\", \"args\": {}}");
  });

  ws.onMessage([](uWS::WebSocket<uWS::CLIENT> s, const char* message,
                  size_t length,
                  uWS::OpCode opCode) { std::cout << message << std::endl; });

  // ws.listen(8000);
  ws.connect("ws://localhost:8989/client", nullptr);
  ws.run();
}
