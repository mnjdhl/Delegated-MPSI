#ifndef CHANNELS_HPP
#define CHANNELS_HPP

#include <unordered_map>
#include <mutex>
#include <queue>
#include <vector>
#include <cstdint>
#include <iostream>

class Channels {
    public:
    //Channels() = default;
    Channels() {
        std::cout << "Channels constructor called" << std::endl;
        // Debugging checkpoints
        std::cout << "Before allocating buffers" << std::endl;
        network.bucket_size(100); //.resize(1024); // If this crashes, buffer is corrupted
        std::cout << "After allocating buffers" << std::endl;

        std::cout << "Channels constructor finished" << std::endl;
    }
    void send(const std::vector<uint8_t>& data, int recipient);

    std::vector<uint8_t> receive(int sender);
    void receive(int from, std::vector<std::vector<size_t>>& data);
    void send(int to, const std::vector<std::vector<size_t>>& data);
    void receive(int from, std::vector<bool>& data);
    void send(int to, const std::vector<bool>& data);

    private:
    // Static map to simulate a network (for testing without real networking)
    static std::unordered_map<int, std::queue<std::vector<uint8_t>>> network;

    // Mutex for thread safety
    static std::mutex network_mutex;
};

#endif // CHANNELS_HPP