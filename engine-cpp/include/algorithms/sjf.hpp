#pragma once
#include "../scheduler_factory.hpp"
#include <algorithm>
#include <queue>

namespace schedulrx::core {

    class SJFScheduler : public IScheduler {
    public:
        models::SimulationResult simulate(
            std::vector<models::Process> processes, 
            const models::WorkloadSettings& settings,
            const std::string& workload_id
        ) override {
            
            models::SimulationResult result;
            result.workload_id = workload_id;
            result.algorithm_used = "SJF";
            if (processes.empty()) return result;

            // Sort by arrival time initially
            std::sort(processes.begin(), processes.end(), 
                [](const models::Process& a, const models::Process& b) {
                    return a.arrival_time < b.arrival_time;
                });

            // Min-Heap to quickly find the shortest burst time among available processes
            auto cmp = [](const models::Process* a, const models::Process* b) {
                if (a->burst_time == b->burst_time) return a->arrival_time > b->arrival_time;
                return a->burst_time > b->burst_time;
            };
            std::priority_queue<models::Process*, std::vector<models::Process*>, decltype(cmp)> ready_queue(cmp);

            double current_time = 0.0;
            size_t completed = 0;
            size_t index = 0;
            size_t n = processes.size();

            double total_waiting = 0.0, total_turnaround = 0.0, total_response = 0.0;

            while (completed < n) {
                // Push all processes that have arrived into the ready queue
                while (index < n && processes[index].arrival_time <= current_time) {
                    ready_queue.push(&processes[index]);
                    index++;
                }

                if (ready_queue.empty()) {
                    // CPU is idle, jump time to the next arriving process
                    current_time = processes[index].arrival_time;
                    continue;
                }

                // Execute the shortest job
                models::Process* p = ready_queue.top();
                ready_queue.pop();

                p->start_time = current_time;
                p->response_time = p->start_time - p->arrival_time;
                p->waiting_time = p->response_time; // Non-preemptive

                double start_exec = current_time;
                current_time += p->burst_time + settings.context_switch_penalty;
                
                p->completion_time = current_time;
                p->turnaround_time = p->completion_time - p->arrival_time;

                total_waiting += p->waiting_time;
                total_turnaround += p->turnaround_time;
                total_response += p->response_time;

                result.gantt_chart.emplace_back(p->id, start_exec, start_exec + p->burst_time);
                completed++;
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