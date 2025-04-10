#ifndef FULLMESH_HPP
#define FULLMESH_HPP

#include <vector>
#include <queue>
#include <unordered_map>
#include <mutex>
#include <cstdint>
#include <chrono>
#include <thread>
#include "Channels.hpp"
#include "Stats.hpp"

class FullMesh {
public:
    static FullMesh new_default() {
        return FullMesh(0.0, 0.0);
    }

    static FullMesh new_with_overhead(double latency, double bytes_per_sec) {
        return FullMesh(latency, bytes_per_sec);
    }
    
    FullMesh(double latency_seconds, double bytes_per_sec, size_t party_count /*, Stats & stats*/);


    /*FullMesh(std::vector<std::unique_ptr<Channels>>&& channels);*/

    explicit FullMesh(size_t party_count);
    
    // Delete copy constructor and copy assignment to prevent invalid copies
    //FullMesh(const FullMesh&) = delete;
    //FullMesh& operator=(const FullMesh&) = delete;

    void initialize_channels();

    FullMesh& operator=(FullMesh&& other) noexcept;

    // Send a message to a specific party
    void send(size_t sender_id, size_t recipient_id, const std::vector<uint8_t>& data);
    void send(size_t sender_id, size_t recipient_id, const std::vector<std::vector<size_t>>& data);
    void send(size_t sender_id, size_t recipient_id, const std::vector<bool>& data);

    // Receive a message from a specific party
    std::vector<uint8_t> receive(size_t receiver_id, size_t sender_id);
    void receive(size_t receiver_id, size_t sender_id, std::vector<std::vector<size_t>>& data);
    void receive(size_t receiver_id, size_t sender_id, std::vector<bool>& data);

    bool can_receive(size_t receiver_id, size_t sender_id);
    
    // Returns a reference to the communication channels
    Channels& get_channels(size_t party_id) const;

private:
    // Simulated latency and bandwidth
    double latency_seconds;
    double bytes_per_sec;
    size_t party_count;
    std::vector<std::unique_ptr<Channels>> channels;

    // Static map to simulate a network (for testing without real networking)
    static std::unordered_map<size_t, std::unordered_map<size_t, std::queue<std::vector<uint8_t>>>> network;

    // Mutex for thread safety
    static std::mutex network_mutex;
    //Stats stats;
    
        // Constructor with optional latency and bandwidth constraints
    FullMesh(double latency_seconds = 0.0, double bytes_per_sec = 0.0);
};

#endif // FULLMESH_HPP