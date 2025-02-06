#include <vector>
#include <fstream>
#include <iostream>

class Stats {
private:
    std::vector<double> execution_times;
    size_t successful_runs = 0;
    size_t total_repetitions;
    std::string filename;
    std::ofstream file;  // File stream for CSV logging
    std::vector<double> xor_exec_times;
    std::vector<double> xof_exec_times;
    std::vector<double> bloomfilter_exec_times;

public:
    enum OPS {
        XOR_OP,
        XOF_OP,
        BLOOMFILTER_OP
    };

    ~Stats() {
        output_party_csv();
        if (file.is_open()) file.close();
    }
    Stats(size_t repetitions, const std::string& results_filename)
        : total_repetitions(repetitions), filename(results_filename) {
        // Open file and write CSV header
        std::ofstream lfile(filename, std::ios::trunc);
        if (!lfile.is_open()) {
            std::cerr << "Error: Unable to open results file " << filename << "\n";
            return;
        }
        //lfile << "Repetition, ExecutionTime(ms), Success\n";
        //lfile.close();
    }

    Stats(const std::string& filename) : file(filename, std::ios::app) {
        if (!file.is_open()) {
            std::cerr << "Error opening file: " << filename << std::endl;
        } 
        /*else {
            file << "Repetition, Success\n";  // Write CSV header if needed
        }*/
    }

    void log_result(size_t repetition, double exec_time, bool success) {
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
        std::cout << "Experiment Summary:\n";
        std::cout << "  Average Execution Time: " << get_average_time() << " ms\n";
        std::cout << "  Success Rate: " << successful_runs << " / " << total_repetitions
                  << " (" << (100.0 * successful_runs / total_repetitions) << "%)\n";
    }

    void output_party_csv() { //int party_id) { 
        if (!file.is_open()) {
            file.open(filename, std::ios::trunc);
            if (!file.is_open())
                throw std::ios_base::failure("Failed to open file: " + filename);
        }

        // Output dummy stats for now
        //file << "PartyID, ExecutionTime\n";
        //file << party_id << ", " << 0.123 << "\n"; // Example data
        double xor_sum = 0.0, xof_sum = 0.0, bloomfilter_sum = 0.0;
        file <<"XOR, XOF, BloomFilter\n";
        for (int i=0; i<xor_exec_times.size(); i++) {
            file <<xor_exec_times[i]<<", "<<xof_exec_times[i]<<", "<<bloomfilter_exec_times[i]<< "\n";
            xor_sum += xor_exec_times[i];
            xof_sum += xof_exec_times[i];
            bloomfilter_sum += bloomfilter_exec_times[i];
        }
        file <<"Total: "<<xor_sum<<", "<<xof_sum<<", "<<bloomfilter_sum<< "\n";
        file << "Average: " << xor_sum/xor_exec_times.size() << ", " << xof_sum/xof_exec_times.size() << ", " << bloomfilter_sum/bloomfilter_exec_times.size() << "\n";

    }

    void log_experiment(size_t repetition, bool success) {
        if (file.is_open()) {
            file << repetition << "," << (success ? "1" : "0") << "\n";
        } else {
            std::cerr << "Error: CSV file is not open for logging.\n";
        }
    }

    void log_duration(const std::string& label, 
                      const std::chrono::steady_clock::time_point& start_time, 
                      const std::chrono::steady_clock::time_point& end_time) {
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        if (file.is_open()) {
            file << label << "," << duration << " ms" << std::endl;
        }
    }

    void log_duration(const OPS& op, 
                      const std::chrono::steady_clock::time_point& start_time, 
                      const std::chrono::steady_clock::time_point& end_time) {
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
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
        }
    }
};

