#pragma once
#include "../scheduler_factory.hpp"
#include <algorithm>
#include <deque>
#include <vector>

namespace schedulrx::core {

    struct CPUCore {
        int id;
        double current_time = 0.0;
        std::deque<models::Process*> local_queue;
        models::Process* active_process = nullptr;
    };

    class SMPWorkStealingScheduler : public IScheduler {
    public:
        models::SimulationResult simulate(
            std::vector<models::Process> processes, 
            const models::WorkloadSettings& settings,
            const std::string& workload_id
        ) override {
            
            models::SimulationResult result;
            result.workload_id = workload_id;
            result.algorithm_used = "SMP_WORK_STEALING";
            
            int num_cores = settings.num_cores > 0 ? settings.num_cores : 1;
            std::vector<CPUCore> cores(num_cores);
            for(int i=0; i<num_cores; i++) cores[i].id = i;

            if (processes.empty()) return result;

            // Sort by arrival
            std::sort(processes.begin(), processes.end(), 
                [](const models::Process& a, const models::Process& b) { return a.arrival_time < b.arrival_time; });

            for (auto& p : processes) p.remaining_time = p.burst_time;

            size_t index = 0;
            size_t n = processes.size();
            size_t completed = 0;
            double global_time = 0.0;

            // Event-based simulation loop to avoid O(tasks x time) behavior
            while (completed < n) {
                
                // 1. Distribute new arrivals (Round Robin to core queues)
                while (index < n && processes[index].arrival_time <= global_time) {
                    int target_core = index % num_cores; // Initial Load Balancing
                    cores[target_core].local_queue.push_front(&processes[index]);
                    index++;
                }

                // 2. Work Stealing Phase
                for (int i = 0; i < num_cores; i++) {
                    if (cores[i].local_queue.empty() && cores[i].active_process == nullptr) {
                        // Core is idle, look for the busiest core to steal from
                        int busiest_core = -1;
                        size_t max_q = 0;
                        for (int j = 0; j < num_cores; j++) {
                            if (cores[j].local_queue.size() > max_q) {
                                max_q = cores[j].local_queue.size();
                                busiest_core = j;
                            }
                        }

                        // Steal from the BACK of the victim's queue (cache cold tasks)
                        if (busiest_core != -1 && max_q > 1) {
                            models::Process* stolen_task = cores[busiest_core].local_queue.back();
                            cores[busiest_core].local_queue.pop_back();
                            cores[i].local_queue.push_front(stolen_task);
                            // Apply migration penalty (simulating L1/L2 cache miss)
                            global_time += settings.context_switch_penalty * 2.0; 
                        }
                    }
                }

                // 3. Execution Phase (Dynamic Time Step calculation)
                double time_step = 1e9;
                bool cpu_active = false;

                // Pass 1: Activate cores and find shortest remaining runtime among active tasks
                for (int i = 0; i < num_cores; i++) {
                    if (cores[i].active_process == nullptr && !cores[i].local_queue.empty()) {
                        cores[i].active_process = cores[i].local_queue.front();
                        cores[i].local_queue.pop_front();

                        if (cores[i].active_process->start_time == -1.0) {
                            cores[i].active_process->start_time = global_time;
                            cores[i].active_process->response_time = global_time - cores[i].active_process->arrival_time;
                        }
                    }

                    if (cores[i].active_process != nullptr) {
                        cpu_active = true;
                        time_step = std::min(time_step, cores[i].active_process->remaining_time);
                    }
                }

                // Pass 2: If a new task arrives earlier, execute only until that event
                if (index < n && processes[index].arrival_time < (global_time + time_step)) {
                    time_step = processes[index].arrival_time - global_time;
                }

                // Pass 3: Execute each active core for the chosen time slice
                for (int i = 0; i < num_cores; i++) {
                    if (cores[i].active_process != nullptr) {
                        models::Process* p = cores[i].active_process;
                        p->remaining_time -= time_step;
                        cores[i].current_time += time_step;

                        result.gantt_chart.emplace_back(p->id + "_core" + std::to_string(i), global_time, global_time + time_step);

                        if (p->remaining_time <= 0.00001) {
                            p->completion_time = global_time + time_step;
                            p->turnaround_time = p->completion_time - p->arrival_time;
                            p->waiting_time = p->turnaround_time - p->burst_time;
                            cores[i].active_process = nullptr;
                            completed++;
                        }
                    }
                }

                // 4. Advance Time
                if (!cpu_active && index < n) {
                    global_time = processes[index].arrival_time; // Fast forward to next arrival
                } else {
                    global_time += time_step;
                }
            }

            // Aggregate Metrics
            double total_waiting = 0.0, total_turnaround = 0.0;
            for (const auto& p : processes) {
                total_waiting += p.waiting_time;
                total_turnaround += p.turnaround_time;
            }

            result.total_time = global_time;
            result.throughput = (n / result.total_time) * 1000;
            result.avg_waiting_time = total_waiting / n;
            result.avg_turnaround_time = total_turnaround / n;

            return result;
        }
    };
}