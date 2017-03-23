#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

#include "Hub.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "cscoins_wallet.h"
#include "guarded_value.h"
#include "threadpool.h"

#include "shortest_path.h"
#include "sorted_list.h"

using namespace rapidjson;
using JobHandle = std::future<void>;

Document parse_json(const std::string& message) {
  Document d;
  d.Parse(message.data());
  return d;
}

bool is_challenge_message(const Document& d) {
  return d.HasMember("challenge_name");
}

template <typename Comparator>
std::vector<JobHandle> start_sorted_list_jobs(const Document& message,
                                              qp::threading::Threadpool& pool,
                                              const std::atomic<bool>& stop,
                                              GuardedValue<uint64_t>& nonce) {
  const std::string& last_solution_hash =
      message["last_solution_hash"].GetString();
  const std::string& hash_prefix = message["hash_prefix"].GetString();
  const int n_elements = message["parameters"]["nb_elements"].GetInt();

  std::vector<JobHandle> handles;
  // Intentional copy.
  for (unsigned i = 0; i < std::thread::hardware_concurrency(); ++i) {
    std::cout << "Starting thread: " << i << std::endl;
    handles.emplace_back(pool.add(solve_sorted_list<Comparator>,
                                  last_solution_hash, hash_prefix, n_elements,
                                  std::cref(stop), std::ref(nonce), rand()));
  }
  return handles;
}

std::vector<JobHandle> start_shortest_path_jobs(const Document& message,
                                                qp::threading::Threadpool& pool,
                                                const std::atomic<bool>& stop,
                                                GuardedValue<uint64_t>& nonce) {
  const std::string& last_solution_hash =
      message["last_solution_hash"].GetString();
  const std::string& hash_prefix = message["hash_prefix"].GetString();
  const int grid_size = message["parameters"]["grid_size"].GetInt();
  const int n_blockers = message["parameters"]["nb_blockers"].GetInt();

  std::vector<JobHandle> handles;
  // Intentional copy.
  for (unsigned i = 0; i < std::thread::hardware_concurrency(); ++i) {
    std::cout << "Starting thread: " << i << std::endl;
    handles.emplace_back(pool.add(solve_shortest_path, last_solution_hash,
                                  hash_prefix, grid_size, n_blockers,
                                  std::cref(stop), std::ref(nonce), rand()));
  }
  return handles;
}

std::vector<JobHandle> start_jobs(const Document& message,
                                  qp::threading::Threadpool& pool,
                                  const std::atomic<bool>& stop,
                                  GuardedValue<uint64_t>& nonce) {
  const std::string challenge_type = message["challenge_name"].GetString();
  if (challenge_type == "sorted_list") {
    return start_sorted_list_jobs<std::less<uint64_t>>(message, pool, stop,
                                                       nonce);
  } else if (challenge_type == "reverse_sorted_list") {
    return start_sorted_list_jobs<std::greater<uint64_t>>(message, pool, stop,
                                                          nonce);
  } else if (challenge_type == "shortest_path") {
    return start_shortest_path_jobs(message, pool, stop, nonce);
  } else {
    std::cout << "unsupported challenge type: " << challenge_type << std::endl;
  }
  return {};
}

void send_registration(uWS::WebSocket<uWS::CLIENT>& ws,
                       const cscoins_wallet::CSCoinsWallet& wallet) {
  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  writer.StartObject();
  writer.Key("command");
  writer.String("register_wallet");
  writer.Key("args");
  writer.StartObject();
  writer.Key("name");
  writer.String("DanglingPointers");
  writer.Key("key");
  writer.String(wallet.public_key().data());
  writer.Key("signature");
  writer.String(wallet.registration_signature().data());
  writer.EndObject();
  writer.EndObject();

  std::cout << "registering.." << std::endl;
  ws.send(buffer.GetString());
}

void send_submission(uWS::WebSocket<uWS::CLIENT>& ws, uint64_t nonce,
                     const std::string& wallet_id) {
  const auto nonce_string = std::to_string(nonce);
  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  writer.StartObject();
  writer.Key("command");
  writer.String("submission");
  writer.Key("args");
  writer.StartObject();
  writer.Key("wallet_id");
  writer.String(wallet_id.data());
  writer.Key("nonce");
  writer.String(nonce_string.data());
  writer.EndObject();
  writer.EndObject();

  std::cout << "sending: " << buffer.GetString() << std::endl;
  ws.send(buffer.GetString());
}

void wait_jobs(std::vector<JobHandle>& job_handles,
               std::atomic<bool>& stopped) {
  static std::mutex mu;
  std::lock_guard<std::mutex> lock(mu);

  stopped = true;
  for (auto& handle : job_handles) handle.wait();
  stopped = false;
  job_handles.clear();
}

int main() {
  std::srand(time(nullptr));
  std::ios_base::sync_with_stdio(false);

  std::atomic<bool> stop_jobs(false);
  qp::threading::Threadpool thread_pool;
  GuardedValue<uint64_t> nonce;
  std::vector<JobHandle> job_handles;

  uWS::Hub ws;
  uWS::WebSocket<uWS::CLIENT> csgames_socket;

  cscoins_wallet::CSCoinsWallet wallet("public.pem", "private.pem",
                                       "public.der");

  ws.onConnection([&](uWS::WebSocket<uWS::CLIENT> s, uWS::HttpRequest _) {
    send_registration(s, wallet);
    s.send("{\"command\":\"get_current_challenge\",\"args\":{}}");
    csgames_socket = s;
  });

  ws.onMessage([&](uWS::WebSocket<uWS::CLIENT> s, const char* message,
                   size_t length, uWS::OpCode) {
    std::string actual_message(message, message + length);
    std::cout << "Received: " << std::endl << actual_message << std::endl;
    std::cout << "length: " << length << std::endl;
    const auto json_message = parse_json(actual_message);

    if (!is_challenge_message(json_message)) return;

    std::cout << "Stopping existing jobs" << std::endl;
    wait_jobs(job_handles, stop_jobs);

    std::cout << "Starting mining jobs" << std::endl;
    job_handles = start_jobs(json_message, thread_pool, stop_jobs, nonce);
  });

  std::thread poll([&]() {
    while (true) {
      nonce.hold();
      if (nonce.set()) {
        std::cout << "Nonce was discovered, making submission" << std::endl;
        send_submission(csgames_socket, nonce.get(), wallet.wallet_id());
        wait_jobs(job_handles, stop_jobs);
      }
      nonce.unset();
      nonce.drop();
      std::this_thread::yield();
    }
  });

  ws.connect("ws://localhost:8989/client", nullptr);
  ws.run();
  poll.join();
}
