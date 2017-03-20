#ifndef __DANGMINER_SHORTEST_PATH__
#define __DANGMINER_SHORTEST_PATH__

#include <openssl/sha.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <iostream>
#include <queue>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>

#include "guarded_value.h"
#include "sorted_list.h"  // For the utility functions.

struct State {
  uint64_t row;
  uint64_t col;
  uint64_t priority;

  bool operator==(const State& rhs) const {
    return row == rhs.row && col == rhs.col;
  }

  // Compare on priority FIRST, and then row and col.
  bool operator<(const State& rhs) const {
    if (priority != rhs.priority) return priority < rhs.priority;
    if (row != rhs.row) return row < rhs.row;
    return col < rhs.col;
  }

  // Compare on priority FIRST, and then row and col.
  bool operator>(const State& rhs) const {
    if (priority != rhs.priority) return priority > rhs.priority;
    if (row != rhs.row) return row > rhs.row;
    return col > rhs.col;
  }
};

namespace std {

template <>
class hash<State> {
 public:
  // Thanks boost.
  std::size_t operator()(const State& state) const {
    std::size_t seed = 0;
    std::hash<uint64_t> hash_value;

    seed ^= hash_value(state.row) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= hash_value(state.col) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
  }
};

}  // namespace std

const bool PASSABLE = true;
const bool BLOCKED = false;

void reset_grid(std::vector<std::vector<bool>>& grid) {
  // First row and last rows are blocked.
  std::fill(grid.front().begin(), grid.front().end(), BLOCKED);
  std::fill(grid.back().begin(), grid.back().end(), BLOCKED);
  for (unsigned row = 1; row < grid.size() - 1; ++row) {
    std::fill(grid[row].begin(), grid[row].end(), PASSABLE);
    // First and last columns are blocked.
    grid[row].front() = BLOCKED;
    grid[row].back() = BLOCKED;
  }
}

void solve_shortest_path(const std::string& last_solution_hash,
                         const std::string& hash_prefix, const int grid_size,
                         const int n_blockers, const std::atomic<bool>& stopped,
                         GuardedValue<uint64_t>& nonce,
                         const uint64_t initial_nonce) {
  std::cout << "Mining from " << std::this_thread::get_id() << std::endl;

  std::string buffer;
  unsigned char hash[SHA256_DIGEST_LENGTH];
  uint64_t last_nonce = initial_nonce;
  const auto initial_seed =
      generate_seed(last_nonce, last_solution_hash, buffer, hash);

  std::mt19937_64 rng(initial_seed);
  std::vector<std::vector<bool>> grid(grid_size, std::vector<bool>(grid_size));

  std::unordered_map<State, uint64_t> cost_so_far;
  std::unordered_map<State, State> came_from;
  std::vector<State> path;

  const std::array<int, 4> delta_row{1, -1, 0, 0};
  const std::array<int, 4> delta_col{0, 0, 1, -1};

  uint64_t ugrid_size = grid_size;

  while (!stopped) {
    last_nonce = rng();
    const auto new_seed =
        generate_seed(last_nonce, last_solution_hash, buffer, hash);
    rng.seed(new_seed);

    reset_grid(grid);
    cost_so_far.clear();
    came_from.clear();
    path.clear();

    uint64_t start_row = rng() % ugrid_size;
    uint64_t start_col = rng() % ugrid_size;
    while (grid[start_row][start_col] == BLOCKED) {
      start_row = rng() % ugrid_size;
      start_col = rng() % ugrid_size;
    }

    uint64_t end_row = rng() % ugrid_size;
    uint64_t end_col = rng() % ugrid_size;
    while ((start_row == end_row && start_col == end_col) ||
           grid[end_row][end_col] == BLOCKED) {
      end_row = rng() % ugrid_size;
      end_col = rng() % ugrid_size;
    }

    for (int i = 0; i < n_blockers; ++i) {
      uint64_t block_row = rng() % ugrid_size;
      uint64_t block_col = rng() % ugrid_size;
      if ((block_row == start_row && block_col == start_col) ||
          (block_row == end_row && block_col == end_col))
        continue;
      grid[block_row][block_col] = BLOCKED;
    }

    std::priority_queue<State, std::vector<State>,
                        std::greater<State>> /* the final */ frontier;
    State start_state{start_row, start_col, 0};
    frontier.emplace(start_state);
    cost_so_far[start_state] = 0;

    while (!frontier.empty() && !stopped) {
      const auto current = frontier.top();
      frontier.pop();

      if (current.row == end_row && current.col == end_col) {
        State item{end_row, end_col, 0};
        while (came_from.find(item) != came_from.end()) {
          path.push_back(item);
          item = came_from[item];
        }
        path.push_back(item);
        std::reverse(path.begin(), path.end());

        SHA256_CTX solution_ctx;
        SHA256_Init(&solution_ctx);

        for (const auto& state : path) {
          custom_to_string(state.row, buffer);
          SHA256_Update(&solution_ctx, buffer.data(), buffer.size());

          custom_to_string(state.col, buffer);
          SHA256_Update(&solution_ctx, buffer.data(), buffer.size());
        }

        SHA256_Final(hash, &solution_ctx);
        buffer.clear();
        for (unsigned i = 0; i < hash_prefix.length(); ++i) {
          if ((i & 1ul) == 0ul) {
            buffer.push_back(TO_HEX_CHAR(hash[i / 2] >> 4));
          } else {
            buffer.push_back(TO_HEX_CHAR(hash[i / 2] & 0x0F));
          }
        }

        if (buffer == hash_prefix) {
          nonce.hold();
          nonce.set(last_nonce);
          nonce.drop();
          return;
        } else {
          break;
        }
      }

      const auto current_cost = cost_so_far[current];

      for (int i = 0; i < delta_row.size(); ++i) {
        // NOTE: I don't think this will underflow since the 0'th row and col
        // are all blockers, therefore they will never be in the queue.
        // But this could be a source of error.
        uint64_t next_row = current.row + delta_row[i];
        uint64_t next_col = current.col + delta_col[i];

        if ((next_row >= 0 && next_row < ugrid_size) &&
            (next_col >= 0 && next_col < ugrid_size) &&
            (grid[next_row][next_col] == PASSABLE)) {
          const auto new_cost = current_cost + 1;
          State next_state{next_row, next_col, new_cost};
          if (cost_so_far.find(next_state) == cost_so_far.end() ||
              new_cost < cost_so_far[next_state]) {
            cost_so_far[next_state] = new_cost;
            came_from[next_state] = current;

            // Add a manhattan distance heuristic to get A*.  Gotta be careful
            // since the values are unsigned, so taking the absolute value
            // of the difference won't work.
            next_state.priority += std::max(next_state.row, end_row) -
                                   std::min(next_state.row, end_row) +
                                   std::max(next_state.col, end_col) -
                                   std::min(next_state.col, end_col);
            frontier.push(next_state);
          }
        }
      }
    }
  }
}

#endif
