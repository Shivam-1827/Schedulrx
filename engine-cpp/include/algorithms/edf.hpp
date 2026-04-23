#pragma once
#include "../scheduler_factory.hpp"
#include <algorithm>
#include <queue>

namespace schedulrx::core {

    class EDFScheduler : public IScheduler {
    public:
        models::SimulationResult simulate(
            std::vector<models::Process> processes, 
            const models::WorkloadSettings& settings,
            const std::string& workload_id
        ) override {
            
            models::SimulationResult result;
            result.workload_id = workload_id;
            result.algorithm_used = "EDF";
            if (processes.empty()) return result;

            std::sort(processes.begin(), processes.end(), 
                [](const models::Process& a, const models::Process& b) { return a.arrival_time < b.arrival_time; });

            for (auto& p : processes) p.remaining_time = p.burst_time;

            // Min-Heap based on Earliest Deadline
            auto cmp = [](const models::Process* a, const models::Process* b) {
                if (a->deadline == b->deadline) return a->arrival_time > b->arrival_time;
                return a->deadline > b->deadline;
            };
            std::priority_queue<models::Process*, std::vector<models::Process*>, decltype(cmp)> ready_queue(cmp);

            double current_time = 0.0;
            size_t completed = 0;
            size_t index = 0;
            size_t n = processes.size();

            while (completed < n) {
                // 1. Enqueue newly arrived processes
                while (index < n && processes[index].arrival_time <= current_time) {
                    ready_queue.push(&processes[index]);
                    index++;
                }

                if (ready_queue.empty()) {
                    current_time = processes[index].arrival_time;
                    continue;
                }

                // 2. Pick process with earliest deadline
                models::Process* p = ready_queue.top();
                ready_queue.pop();

                if (p->remaining_time == p->burst_time) {
                    p->start_time = current_time;
                    p->response_time = current_time - p->arrival_time;
                }

                // 3. Determine time to execute (until completion OR next process arrival)
                double time_to_next_arrival = (index < n) ? (processes[index].arrival_time - current_time) : p->remaining_time;
                double execute_time = std::min(p->remaining_time, time_to_next_arrival);
                // In pure EDF, we only preempt if the new arrival has an earlier deadline. 
                // We simulate this by checking at the next arrival boundary.
                
                double start_exec = current_time;
                current_time += execute_time;
                p->remaining_time -= execute_time;

                result.gantt_chart.emplace_back(p->id, start_exec, current_time);

                // 4. Preemption & Context Switch logic
                if (p->remaining_time > 0) {
                    // Preempted by a new arrival
                    current_time += settings.context_switch_penalty;
                    p->context_switches++;
                    ready_queue.push(p);
                } else {
                    p->completion_time = current_time;
                    p->turnaround_time = p->completion_time - p->arrival_time;
                    p->waiting_time = p->turnaround_time - p->burst_time;
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