#pragma once
#include "models.hpp"
#include <memory>
#include <stdexcept>
#include <vector>

namespace schedulrx::core {

    // 1. DEFINE THE INTERFACE FIRST
    class IScheduler {
    public:
        virtual ~IScheduler() = default;
        virtual models::SimulationResult simulate(
            std::vector<models::Process> processes, 
            const models::WorkloadSettings& settings,
            const std::string& workload_id
        ) = 0;
    };
}

// 2. INCLUDE THE ALGORITHMS (They now know what IScheduler is)
#include "algorithms/fcfs.hpp"
#include "algorithms/sjf.hpp"
#include "algorithms/rr.hpp"
#include "algorithms/mlfq.hpp"
#include "algorithms/edf.hpp"
#include "algorithms/adaptive.hpp"
#include "multicore/smp_orchestrator.hpp"

namespace schedulrx::core {

    // 3. DEFINE THE FACTORY
    class SchedulerFactory {
    public:
        static std::unique_ptr<IScheduler> create(const std::string& algorithm_name) {
            if (algorithm_name == "FCFS") return std::make_unique<FCFSScheduler>();
            if (algorithm_name == "SJF")  return std::make_unique<SJFScheduler>();
            if (algorithm_name == "RR")   return std::make_unique<RRScheduler>();
            if (algorithm_name == "MLFQ") return std::make_unique<MLFQScheduler>();
            if (algorithm_name == "EDF")  return std::make_unique<EDFScheduler>();
            if (algorithm_name == "SMP")  return std::make_unique<SMPWorkStealingScheduler>();
            if (algorithm_name == "ADAPTIVE") return std::make_unique<AdaptiveScheduler>();
            
            throw std::invalid_argument("Unknown scheduling algorithm requested: " + algorithm_name);
        }
    };
}