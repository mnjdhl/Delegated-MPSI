#ifndef STATS_HPP
#define STATS_HPP
#include <vector>
#include <fstream>
#include <iostream>
#include <map>
#include <filesystem>
#include "common.hpp"

struct msg_complexity {
    size_t msg_cnt;
    size_t msg_size;
};

class Stats {
private:
    std::vector<double> execution_times;
    size_t successful_runs = 0;
    size_t total_repetitions;
    std::string filename;
    std::ofstream file;  // File stream for CSV logging
    std::filesystem::path fpath;
    #if 0
    std::map<int, std::vector<double>> xor_exec_times;
    std::map<int, std::vector<double>> xof_exec_times;
    std::map<int, std::vector<double>> bloomfilter_exec_times;
    #endif

    std::vector<double> xor_exec_times;
    std::vector<double> xof_exec_times;
    std::vector<double> bloomfilter_exec_times;
    std::map<int, double>compute_breakdown_times;
    std::map<int, struct msg_complexity> msg_complexities;

public:
    enum OPS {
        XOR_OP,
        XOF_OP,
        BLOOMFILTER_OP,
        COMPUTE_BREAKDOWN,
        COMPUTE_BREAKDOWN_WAITTIME,
        COMPUTE_MSG_COMPLEXITY
    };

    ~Stats() {
        if (g_options.stats) {
            output_party_csv();
            if (file.is_open()) file.close();
        }
    }

    // Default constructor
    Stats() : total_repetitions(0), filename(""), fpath("") {}

    Stats(size_t repetitions, const std::string& results_filename)
        : total_repetitions(repetitions), filename(results_filename), fpath(results_filename) {
        if (!g_options.stats) {
            return;
        }
        // Open file and write CSV header
        std::ofstream lfile(filename, std::ios::app);
        if (!lfile.is_open()) {
            std::cerr << "Error: Unable to open results file " << filename << "\n";
            return;
        }
        //lfile << "Repetition, ExecutionTime(ms), Success\n";
        //lfile.close();
    }

    Stats(const std::string& filename) : file(filename, std::ios::app), filename(filename), fpath(filename) {
        if (!g_options.stats) {
            return;
        }
        if (!file.is_open()) {
            std::cerr << "Error opening file: " << filename << std::endl;
        } 
        /*else {
            file << "Repetition, Success\n";  // Write CSV header if needed
        }*/
    }

    Stats& operator=(Stats& other) {
        if (this != &other) {
            execution_times = std::move(other.execution_times);
            successful_runs = other.successful_runs;
            total_repetitions = other.total_repetitions;
            filename = std::move(other.filename);
            file = std::move(other.file);
            fpath = std::move(other.fpath);
            xor_exec_times = std::move(other.xor_exec_times);
            xof_exec_times = std::move(other.xof_exec_times);
            bloomfilter_exec_times = std::move(other.bloomfilter_exec_times);
            compute_breakdown_times = std::move(other.compute_breakdown_times);
        }
        return *this;
    }
    
    void log_result(size_t repetition, double exec_time, bool success) {
        if (!g_options.stats) {
            return;
        }
        execution_times.push_back(exec_time);
        if (success) successful_runs++;

        // Append to CSV file
        std::ofstream file(filename, std::ios::app);
        if (file.is_open()) {
            file << repetition << "," << exec_time << "," << (success ? "1" : "0") << "\n";
            file.close();
        } else {
            std::cerr << "Error: Unable to write to results file " << filename << "\n";
        }
    }

    double get_average_time() const {
        double total = 0.0;
        for (double t : execution_times) total += t;
        return execution_times.empty() ? 0.0 : total / execution_times.size();
    }

    bool was_successful() const {
        return successful_runs == total_repetitions;
    }

    void print_summary() const {
        if (!g_options.stats) {
            return;
        }
        std::cout << "Experiment Summary:\n";
        std::cout << "  Average Execution Time: " << get_average_time() << " ms\n";
        std::cout << "  Success Rate: " << successful_runs << " / " << total_repetitions
                  << " (" << (100.0 * successful_runs / total_repetitions) << "%)\n";
    }

