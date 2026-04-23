#pragma once
#include "../scheduler_factory.hpp"
#include <algorithm>
#include <queue>
#include <map>

namespace schedulrx::core {

    class AdaptiveScheduler : public IScheduler {
    private:
        const double ALPHA = 0.5; // Exponential Smoothing Factor

    public:
        models::SimulationResult simulate(
            std::vector<models::Process> processes, 
            const models::WorkloadSettings& settings,
            const std::string& workload_id
        ) override {
            
            models::SimulationResult result;
            result.workload_id = workload_id;
            result.algorithm_used = "ADAPTIVE_HEURISTIC";
            if (processes.empty()) return result;

            std::sort(processes.begin(), processes.end(), 
                [](const models::Process& a, const models::Process& b) { return a.arrival_time < b.arrival_time; });

            // 1. Task 5.1: Burst Prediction State (tau)
            std::map<models::TaskType, double> tau_predictions = {
                {models::TaskType::CPU_BOUND, 10.0},
                {models::TaskType::IO_BOUND, 5.0},
                {models::TaskType::INTERACTIVE, 2.0}
            };

            for (auto& p : processes) p.remaining_time = p.burst_time;

            // We use a vector as a dynamic ready queue so we can sort by predicted burst
            std::vector<models::Process*> ready_queue;
            double current_time = 0.0;
            size_t completed = 0, index = 0, n = processes.size();

            auto pull_new_arrivals = [&]() {
                while (index < n && processes[index].arrival_time <= current_time) {
                    ready_queue.push_back(&processes[index]);
                    index++;
                }
            };

            while (completed < n) {
                pull_new_arrivals();

                if (ready_queue.empty()) {
                    current_time = processes[index].arrival_time;
                    continue;
                }

                // Sort queue based on PREDICTED burst time (Adaptive SJF hybrid)
                std::sort(ready_queue.begin(), ready_queue.end(), 
                    [&tau_predictions](const models::Process* a, const models::Process* b) {
                        return tau_predictions[a->task_type] < tau_predictions[b->task_type];
                    });

                models::Process* p = ready_queue.front();
                ready_queue.erase(ready_queue.begin()); // Pop front

                if (p->remaining_time == p->burst_time) {
                    p->start_time = current_time;
                    p->response_time = current_time - p->arrival_time;
                }

                // 2. Task 5.2: Dynamic Quantum Tuning
                int interactive_count = 0, cpu_count = 0;
                for (const auto& qp : ready_queue) {
                    if (qp->task_type == models::TaskType::INTERACTIVE) interactive_count++;
                    else if (qp->task_type == models::TaskType::CPU_BOUND) cpu_count++;
                }

                double dynamic_quantum = settings.quantum > 0 ? settings.quantum : 4.0;
                // If interactive tasks dominate, shrink quantum to guarantee fast response times
                if (interactive_count > cpu_count) {
                    dynamic_quantum *= 0.5; 
                } 
                // If batch/CPU tasks dominate, expand quantum to reduce context switch overhead
                else if (cpu_count > interactive_count) {
                    dynamic_quantum *= 2.0; 
                }

                double time_to_execute = std::min(p->remaining_time, dynamic_quantum);
                double start_exec = current_time;
                
                current_time += time_to_execute;
                p->remaining_time -= time_to_execute;
                p->context_switches++;

                result.gantt_chart.emplace_back(p->id, start_exec, current_time);

                pull_new_arrivals();

                if (p->remaining_time > 0) {
                    current_time += settings.context_switch_penalty;
                    ready_queue.push_back(p);
                } else {
                    p->completion_time = current_time;
                    p->turnaround_time = p->completion_time - p->arrival_time;
                    p->waiting_time = p->turnaround_time - p->burst_time;
                    
                    // Task 5.1: Update EWMA Prediction after task completes
                    double actual_burst = p->burst_time;
                    tau_predictions[p->task_type] = (ALPHA * actual_burst) + ((1.0 - ALPHA) * tau_predictions[p->task_type]);
                    
                    completed++;
                }
            }

            double total_waiting = 0.0, total_turnaround = 0.0;
            for (const auto& p : processes) {
                total_waiting += p.waiting_time;
                total_turnaround += p.turnaround_time;
            }

            result.total_time = current_time;
            result.throughput = (n / result.total_time) * 1000;
            result.avg_waiting_time = total_waiting / n;
            result.avg_turnaround_time = total_turnaround / n;

            return result;
        }
    };
}