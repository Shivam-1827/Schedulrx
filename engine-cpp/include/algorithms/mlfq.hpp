#pragma once
#include "../scheduler_factory.hpp"
#include <algorithm>
#include <queue>
#include <vector>

namespace schedulrx::core {

    class MLFQScheduler : public IScheduler {
    public:
        models::SimulationResult simulate(
            std::vector<models::Process> processes, 
            const models::WorkloadSettings& settings,
            const std::string& workload_id
        ) override {
            
            models::SimulationResult result;
            result.workload_id = workload_id;
            result.algorithm_used = "MLFQ";
            if (processes.empty()) return result;

            std::sort(processes.begin(), processes.end(), 
                [](const models::Process& a, const models::Process& b) { return a.arrival_time < b.arrival_time; });

            for (auto& p : processes) p.remaining_time = p.burst_time;

            // 3 Queues: Q0 (RR, q=2), Q1 (RR, q=4), Q2 (FCFS)
            std::vector<std::queue<models::Process*>> queues(3);
            double current_time = 0.0;
            size_t completed = 0;
            size_t index = 0;
            size_t n = processes.size();

            auto pull_new_arrivals = [&]() {
                while (index < n && processes[index].arrival_time <= current_time) {
                    queues[0].push(&processes[index]); // All processes start in highest priority Q0
                    index++;
                }
            };

            while (completed < n) {
                pull_new_arrivals();

                int active_q = -1;
                for (int i = 0; i < 3; ++i) {
                    if (!queues[i].empty()) { active_q = i; break; }
                }

                if (active_q == -1) {
                    current_time = processes[index].arrival_time;
                    continue; // CPU Idle
                }

                models::Process* p = queues[active_q].front();
                queues[active_q].pop();

                if (p->remaining_time == p->burst_time) {
                    p->start_time = current_time;
                    p->response_time = current_time - p->arrival_time;
                }

                // Determine quantum based on queue level
                double current_quantum = (active_q == 0) ? 2.0 : (active_q == 1) ? 4.0 : p->remaining_time;
                double time_to_execute = std::min(p->remaining_time, current_quantum);
                
                double start_exec = current_time;
                current_time += time_to_execute;
                p->remaining_time -= time_to_execute;
                p->context_switches++;

                result.gantt_chart.emplace_back(p->id, start_exec, current_time);

                pull_new_arrivals(); // Check if new processes arrived while executing

                if (p->remaining_time > 0) {
                    current_time += settings.context_switch_penalty;
                    // Demote if it used full quantum and isn't already in the lowest queue
                    int next_q = (time_to_execute == current_quantum && active_q < 2) ? active_q + 1 : active_q;
                    queues[next_q].push(p);
                } else {
                    p->completion_time = current_time;
                    p->turnaround_time = p->completion_time - p->arrival_time;
                    p->waiting_time = p->turnaround_time - p->burst_time;
                    completed++;
                }
            }

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