package handlers

import (
	"net/http"

	"github.com/gin-gonic/gin"
	"github.com/Shivam-1827/Schedulrx/internal/models"
)

type RecommendHandler struct{}

// POST /recommend
func (h *RecommendHandler) RecommendAlgorithm(c *gin.Context) {
	var req models.SimulationRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "Invalid payload", "details": err.Error()})
		return
	}

	tasks := req.Tasks
	if len(tasks) == 0 {
		c.JSON(http.StatusBadRequest, gin.H{"error": "Workload contains no tasks"})
		return
	}

	// 1. Workload Profiling
	var hasDeadlines bool
	var interactiveCount, cpuBoundCount int
	var totalBurst float64

	for _, t := range tasks {
		totalBurst += t.BurstTime
		if t.Deadline > 0 {
			hasDeadlines = true
		}
		if t.TaskType == "INTERACTIVE" {
			interactiveCount++
		} else if t.TaskType == "CPU_BOUND" {
			cpuBoundCount++
		}
	}

	interactiveRatio := float64(interactiveCount) / float64(len(tasks))
	avgBurst := totalBurst / float64(len(tasks))

	// 2. Heuristic Decision Matrix
	var recommendation string
	var reasoning string

	if hasDeadlines {
		recommendation = "EDF"
		reasoning = "Workload contains hard deadlines. Earliest Deadline First (EDF) guarantees optimal real-time processing."
	} else if req.Settings.NumCores > 1 {
		recommendation = "SMP"
		reasoning = "Multi-core hardware detected. Symmetric Multiprocessing (SMP) with Work Stealing will prevent core idling."
	} else if interactiveRatio > 0.4 {
		recommendation = "ADAPTIVE"
		reasoning = "High ratio of interactive tasks detected. The Adaptive Heuristic will dynamically shrink the RR quantum to ensure responsive UI/IO handling."
	} else if avgBurst > 20.0 && cpuBoundCount > interactiveCount {
		recommendation = "FCFS"
		reasoning = "Workload is highly CPU-bound with long bursts. FCFS minimizes the context switch penalty."
	} else {
		recommendation = "MLFQ"
		reasoning = "Mixed uniform workload detected. Multi-Level Feedback Queue prevents starvation while balancing throughput."
	}

	c.JSON(http.StatusOK, gin.H{
		"workload_profile": gin.H{
			"total_tasks":       len(tasks),
			"interactive_ratio": interactiveRatio,
			"avg_burst_time":    avgBurst,
			"has_deadlines":     hasDeadlines,
		},
		"recommended_algorithm": recommendation,
		"reasoning":             reasoning,
		"next_action":           "/api/v1/simulate with the recommended algorithm",
	})
}