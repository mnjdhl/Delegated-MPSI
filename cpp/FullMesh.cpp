#include "FullMesh.hpp"
#include <stdexcept>
#include <iostream>

using namespace std::chrono_literals;

// Define static members
std::unordered_map<size_t, std::unordered_map<size_t, std::queue<std::vector<uint8_t>>>> FullMesh::network;
std::mutex FullMesh::network_mutex;

// Constructor
FullMesh::FullMesh(double latency_seconds, double bytes_per_sec)
    : latency_seconds(latency_seconds), bytes_per_sec(bytes_per_sec) {
        initialize_channels();
    }

FullMesh::FullMesh(double latency_seconds, double bytes_per_sec, size_t party_count)
    : latency_seconds(latency_seconds), bytes_per_sec(bytes_per_sec), party_count(party_count) {
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
        channels.emplace_back(std::make_unique<Channels>());
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

// Receive a message from a sender party
std::vector<uint8_t> FullMesh::receive(size_t receiver_id, size_t sender_id) {
    std::lock_guard<std::mutex> lock(network_mutex);

    auto& queue = network[receiver_id][sender_id];

    if (queue.empty()) {
        throw std::runtime_error("No messages from sender " + std::to_string(sender_id));
    }

    std::vector<uint8_t> message = queue.front();
    queue.pop();
    return message;
}

Channels& FullMesh::get_channels(size_t party_id) const {
    if (party_id >= channels.size()) {
        throw std::out_of_range("Invalid party_id: " + std::to_string(party_id));
    }
    return *channels[party_id];  // Return reference to the party's channels
}