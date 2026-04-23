#pragma once
#include "models.hpp"
#include <memory>
#include <stdexcept>

// Include all algorithm headers
#include "algorithms/fcfs.hpp"
#include "algorithms/sjf.hpp"
#include "algorithms/rr.hpp"
#include "algorithms/mlfq.hpp"

namespace schedulrx::core {

    class IScheduler {
    public:
        virtual ~IScheduler() = default;
        virtual models::SimulationResult simulate(
            std::vector<models::Process> processes, 
            const models::WorkloadSettings& settings,
            const std::string& workload_id
        ) = 0;
    };

    class SchedulerFactory {
    public:
        static std::unique_ptr<IScheduler> create(const std::string& algorithm_name) {
            if (algorithm_name == "FCFS") return std::make_unique<FCFSScheduler>();
            if (algorithm_name == "SJF")  return std::make_unique<SJFScheduler>();
            if (algorithm_name == "RR")   return std::make_unique<RRScheduler>();
            if (algorithm_name == "MLFQ") return std::make_unique<MLFQScheduler>();
            
            throw std::invalid_argument("Unknown scheduling algorithm requested: " + algorithm_name);
        }
    };
}