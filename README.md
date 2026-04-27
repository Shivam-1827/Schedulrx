# ⚡ SchedulrX – Adaptive CPU Scheduling Platform

![Go Version](https://img.shields.io/badge/Go-1.25-00ADD8?style=flat&logo=go)
![C++ Version](https://img.shields.io/badge/C++-17-00599C?style=flat&logo=c%2B%2B)
![Redis](https://img.shields.io/badge/Redis-7.0-DC382D?style=flat&logo=redis)
![PostgreSQL](https://img.shields.io/badge/PostgreSQL-15-4169E1?style=flat&logo=postgresql)
![Docker](https://img.shields.io/badge/Docker-Multi--Stage-2496ED?style=flat&logo=docker)
![License](https://img.shields.io/badge/License-MIT-green.svg)

**SchedulrX** is a **distributed, high-performance simulation platform** for evaluating modern operating system CPU schedulers. It bridges the gap between high-concurrency web orchestration (Go) and low-level system execution (C++17), enabling researchers and systems engineers to benchmark scheduling algorithms in realistic, concurrent scenarios.

Unlike academic simulators limited to single-threaded evaluation, SchedulrX is built as a **production-grade Distributed Backend Architecture**:

1. **The Control Plane (Go):** A high-throughput API gateway (Gin framework) that ingests workloads, manages asynchronous worker pools via Redis, validates payloads, and executes heuristic recommendations.
2. **The Data Plane (C++17):** A lightning-fast, lock-free discrete event simulator handling multi-core workload distribution, work-stealing queue rebalancing, and algorithmic execution with <1ms latencies.
3. **Observability Layer:** Built-in Prometheus telemetry tracking API throughput, simulation latency, queue depths, and scheduler performance metrics.

---

## 🧠 Core Features

- **🔄 Multi-Core SMP Simulation:** Simulates $N$-core processors utilizing a **Work Stealing** algorithm to dynamically rebalance run queues and prevent core idling, accurately modeling real multiprocessor behavior.

- **📊 The Intelligence Layer:** Implements an **Adaptive Scheduler** using **Exponentially Weighted Moving Average (EWMA)** ($\tau_{n+1} = \alpha t_n + (1-\alpha)\tau_n$) to predict CPU burst times, paired with real-time Round Robin quantum auto-tuning for dynamic workload adjustment.

- **⚙️ Distributed Asynchronous Execution:** Eliminates the "Thundering Herd" problem using Redis as a message broker and a Go-based Goroutine worker pool with configurable concurrency.

- **⚡ Parallel Benchmarking:** The `/compare` API fans out a single workload to multiple C++ worker processes simultaneously using Go channels, ranking algorithm performance in real-time without blocking.

- **📚 Classical Algorithms Supported:**
  - **FCFS** (First-Come-First-Served)
  - **SJF** (Shortest Job First)
  - **RR** (Round Robin) with dynamic quantum tuning
  - **MLFQ** (Multi-Level Feedback Queue)
  - **EDF** (Earliest Deadline First)
  - **Adaptive** (Intelligent burst prediction)

- **📈 Observability:** Prometheus metrics including:
  - `schedulrx_simulation_duration_seconds` – Histogram of simulation execution time
  - `schedulrx_jobs_queued_current` – Gauge of pending async jobs
  - `schedulrx_api_requests_total` – Counter of API requests by endpoint
  - Custom performance metrics per algorithm

---

## 🏗 System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                  Client (cURL / Postman / SDK)              │
└────────────────────────────┬────────────────────────────────┘
                             │
                             ▼
        ┌────────────────────────────────────────────────┐
        │          Go API Gateway (Gin)                  │ (Control Plane)
        │  ┌──────────────────────────────────────────┐  │
        │  │ • Request Validation & Schema Checks    │  │
        │  │ • Heuristic Recommendation Engine       │  │
        │  │ • JSON ↔ Protocol Buffer Serialization  │  │
        │  └──────────────────────────────────────────┘  │
        └────────┬──────────────────────────────┬────────┘
                 │                              │
        [Sync Execute]                   [Async Queue]
                 │                              │
                 │                              ▼
                 │                      ┌──────────────────┐
                 │                      │  Redis Queue     │ (Message Broker)
                 │                      │ (Job Distribution)
                 │                      └────────┬─────────┘
                 │                              │
                 ▼                              ▼
        ┌────────────────────────────────────────────────┐
        │       Go Worker Pool (Goroutines)              │ (Orchestrator)
        │  ┌──────────────────────────────────────────┐  │
        │  │ • C++ Process Lifecycle Management      │  │
        │  │ • JSON-over-Pipe IPC                    │  │
        │  │ • Error Recovery & Retry Logic          │  │
        │  │ • Concurrent Process Pooling            │  │
        │  └──────────────────────────────────────────┘  │
        └────────┬──────────────────────────┬────────────┘
                 │ (Fork C++ Engines)       │
        ┌────────▼──────────┐      ┌────────▼──────────┐
        │   C++ Engine #1   │      │   C++ Engine #N   │ (Data Plane)
        │  ┌──────────────┐ │      │  ┌──────────────┐ │
        │  │ • Scheduler  │ │      │  │ • Scheduler  │ │
        │  │ • MLFQ Impl  │ │ ...  │  │ • Adaptive   │ │
        │  │ • SMP Queue  │ │      │  │ • Work Steal │ │
        │  │ • Cache Ops  │ │      │  │ • GC-Free    │ │
        │  └──────────────┘ │      │  └──────────────┘ │
        └────────┬──────────┘      └────────┬──────────┘
                 │                         │
                 └────────┬────────────────┘
                          ▼
        ┌────────────────────────────────────┐
        │   PostgreSQL (Persistent Storage)   │ (Source of Truth)
        │ ┌────────────────────────────────┐ │
        │ │ • RunRecord (Simulation Logs)  │ │
        │ │ • WorkloadHistory              │ │
        │ │ • Performance Baselines        │ │
        │ └────────────────────────────────┘ │
        └────────────────────────────────────┘
                          ▲
                          │
        ┌────────────────────────────────────┐
        │    Prometheus Scraper (9090)       │ (Metrics)
        │    • Real-time Performance Graphs  │
        │    • Latency Percentiles           │
        └────────────────────────────────────┘
```

---

## 🚀 Quick Start

### Prerequisites

- **Docker** 20.10+
- **Docker Compose** 1.29+
- **Git**

### 1. Clone the Repository

```bash
git clone https://github.com/Shivam-1827/schedulrx.git
cd schedulrx
```

### 2. Build and Start the Cluster

```bash
docker-compose up --build -d
```

This will start:

- **SchedulrX API** on `http://localhost:8080`
- **PostgreSQL** on `localhost:5432`
- **Redis** on `localhost:6379`
- **Prometheus** on `http://localhost:9090`

### 3. Verify Deployment

```bash
# Check API health
curl http://localhost:8080/health

# Expected response:
# {"status":"orchestrator_online"}
```

### 4. Run Your First Simulation

```bash
curl -X POST http://localhost:8080/api/v1/simulate \
  -H "Content-Type: application/json" \
  -d '{
    "settings": {
      "algorithm": "MLFQ",
      "quantum": 2.0,
      "context_switch_penalty": 0.1,
      "num_cores": 4
    },
    "tasks": [
      {"id": "t1", "arrival_time": 0.0, "burst_time": 10.5, "priority": 1, "task_type": "CPU_BOUND", "deadline": 15.0},
      {"id": "t2", "arrival_time": 2.0, "burst_time": 3.0, "priority": 2, "task_type": "INTERACTIVE", "deadline": 5.0}
    ]
  }'
```

---

## 🔌 API Reference

### Base URL

```
http://localhost:8080/api/v1
```

### 1. Synchronous Simulation

**Endpoint:** `POST /simulate`

Executes a workload directly and returns the result immediately.

**Request Payload:**

```json
{
  "settings": {
    "algorithm": "MLFQ",
    "quantum": 2.0,
    "context_switch_penalty": 0.1,
    "num_cores": 4
  },
  "tasks": [
    {
      "id": "t1",
      "arrival_time": 0.0,
      "burst_time": 10.5,
      "priority": 1,
      "task_type": "CPU_BOUND",
      "deadline": 15.0
    },
    {
      "id": "t2",
      "arrival_time": 2.0,
      "burst_time": 3.0,
      "priority": 2,
      "task_type": "INTERACTIVE",
      "deadline": 5.0
    }
  ]
}
```

**Response (200 OK):**

```json
{
  "workload_id": "550e8400-e29b-41d4-a716-446655440000",
  "algorithm_used": "MLFQ",
  "avg_waiting_time": 4.25,
  "avg_turnaround_time": 7.75,
  "throughput": 0.129,
  "total_time": 15.5,
  "task_results": [
    {
      "id": "t1",
      "start_time": 0.0,
      "end_time": 10.5,
      "waiting_time": 0.0,
      "turnaround_time": 10.5
    }
  ]
}
```

**Status Codes:**

- `200 OK` – Simulation completed successfully
- `400 Bad Request` – Invalid payload or schema violation
- `500 Internal Server Error` – Execution failure

---

### 2. Asynchronous Execution

**Endpoint:** `POST /simulate/async`

Pushes large workloads to Redis queue for background processing. Returns immediately with a `job_id`.

**Request Payload:**

```json
{
  "settings": {
    "algorithm": "EDF",
    "quantum": 1.5,
    "context_switch_penalty": 0.05,
    "num_cores": 8
  },
  "tasks": [
    {
      "id": "t1",
      "arrival_time": 0.0,
      "burst_time": 5.0,
      "priority": 1,
      "deadline": 10.0
    },
    {
      "id": "t2",
      "arrival_time": 1.0,
      "burst_time": 3.0,
      "priority": 2,
      "deadline": 8.0
    }
  ]
}
```

**Response (202 Accepted):**

```json
{
  "job_id": "job_12345",
  "status": "QUEUED",
  "message": "Workload submitted. Check status with GET /results/job_12345"
}
```

**Poll for Results:**

```bash
curl http://localhost:8080/api/v1/results/job_12345
```

**Result Response (200 OK):**

```json
{
  "job_id": "job_12345",
  "status": "COMPLETED",
  "result": {
    "workload_id": "...",
    "algorithm_used": "EDF",
    "avg_turnaround_time": 6.5,
    ...
  }
}
```

**Status Codes:**

- `202 Accepted` – Job queued successfully
- `200 OK` – Job results ready
- `202 Accepted` – Job still processing
- `404 Not Found` – Job ID does not exist

---

### 3. Parallel Comparison

**Endpoint:** `POST /compare`

Executes all supported algorithms (FCFS, SJF, RR, MLFQ, EDF) concurrently and returns a ranked leaderboard.

**Request Payload:**

```json
{
  "settings": {
    "quantum": 2.0,
    "context_switch_penalty": 0.1,
    "num_cores": 4
  },
  "tasks": [
    {
      "id": "t1",
      "arrival_time": 0.0,
      "burst_time": 10.5,
      "priority": 1,
      "deadline": 15.0
    },
    {
      "id": "t2",
      "arrival_time": 2.0,
      "burst_time": 3.0,
      "priority": 2,
      "deadline": 5.0
    }
  ]
}
```

**Response (200 OK):**

```json
{
  "workload_id": "wl_abc123",
  "results": [
    {
      "rank": 1,
      "algorithm": "EDF",
      "avg_turnaround_time": 6.25,
      "avg_waiting_time": 3.25,
      "throughput": 0.16,
      "total_time": 12.5
    },
    {
      "rank": 2,
      "algorithm": "MLFQ",
      "avg_turnaround_time": 7.5,
      "avg_waiting_time": 4.5,
      "throughput": 0.13,
      "total_time": 15.0
    }
  ]
}
```

---

### 4. Intelligent Recommendation

**Endpoint:** `POST /recommend`

Performs $O(N)$ static analysis on your workload and recommends the best algorithm based on heuristics.

**Request Payload:**

```json
{
  "tasks": [
    {
      "id": "t1",
      "arrival_time": 0.0,
      "burst_time": 50.0,
      "priority": 1,
      "deadline": 100.0
    },
    {
      "id": "t2",
      "arrival_time": 5.0,
      "burst_time": 2.0,
      "priority": 2,
      "deadline": 10.0
    }
  ]
}
```

**Response (200 OK):**

```json
{
  "recommended_algorithm": "EDF",
  "confidence": 0.92,
  "reasoning": "Hard deadlines detected. EDF guarantees deadline feasibility.",
  "analysis": {
    "has_hard_deadlines": true,
    "interactive_to_batch_ratio": 0.3,
    "deadline_tightness": 0.15,
    "priority_variance": 0.7
  }
}
```

---

### 5. Telemetry / Metrics

**Endpoint:** `GET /metrics`

Exposes Prometheus-formatted metrics for monitoring and alerting.

**Sample Output:**

```
# HELP schedulrx_simulation_duration_seconds Simulation execution time in seconds
# TYPE schedulrx_simulation_duration_seconds histogram
schedulrx_simulation_duration_seconds_bucket{algorithm="MLFQ",le="0.001"} 45
schedulrx_simulation_duration_seconds_bucket{algorithm="MLFQ",le="0.01"} 132
schedulrx_simulation_duration_seconds_sum{algorithm="MLFQ"} 2.45
schedulrx_simulation_duration_seconds_count{algorithm="MLFQ"} 200

# HELP schedulrx_jobs_queued_current Current number of queued async jobs
# TYPE schedulrx_jobs_queued_current gauge
schedulrx_jobs_queued_current 12

# HELP schedulrx_api_requests_total Total API requests
# TYPE schedulrx_api_requests_total counter
schedulrx_api_requests_total{endpoint="/simulate",method="POST"} 1000
```

**Visualization:** Open [http://localhost:9090](http://localhost:9090) to access Prometheus dashboards.

---

## 📋 Configuration

### Environment Variables

Set these in your `.env` file or docker-compose.yml:

| Variable           | Default                                                                                    | Description                               |
| ------------------ | ------------------------------------------------------------------------------------------ | ----------------------------------------- |
| `DATABASE_URL`     | `host=postgres user=postgres password=postgres dbname=schedulrx port=5432 sslmode=disable` | PostgreSQL connection string              |
| `REDIS_URL`        | `redis:6379`                                                                               | Redis broker address                      |
| `ENGINE_PATH`      | `/app/engine_main`                                                                         | Path to C++ engine executable             |
| `GIN_MODE`         | `debug`                                                                                    | Gin framework mode (`debug` or `release`) |
| `WORKER_POOL_SIZE` | `4`                                                                                        | Number of concurrent C++ workers          |
| `QUEUE_MAX_RETRY`  | `3`                                                                                        | Max retries for failed async jobs         |

### Workload Settings Schema

| Field                    | Type   | Range                                          | Description                      |
| ------------------------ | ------ | ---------------------------------------------- | -------------------------------- |
| `algorithm`              | string | `FCFS`, `SJF`, `RR`, `MLFQ`, `EDF`, `ADAPTIVE` | Scheduling algorithm             |
| `quantum`                | float  | > 0                                            | Time slice for RR (milliseconds) |
| `context_switch_penalty` | float  | 0.0 to 1.0                                     | Overhead for context switching   |
| `num_cores`              | int    | 1 to 16                                        | Number of CPU cores to simulate  |

### Task Schema

| Field          | Type   | Required | Description                             |
| -------------- | ------ | -------- | --------------------------------------- |
| `id`           | string | ✓        | Unique task identifier                  |
| `arrival_time` | float  | ✓        | When task enters system (ms)            |
| `burst_time`   | float  | ✓        | CPU time needed (ms)                    |
| `priority`     | int    | ✓        | Priority level (1-10, lower = higher)   |
| `task_type`    | string | ✗        | `CPU_BOUND`, `INTERACTIVE`, `I_O_BOUND` |
| `deadline`     | float  | ✗        | Hard deadline (ms, for EDF)             |

---

## 📁 Project Structure

```
schedulrx/
├── README.md                           # This file
├── Dockerfile                          # 3-stage Docker build
├── docker-compose.yml                  # Orchestration config
│
├── api-go/                             # Go Control Plane
│   ├── cmd/
│   │   └── server/
│   │       └── main.go                 # API entrypoint
│   ├── internal/
│   │   ├── handlers/
│   │   │   ├── async.go                # Async job handlers
│   │   │   ├── compare.go              # Parallel comparison logic
│   │   │   ├── recommend.go            # Heuristic recommendation
│   │   │   └── simulate.go             # Sync simulation handler
│   │   ├── models/
│   │   │   └── schema.go               # GORM models & JSON structs
│   │   ├── queue/
│   │   │   └── redis.go                # Redis job queue implementation
│   │   ├── telemetry/
│   │   │   └── metrics.go              # Prometheus metrics
│   │   └── worker/
│   │       ├── engine.go               # C++ process bridge
│   │       └── pool.go                 # Goroutine worker pool
│   ├── go.mod                          # Module definition
│   └── go.sum                          # Dependency checksums
│
├── engine-cpp/                         # C++ Data Plane
│   ├── src/
│   │   └── main.cpp                    # Engine entrypoint (JSON I/O)
│   ├── include/
│   │   ├── models.hpp                  # JSON serialization & task models
│   │   ├── scheduler_factory.hpp       # Factory pattern for algorithms
│   │   ├── algorithms/
│   │   │   ├── fcfs.hpp                # First-Come-First-Served
│   │   │   ├── sjf.hpp                 # Shortest Job First
│   │   │   ├── rr.hpp                  # Round Robin
│   │   │   ├── mlfq.hpp                # Multi-Level Feedback Queue
│   │   │   ├── edf.hpp                 # Earliest Deadline First
│   │   │   └── adaptive.hpp            # Adaptive with EWMA
│   │   └── multicore/
│   │       └── smp_orchestrator.hpp    # SMP work-stealing scheduler
│   ├── CMakeLists.txt                  # Build configuration
│   ├── tests/                          # Unit tests
│   └── third_party/                    # External dependencies (nlohmann/json)
│
├── docs/
│   └── schema_v1.json                  # OpenAPI 3.0 schema
│
├── deployments/
│   ├── docker/
│   └── k8s/                            # Kubernetes manifests (future)
│
└── scripts/                            # Utility scripts
    ├── benchmark.sh                    # Run benchmarks
    ├── setup.sh                        # Environment setup
    └── migrate.sh                      # Database migrations
```

---

## 🛠 Development Setup

### Prerequisites (Local Development)

- **Go 1.25+**
- **C++17-compliant compiler** (GCC 10+, Clang 12+)
- **CMake 3.10+**
- **PostgreSQL 15+**
- **Redis 7.0+**

### 1. Build the C++ Engine

```bash
cd engine-cpp
mkdir -p build && cd build
cmake ..
make -j$(nproc)

# Verify build
./engine_main < sample_input.json
```

### 2. Start Dependencies (PostgreSQL & Redis)

```bash
# Using Docker
docker run -d --name postgres -e POSTGRES_PASSWORD=postgres \
  -p 5432:5432 postgres:15-alpine

docker run -d --name redis -p 6379:6379 redis:7-alpine

# Or using your system package manager
```

### 3. Initialize Database

```bash
export DATABASE_URL="host=localhost user=postgres password=postgres dbname=schedulrx port=5432 sslmode=disable"

# Run GORM auto-migrations
cd api-go
go run cmd/server/main.go
```

### 4. Run the Go API

```bash
cd api-go

# Set environment variables
export ENGINE_PATH=../engine-cpp/build/engine_main
export DATABASE_URL="host=localhost user=postgres password=postgres dbname=schedulrx port=5432 sslmode=disable"
export REDIS_URL="localhost:6379"

# Run the server
go run cmd/server/main.go

# Server runs on http://localhost:8080
```

### 5. Running Tests

```bash
# Go unit tests
cd api-go
go test ./...

# C++ unit tests
cd engine-cpp/build
ctest --output-on-failure
```

---

## 🐳 Docker Deployment

### Build All Components

```bash
docker-compose build
```

### Start the Stack

```bash
docker-compose up -d
```

### Monitor Logs

```bash
docker-compose logs -f schedulrx-api
docker-compose logs -f postgres
docker-compose logs -f redis
```

### Stop the Stack

```bash
docker-compose down -v  # -v removes volumes
```

### Access Services

| Service    | URL                   | Port |
| ---------- | --------------------- | ---- |
| API        | http://localhost:8080 | 8080 |
| PostgreSQL | localhost             | 5432 |
| Redis      | localhost             | 6379 |
| Prometheus | http://localhost:9090 | 9090 |

---

## 📊 Benchmark Examples

### Example 1: CPU-Bound Workload

**Scenario:** 5 CPU-intensive tasks, no deadlines.

**Expected Winner:** SJF or MLFQ (minimize response time)

```bash
curl -X POST http://localhost:8080/api/v1/compare \
  -H "Content-Type: application/json" \
  -d '{
    "settings": {"quantum": 2.0, "num_cores": 4},
    "tasks": [
      {"id": "cpu1", "arrival_time": 0, "burst_time": 20, "priority": 1},
      {"id": "cpu2", "arrival_time": 0, "burst_time": 15, "priority": 1},
      {"id": "cpu3", "arrival_time": 0, "burst_time": 10, "priority": 1},
      {"id": "cpu4", "arrival_time": 5, "burst_time": 8, "priority": 1},
      {"id": "cpu5", "arrival_time": 10, "burst_time": 5, "priority": 1}
    ]
  }'
```

### Example 2: Mixed Workload with Deadlines

**Scenario:** Interactive + batch tasks with hard deadlines.

**Expected Winner:** EDF (guarantees deadline feasibility)

```bash
curl -X POST http://localhost:8080/api/v1/compare \
  -H "Content-Type: application/json" \
  -d '{
    "settings": {"quantum": 1.0, "num_cores": 8},
    "tasks": [
      {"id": "interactive1", "arrival_time": 0, "burst_time": 2, "priority": 1, "deadline": 5, "task_type": "INTERACTIVE"},
      {"id": "batch1", "arrival_time": 0, "burst_time": 50, "priority": 5, "deadline": 100, "task_type": "CPU_BOUND"},
      {"id": "interactive2", "arrival_time": 3, "burst_time": 1, "priority": 1, "deadline": 8, "task_type": "INTERACTIVE"}
    ]
  }'
```

---

## 🔍 Troubleshooting

### Issue: `Failed to connect to Redis`

**Solution:**

```bash
# Verify Redis is running
redis-cli ping  # Should return PONG

# Check connection string
echo $REDIS_URL
```

### Issue: `C++ Engine not found`

**Solution:**

```bash
# Set correct ENGINE_PATH
export ENGINE_PATH=$(pwd)/engine-cpp/build/engine_main
# Verify file exists and is executable
ls -la $ENGINE_PATH
```

### Issue: `Database connection timeout`

**Solution:**

```bash
# Check PostgreSQL is running
pg_isready -h localhost -p 5432

# Verify credentials
psql -h localhost -U postgres -d schedulrx
```

### Issue: `Simulation timeout or hangs`

**Solution:**

```bash
# Increase worker timeout in Go
export WORKER_TIMEOUT=30s

# Check available system resources
free -h  # Memory
top     # CPU usage
```

---

## 📈 Performance Characteristics

| Metric                 | Value            | Notes                  |
| ---------------------- | ---------------- | ---------------------- |
| API Response Latency   | <100ms           | For 100-task workload  |
| C++ Engine Throughput  | 10,000 tasks/sec | On 4-core system       |
| Work-Stealing Overhead | <2%              | Per rebalance          |
| Memory per Engine      | ~50MB            | Baseline + task buffer |
| Max Concurrent Workers | 16               | Configurable           |

---

## 🤝 Contributing

We welcome contributions! Please follow these guidelines:

1. **Fork** the repository
2. **Create** a feature branch: `git checkout -b feature/my-feature`
3. **Commit** your changes: `git commit -m "Add my feature"`
4. **Push** to the branch: `git push origin feature/my-feature`
5. **Open a Pull Request** with a clear description

### Development Workflow

- Ensure code passes tests: `go test ./...` and `ctest`
- Follow Go conventions (gofmt, golint)
- Follow C++ conventions (clang-format, clang-tidy)
- Add tests for new features
- Update documentation

---

## 📝 License

SchedulrX is licensed under the **MIT License** – see the [LICENSE](LICENSE) file for details.

---

## 🙋 Support & Questions

- **Issues:** [GitHub Issues](https://github.com/Shivam-1827/schedulrx/issues)
- **Discussions:** [GitHub Discussions](https://github.com/Shivam-1827/schedulrx/discussions)
- **Email:** [shivam@schedulrx.dev](mailto:shivam@schedulrx.dev)

---

## 🎓 Academic References

If you use SchedulrX in your research, please cite:

```bibtex
@software{schedulrx2024,
  title={SchedulrX: A Distributed CPU Scheduling Simulation Platform},
  author={Shivam, et al.},
  year={2024},
  url={https://github.com/Shivam-1827/schedulrx}
}
```

---

**Built with ❤️ for the systems engineering community.**
