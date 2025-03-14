#include "Channels.hpp"
#include <sstream>

using namespace std::chrono_literals;

/* Members/Methods Definitions for 'Channels' class */
// Define static members
std::unordered_map<int, std::queue<std::vector<uint8_t>>> Channels::network;
std::mutex Channels::network_mutex;
void Channels::send(const std::vector<uint8_t>& data, int recipient) {
    simulate_network(data);
    std::lock_guard<std::mutex> lock(network_mutex);
    network[recipient].push(data);
}

void Channels::simulate_network(const std::vector<uint8_t>& data) {
    // Simulate latency
    if (latency_seconds > 0.0) {
        std::this_thread::sleep_for(latency_seconds*1ms);
    }

    // Simulate bandwidth restriction
    if (bytes_per_sec > 0.0) {
        std::cout<<"Channels::simulate_network:send():data size = "<<data.size()<<"\n";
        auto transmission_time = (data.size() / bytes_per_sec)*1s;
        std::this_thread::sleep_for(transmission_time);
    }
}

std::vector<uint8_t> Channels::receive(int sender) {
    std::lock_guard<std::mutex> lock(network_mutex);
    std::vector<uint8_t> data;
    
     if (network[sender].empty()) {
        //throw std::runtime_error("No messages from sender " + std::to_string(sender));
        return data;
    }
    data = network[sender].front();
    network[sender].pop();
    return data;
}

void Channels::receive(int from, std::vector<std::vector<size_t>>& data) {
    std::vector<uint8_t> raw_bytes;
    raw_bytes = receive(from);

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

void Channels::send(int to, const std::vector<std::vector<size_t>>& data) {
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
    send(std::vector<uint8_t>(serialized.begin(), serialized.end()), to);
}

void Channels::receive(int from, std::vector<bool>& data) {
    std::vector<uint8_t> byte_data;
    byte_data = receive(from);  // Get raw bytes

    data.resize(byte_data.size() * 8, false);  // Resize to match original bool count

    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = (byte_data[i / 8] & (1 << (i % 8))) != 0;  // Extract each bit
    }
}

void Channels::send(int to, const std::vector<bool>& data) {
    std::vector<uint8_t> byte_data((data.size() + 7) / 8, 0);  // Allocate bytes (8 bools per byte)

    for (size_t i = 0; i < data.size(); ++i) {
        if (data[i]) {
            byte_data[i / 8] |= (1 << (i % 8));  // Set the corresponding bit
        }
    }

    send(byte_data, to);  // Send as raw bytes
}
