#include <cassert>
#include <iostream>
#include <memory>

#include "easywsclient/easywsclient.hpp"

#include "rapidjson/include/rapidjson/document.h"
#include "rapidjson/include/rapidjson/stringbuffer.h"
#include "rapidjson/include/rapidjson/writer.h"

#include "solvers/sorted_list.h"

using namespace rapidjson;

std::uint64_t handle_message(const std::string& message, bool* error) {
  Document d;
  d.Parse(message.data());
  *error = false;

  if (!d.HasMember("challenge_name")) {
    std::cout << "Non Challenge message: " << std::endl << message << std::endl;
    *error = true;
    return -1;
  }

  const std::string challenge_type = d["challenge_name"].GetString();
  if (challenge_type == "sorted_list") {
    std::cout << "Attempting to solve..." << std::endl;
    return solve_sorted_list(d["last_solution_hash"].GetString(),
                             d["hash_prefix"].GetString(),
                             d["parameters"]["nb_elements"].GetInt());
  } else {
    *error = true;
    std::cout << "unsupported challenge type: " << challenge_type << std::endl;
    return -1;
  }
}

int main() {
  std::srand(time(nullptr));
  std::ios_base::sync_with_stdio(false);
  using easywsclient::WebSocket;

  std::unique_ptr<WebSocket> ws(
      WebSocket::from_url("ws://localhost:8989/client"));
  assert(ws);
  ws->send("{\"command\":\"get_current_challenge\",\"args\":{}}");

  while (true) {
    ws->poll();
    ws->dispatch([&ws](const std::string& message) {
      bool error;
      std::cout << "got: " << std::endl << message << std::endl;
      const auto nonce = std::to_string(handle_message(message, &error));
      if (error) return;

      StringBuffer buffer;
      Writer<StringBuffer> writer(buffer);
      writer.StartObject();
      writer.Key("command");
      writer.String("submission");
      writer.Key("args");
      writer.StartObject();
      writer.Key("wallet_id");
      writer.String("fuck");
      writer.Key("nonce");
      writer.String(nonce.data());
      writer.EndObject();
      writer.EndObject();

      std::cout << "sending: " << buffer.GetString() << std::endl;
      ws->send(buffer.GetString());
    });
  }
}
