#include <iostream>
#include <nlohmann/json.hpp>
#include "models.hpp"
#include "scheduler_factory.hpp"
#include "algorithms/fcfs.hpp" // Include your algorithms here

using json = nlohmann::json;
using namespace schedulrx;

// Helper to output standard JSON errors to Go
void output_error(const std::string& message) {
    json err;
    err["status"] = "error";
    err["message"] = message;
    std::cout << err.dump() << std::endl;
}

int main() {
    // 1. Fast I/O for pipe streaming
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    try {
        // 2. Read and Parse JSON
        json input_json;
        std::cin >> input_json;

        // 3. Extract Payload securely
        std::string workload_id = input_json.at("workload_id").get<std::string>();
        auto settings = input_json.at("settings").get<models::WorkloadSettings>();
        auto processes = input_json.at("tasks").get<std::vector<models::Process>>();

        // 4. Instantiate via Factory and Execute
        auto scheduler = core::SchedulerFactory::create(settings.algorithm);
        models::SimulationResult result = scheduler->simulate(processes, settings, workload_id);

        // 5. Output Result as JSON
        json output_json = result; // Auto-serialized via macro in models.hpp
        output_json["status"] = "success";
        
        std::cout << output_json.dump() << "\n";

    } catch (const json::exception& e) {
        output_error(std::string("JSON Parsing Error: ") + e.what());
        return 1;
    } catch (const std::exception& e) {
        output_error(std::string("Engine Error: ") + e.what());
        return 1;
    }

    return 0;
}