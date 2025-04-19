#include "FullMesh.hpp"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include "common.hpp"
#include "Stats.hpp"
using namespace std::chrono_literals;

extern Stats g_stats;
// Define static members
std::unordered_map<size_t, std::unordered_map<size_t, std::queue<std::vector<uint8_t>>>> FullMesh::network;
std::mutex FullMesh::network_mutex;

// Constructor
FullMesh::FullMesh(double latency_seconds, double bytes_per_sec)
    : latency_seconds(latency_seconds), bytes_per_sec(bytes_per_sec) {
        initialize_channels();
    }

FullMesh::FullMesh(double latency_seconds, double bytes_per_sec, size_t party_count /*, Stats & pstats*/)
    : latency_seconds(latency_seconds), bytes_per_sec(bytes_per_sec), party_count(party_count) /*, stats(pstats)*/ {
        initialize_channels();
    }

FullMesh::FullMesh(size_t party_count) : party_count(party_count) {
    channels.reserve(party_count);
    for (size_t i = 0; i < party_count; ++i) {
        channels.emplace_back(std::make_unique<Channels>());
    }
}

/*FullMesh::FullMesh(std::vector<std::unique_ptr<Channels>>&& channels)
    : channels(std::move(channels)) {}*/

void FullMesh::initialize_channels() {
    channels.reserve(party_count);
    for (size_t i = 0; i < party_count; ++i) {
        channels.emplace_back(std::make_unique<Channels>(latency_seconds, bytes_per_sec));
        //channels[i] = std::make_unique<Channels>();  // Assuming Channels has a default constructor
    }
}

// Move assignment operator (allowed)
FullMesh& FullMesh::operator=(FullMesh&& other) noexcept {
    if (this != &other) {
        party_count = other.party_count;
        channels = std::move(other.channels);
    }
    return *this;
}

// Send a message to a recipient party
void FullMesh::send(size_t sender_id, size_t recipient_id, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(network_mutex);

    // Simulate latency
    if (latency_seconds > 0.0) {
        //std::this_thread::sleep_for(std::chrono::duration<double>(latency_seconds));
        std::this_thread::sleep_for(latency_seconds*1ms);
    }

    // Simulate bandwidth restriction
    if (bytes_per_sec > 0.0) {
        auto transmission_time = (data.size() / bytes_per_sec)*1s;
        //std::this_thread::sleep_for(std::chrono::duration<double>(transmission_time));
        std::this_thread::sleep_for(transmission_time);
    }

    // Push the data into the recipient’s queue
    network[recipient_id][sender_id].push(data);
}

void FullMesh::send(size_t sender_id, size_t recipient_id, const std::vector<std::vector<size_t>>& data) {
    std::ostringstream oss;
    size_t outer_size = data.size();
    oss.write(reinterpret_cast<const char*>(&outer_size), sizeof(size_t));

    for (const auto& vec : data) {
        size_t inner_size = vec.size();
        oss.write(reinterpret_cast<const char*>(&inner_size), sizeof(size_t));

        for (size_t val : vec) {
            oss.write(reinterpret_cast<const char*>(&val), sizeof(size_t));
        }
    }

    std::string serialized = oss.str();
    send(sender_id, recipient_id, std::vector<uint8_t>(serialized.begin(), serialized.end()));
}


void FullMesh::send(size_t sender_id, size_t recipient_id, const std::vector<bool>& data) {
    std::vector<uint8_t> byte_data((data.size() + 7) / 8, 0);  // Allocate bytes (8 bools per byte)

    for (size_t i = 0; i < data.size(); ++i) {
        if (data[i]) {
            byte_data[i / 8] |= (1 << (i % 8));  // Set the corresponding bit
        }
    }

    send(sender_id, recipient_id, byte_data);  // Send as raw bytes
}


// Receive a message from a sender party
std::vector<uint8_t> FullMesh::receive(size_t receiver_id, size_t sender_id) {

    auto start_time = std::chrono::steady_clock::now();
    /* Wait till data is available  */
    while (!can_receive(receiver_id, sender_id)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    auto end_time = std::chrono::steady_clock::now();
    g_stats.log_duration(Stats::OPS::COMPUTE_BREAKDOWN_WAITTIME, receiver_id, start_time, end_time);

    std::lock_guard<std::mutex> lock(network_mutex);

    auto& queue = network[receiver_id][sender_id];

    if (queue.empty()) {
        throw std::runtime_error("No messages from sender " + std::to_string(sender_id));
    }

    std::vector<uint8_t> message = queue.front();
    queue.pop();
    g_stats.log_msg_complexity(sender_id, 1, message.size()); // Log message complexity
    return message;
}

void FullMesh::receive(size_t receiver_id, size_t sender_id, std::vector<std::vector<size_t>>& data) {
    std::vector<uint8_t> raw_bytes;
    raw_bytes = receive(receiver_id, sender_id);

    std::istringstream iss(std::string(raw_bytes.begin(), raw_bytes.end()));
    size_t outer_size = 0;
    iss.read(reinterpret_cast<char*>(&outer_size), sizeof(size_t));

    data.resize(outer_size);
    for (size_t i = 0; i < outer_size; ++i) {
        size_t inner_size;
        iss.read(reinterpret_cast<char*>(&inner_size), sizeof(size_t));
        data[i].resize(inner_size);

        for (size_t j = 0; j < inner_size; ++j) {
            iss.read(reinterpret_cast<char*>(&data[i][j]), sizeof(size_t));
        }
    }
}

void FullMesh::receive(size_t receiver_id, size_t sender_id, std::vector<bool>& data) {
    std::vector<uint8_t> byte_data;
    byte_data = receive(receiver_id, sender_id);  // Get raw bytes

    data.resize(byte_data.size() * 8, false);  // Resize to match original bool count

    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = (byte_data[i / 8] & (1 << (i % 8))) != 0;  // Extract each bit
    }
}

bool FullMesh::can_receive(size_t receiver_id, size_t sender_id) {
    return !network[receiver_id][sender_id].empty();
}

Channels& FullMesh::get_channels(size_t party_id) const {
    if (party_id >= channels.size()) {
        throw std::out_of_range("Invalid party_id: " + std::to_string(party_id));
    }
    return *channels[party_id];  // Return reference to the party's channels
}