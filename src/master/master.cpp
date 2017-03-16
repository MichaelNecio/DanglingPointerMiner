#include <cassert>
#include <iostream>
#include <memory>

#include "easywsclient.hpp"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "guarded_value.h"
#include "sorted_list.h"
#include "threadpool.h"

using namespace rapidjson;
using namespace easywsclient;
using namespace std::literals::chrono_literals;

Document parse_json(const std::string& message) {
  Document d;
  d.Parse(message.data());
  return d;
}

bool is_challenge_message(const Document& d) {
  return d.HasMember("challenge_name");
}

template <typename Comparator>
void start_sorted_list_jobs(const Document& message,
                            qp::threading::Threadpool& pool,
                            const std::atomic<bool>& stop,
                            GuardedValue<uint64_t>& nonce) {
  const std::string& last_solution_hash =
      message["last_solution_hash"].GetString();
  const std::string& hash_prefix = message["hash_prefix"].GetString();
  const int n_elements = message["parameters"]["nb_elements"].GetInt();

  // Intentional copy.
  for (unsigned i = 0; i < std::thread::hardware_concurrency(); ++i) {
    std::cout << "Starting thread: " << i << std::endl;
    pool.add(solve_sorted_list<Comparator>, last_solution_hash, hash_prefix,
             n_elements, std::cref(stop), std::ref(nonce), rand());
  }
}

void start_jobs(const Document& message, qp::threading::Threadpool& pool,
                const std::atomic<bool>& stop, GuardedValue<uint64_t>& nonce) {
  const std::string challenge_type = message["challenge_name"].GetString();
  if (challenge_type == "sorted_list") {
    start_sorted_list_jobs<std::less<uint64_t>>(message, pool, stop, nonce);
  } else if (challenge_type == "reverse_sorted_list") {
    start_sorted_list_jobs<std::greater<uint64_t>>(message, pool, stop, nonce);
  } else {
    std::cout << "unsupported challenge type: " << challenge_type << std::endl;
  }
}

void send_submission(WebSocket* ws, uint64_t nonce) {
  const auto nonce_string = std::to_string(nonce);
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
  writer.String(nonce_string.data());
  writer.EndObject();
  writer.EndObject();

  std::cout << "sending: " << buffer.GetString() << std::endl;
  ws->send(buffer.GetString());
}

int main() {
  std::srand(time(nullptr));
  std::ios_base::sync_with_stdio(false);

  std::atomic<bool> stop_jobs(false);
  qp::threading::Threadpool thread_pool;
  GuardedValue<uint64_t> nonce;

  std::unique_ptr<WebSocket> ws(
      WebSocket::from_url("ws://localhost:8989/client"));
  if (!ws) {
    std::cerr << "Error connecting to websocket" << std::endl;
    return 1;
  }

  ws->send("{\"command\":\"get_current_challenge\",\"args\":{}}");

  while (true) {
    nonce.hold();
    if (nonce.set()) {
      std::cout << "Nonce was discovered, making submission" << std::endl;
      send_submission(ws.get(), nonce.get());
      stop_jobs = true;
      std::this_thread::sleep_for(500ms);
    }
    nonce.unset();
    nonce.drop();

    ws->poll();
    ws->dispatch([&](const std::string& message) {
      std::cout << "Received: " << std::endl << message << std::endl;
      const auto json_message = parse_json(message);

      if (!is_challenge_message(json_message)) return;

      std::cout << "Stopping existing jobs" << std::endl;
      stop_jobs = true;
      std::this_thread::sleep_for(500ms);
      stop_jobs = false;

      std::cout << "Starting mining jobs" << std::endl;
      start_jobs(json_message, thread_pool, stop_jobs, nonce);
    });
  }
}