    void output_party_csv() {
        if (!file.is_open()) {
            file.open(filename, std::ios::app);
            if (!file.is_open())
                throw std::ios_base::failure("Failed to open file: " + filename);
        }

        if (compute_breakdown_times.size() > 0) {
            if (std::filesystem::file_size(fpath) <= 0) {
                file<<"Set Size, Party Count, Hash Count, Server Side (s), Query Servers (s), Clients (s), Query Servers Message Count, Query Servers Message Size, Client Message Count, Client Message Size\n";
            }
            file<<g_options.set_size<<", "<<g_options.party_count<<", ";
            file<<g_options.hash_count<<", ";

            auto tot_clients = g_options.party_count - 2;
            //Time in seconds
            file<<(compute_breakdown_times[0]/g_options.repetitions)/1000<<", ";
            file<<(compute_breakdown_times[1]/g_options.repetitions)/1000<<", ";
            if (tot_clients <= 0) {
                file<<0<<", ";
            } else {
                file<<((compute_breakdown_times[2]/g_options.repetitions)/1000)/tot_clients<<", ";
            }
            //file <<msg_complexities[0].msg_cnt<<", "<<msg_complexities[0].msg_size<<", ";
            file <<msg_complexities[1].msg_cnt/g_options.repetitions<<", "<<msg_complexities[1].msg_size/g_options.repetitions<<", ";
            if (tot_clients <= 0) {
                file<<0<<", ";
            } else {
                file <<(msg_complexities[2].msg_cnt/g_options.repetitions)/tot_clients<<", "<<(msg_complexities[2].msg_size/g_options.repetitions)/tot_clients<<"\n";
            }
            //return;
        }

    #if 0
        for (int i=0; i<xor_exec_times.size(); i++) {
            double xor_sum = 0.0, xof_sum = 0.0, bloomfilter_sum = 0.0;
            file << "PartyID: " << i << "\n";
            file <<"XOR,    XOF,    BloomFilter\n";
            for (int j=0; j<xor_exec_times[i].size(); j++) {
                file <<xor_exec_times[i][j]<<", "<<xof_exec_times[i][j]<<", "<<bloomfilter_exec_times[i][j]<< "\n";
                xor_sum += xor_exec_times[i][j];
                xof_sum += xof_exec_times[i][j];
                bloomfilter_sum += bloomfilter_exec_times[i][j];
            }
            file<<"Sum: "<<xor_sum<<", "<<xof_sum<<", "<<bloomfilter_sum<< "\n";
            file << "Total: "<<xor_exec_times[i].size()<<", "<<xof_exec_times[i].size()<<", "<<bloomfilter_exec_times[i].size()<< "\n";
            file << "Average: " << xor_sum/xor_exec_times[i].size() << ", " << xof_sum/xof_exec_times[i].size() << ", " << bloomfilter_sum/bloomfilter_exec_times[i].size() << "\n";
            file <<"All times in ms\n";
        }
    #endif
        auto filename2 = (filename.substr(0, filename.find(".")+1));
        filename2.append("_II.");
        filename2.append(filename.substr(filename.find(".")+1));
        std::ofstream file2(filename2, std::ios::app);
        if (!file2.is_open()) {
            std::cerr << "Error: Unable to open results file " << filename2 << "\n";
            return;
        }
        double xor_sum = 0.0, xof_sum = 0.0, bloomfilter_sum = 0.0;
        file2 << "XOR (in ms),    XOF (in ms),    BloomFilter (in ms)\n";
        for (int j=0; j<xor_exec_times.size(); j++) {
            file2 <<xor_exec_times[j]<<", "<<xof_exec_times[j]<<", "<<bloomfilter_exec_times[j]<< "\n";
            xor_sum += xor_exec_times[j];
            xof_sum += xof_exec_times[j];
            bloomfilter_sum += bloomfilter_exec_times[j];
        }
        file2<<"Sum: "<<xor_sum<<", "<<xof_sum<<", "<<bloomfilter_sum<< "\n";
        file2<< "Total: "<<xor_exec_times.size()<<", "<<xof_exec_times.size()<<", "<<bloomfilter_exec_times.size()<< "\n";
        file2<< "Average: " << xor_sum/xor_exec_times.size() << ", " << xof_sum/xof_exec_times.size() << ", " << bloomfilter_sum/bloomfilter_exec_times.size() << "\n";
        file2.close();
    }

    void log_experiment(size_t repetition, bool success) {
        if (!g_options.stats) {
            return;
        }
        if (file.is_open()) {
            file << repetition << "," << (success ? "1" : "0") << "\n";
        } else {
            std::cerr << "Error: CSV file is not open for logging.\n";
        }
    }

    void log_duration(const std::string& label, 
                      const std::chrono::steady_clock::time_point& start_time, 
                      const std::chrono::steady_clock::time_point& end_time) {
        if (!g_options.stats) {
            return;
        }
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        if (file.is_open()) {
            file << label << "," << duration << " ms" << std::endl;
        }
    }

    void log_duration(const OPS& op, 
                    int party_id,
                    const std::chrono::steady_clock::time_point& start_time, 
                    const std::chrono::steady_clock::time_point& end_time) {
        if (!g_options.stats) {
            return;
        }
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        #if 0
        switch (op) {
            case XOR_OP:
                if (xor_exec_times.find(party_id) == xor_exec_times.end()) {
                    xor_exec_times[party_id] = std::vector<double>();
                }
                xor_exec_times[party_id].push_back(duration);
                break;
            case XOF_OP:
                if (xof_exec_times.find(party_id) == xof_exec_times.end()) {
                    xof_exec_times[party_id] = std::vector<double>();
                }
                xof_exec_times[party_id].push_back(duration);
                break;
            case BLOOMFILTER_OP:
                if (bloomfilter_exec_times.find(party_id) == bloomfilter_exec_times.end()) {
                    bloomfilter_exec_times[party_id] = std::vector<double>();
                }
                bloomfilter_exec_times[party_id].push_back(duration);
                break;
        }
        #endif
        switch (op) {
            case XOR_OP:
                xor_exec_times.push_back(duration);
                break;
            case XOF_OP:
                xof_exec_times.push_back(duration);
                break;
            case BLOOMFILTER_OP:
                bloomfilter_exec_times.push_back(duration);
                break;
            case COMPUTE_BREAKDOWN:
                if (party_id < 2) {
                    compute_breakdown_times[party_id] +=duration;
                } else {
                    compute_breakdown_times[2] +=duration;
                }
                break;
            case COMPUTE_BREAKDOWN_WAITTIME:
                if (party_id < 2) {
                    compute_breakdown_times[party_id] -=duration;
                } else {
                    compute_breakdown_times[2] -=duration;
                }
                break;
            
        }
    }
    void log_msg_complexity(int party_id, size_t msg_cnt, size_t msg_size) {
        if (!g_options.stats) {
            return;
        }
        auto mparty_id = party_id;
        if (mparty_id >= 2) {
            mparty_id = 2;
        } 

        if (msg_complexities.find(mparty_id) == msg_complexities.end()) {
            msg_complexities[mparty_id] = msg_complexity();
        }
        msg_complexities[mparty_id].msg_cnt += msg_cnt;
        msg_complexities[mparty_id].msg_size += msg_size;
    }
};

#endif // STATS_HPP
