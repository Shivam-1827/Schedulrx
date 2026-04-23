#pragma once
#include "../scheduler_factory.hpp"
#include <algorithm>

namespace schedulrx::core {

    class FCFSScheduler : public IScheduler {
    public:
        models::SimulationResult simulate(
            std::vector<models::Process> processes, 
            const models::WorkloadSettings& settings,
            const std::string& workload_id
        ) override {
            
            models::SimulationResult result;
            result.workload_id = workload_id;
            result.algorithm_used = "FCFS";

            if (processes.empty()) return result;

            // 1. Sort by arrival time (FCFS logic)
            std::sort(processes.begin(), processes.end(), 
                [](const models::Process& a, const models::Process& b) {
                    return a.arrival_time < b.arrival_time;
                });

            double current_time = 0.0;
            double total_waiting = 0.0, total_turnaround = 0.0, total_response = 0.0;

            // 2. Execution Loop
            for (auto& p : processes) {
                // Handle CPU idle time if process hasn't arrived yet
                if (current_time < p.arrival_time) {
                    current_time = p.arrival_time;
                }

                p.start_time = current_time;
                p.response_time = p.start_time - p.arrival_time;
                p.waiting_time = p.response_time; // True for non-preemptive

                // Simulate execution
                double start_exec = current_time;
                current_time += p.burst_time;
                current_time += settings.context_switch_penalty; // Apply penalty
                
                p.completion_time = current_time;
                p.turnaround_time = p.completion_time - p.arrival_time;

                // Accumulate totals for averages
                total_waiting += p.waiting_time;
                total_turnaround += p.turnaround_time;
                total_response += p.response_time;

                // Log to Gantt chart
                result.gantt_chart.emplace_back(p.id, start_exec, start_exec + p.burst_time);
            }

            // 3. Compute Final Metrics
            size_t n = processes.size();
            result.total_time = current_time;
            result.throughput = (n / result.total_time) * 1000; // Tasks per 1000 time units
            result.avg_waiting_time = total_waiting / n;
            result.avg_turnaround_time = total_turnaround / n;
            result.avg_response_time = total_response / n;

            return result;
        }
    };
}