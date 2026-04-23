#pragma once
#include "../scheduler_factory.hpp"
#include <algorithm>
#include <queue>

namespace schedulrx::core {

    class RRScheduler : public IScheduler {
    public:
        models::SimulationResult simulate(
            std::vector<models::Process> processes, 
            const models::WorkloadSettings& settings,
            const std::string& workload_id
        ) override {
            
            models::SimulationResult result;
            result.workload_id = workload_id;
            result.algorithm_used = "RR";
            if (processes.empty() || settings.quantum <= 0) return result;

            std::sort(processes.begin(), processes.end(), 
                [](const models::Process& a, const models::Process& b) { return a.arrival_time < b.arrival_time; });

            // Initialize remaining times
            for (auto& p : processes) {
                p.remaining_time = p.burst_time;
            }

            std::queue<models::Process*> ready_queue;
            double current_time = 0.0;
            size_t completed = 0;
            size_t index = 0;
            size_t n = processes.size();

            // Push first process to kickstart
            if (n > 0) {
                current_time = processes[0].arrival_time;
                ready_queue.push(&processes[0]);
                index++;
            }

            while (completed < n) {
                if (ready_queue.empty()) {
                    current_time = processes[index].arrival_time;
                    ready_queue.push(&processes[index]);
                    index++;
                    continue;
                }

                models::Process* p = ready_queue.front();
                ready_queue.pop();

                // First time execution? Track response time
                if (p->remaining_time == p->burst_time) {
                    p->start_time = current_time;
                    p->response_time = current_time - p->arrival_time;
                }

                double time_to_execute = std::min(p->remaining_time, settings.quantum);
                double start_exec = current_time;
                
                current_time += time_to_execute;
                p->remaining_time -= time_to_execute;
                p->context_switches++;

                result.gantt_chart.emplace_back(p->id, start_exec, current_time);

                // VERY IMPORTANT: Add newly arrived processes BEFORE re-queueing the current one
                while (index < n && processes[index].arrival_time <= current_time) {
                    ready_queue.push(&processes[index]);
                    index++;
                }

                if (p->remaining_time > 0) {
                    current_time += settings.context_switch_penalty; // Penalty only on preempt
                    ready_queue.push(p);
                } else {
                    p->completion_time = current_time;
                    p->turnaround_time = p->completion_time - p->arrival_time;
                    p->waiting_time = p->turnaround_time - p->burst_time; // Mathematical waiting time
                    completed++;
                }
            }

            // Aggregate metrics
            double total_waiting = 0.0, total_turnaround = 0.0, total_response = 0.0;
            for (const auto& p : processes) {
                total_waiting += p.waiting_time;
                total_turnaround += p.turnaround_time;
                total_response += p.response_time;
            }

            result.total_time = current_time;
            result.throughput = (n / result.total_time) * 1000;
            result.avg_waiting_time = total_waiting / n;
            result.avg_turnaround_time = total_turnaround / n;
            result.avg_response_time = total_response / n;

            return result;
        }
    };
}