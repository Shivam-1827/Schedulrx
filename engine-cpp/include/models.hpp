#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace schedulrx::models{

    enum class TaskType {CPU_BOUND, IO_BOUND, INTERACTIVE};

    NLOHMANN_JSON_SERIALIZE_ENUM(TaskType, {
        {TaskType::CPU_BOUND, "CPU_BOUND"},
        {TaskType::IO_BOUND, "IO_BOUND"},
        {TaskType::INTERACTIVE, "INTERACTIVE"}
    });

    struct Process {
        std::string id;
        double arrival_time;
        double burst_time;
        int priority;
        TaskType task_type;

        double remaining_time = 0.0;
        double start_time = -1.0;
        double completion_time = 0.0;
        double waiting_time = 0.0;
        double response_time = -1.0;
        double turnaround_time = 0.0;
        int context_switches = 0;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(Process, id, arrival_time, burst_time, priority, task_type)
    };

    struct WorkloadSettings {
        std::string algorithm;
        double quantum = 0.0;
        double context_switch_penalty = 0.0;
        NLOHMANN_DEFINE_TYPE_INTRUSIVE(WorkloadSettings, algorithm, quantum, context_switch_penalty)
    };

    struct SimulationResult {
        std::string workload_id;
        std::string algorithm_used;
        double avg_waiting_time = 0.0;
        double avg_turnaround_time = 0.0;
        double avg_response_time = 0.0;
        double throughput = 0.0;
        double total_time = 0.0;

        std::vector<std::tuple<std::string, double, double>> gantt_chart;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(SimulationResult, workload_id, algorithm_used, avg_waiting_time, avg_turnaround_time, avg_response_time, throughput, total_time, gantt_chart)
    };

}