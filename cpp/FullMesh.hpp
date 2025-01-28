// Network simulation (FullMesh equivalent)
class FullMesh {
public:
    static FullMesh new_default() {
        return FullMesh(0.0, 0.0);
    }

    static FullMesh new_with_overhead(double latency, double bytes_per_sec) {
        return FullMesh(latency, bytes_per_sec);
    }

    double latency;
    double bytes_per_sec;

private:
    FullMesh(double latency, double bytes_per_sec) : latency(latency), bytes_per_sec(bytes_per_sec) {}
};


