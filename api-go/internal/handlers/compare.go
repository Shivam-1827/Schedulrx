package handlers

import (
	"context"
	"net/http"
	"sort"
	"sync"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/google/uuid"

	"github.com/Shivam-1827/Schedulrx/internal/models"
	"github.com/Shivam-1827/Schedulrx/internal/worker"
)

type CompareHandler struct {
	Bridge *worker.EngineBridge
}

// POST /compare
func (h *CompareHandler) CompareAlgorithms(c *gin.Context) {
	var baseReq models.SimulationRequest
	if err := c.ShouldBindJSON(&baseReq); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "Invalid payload", "details": err.Error()})
		return
	}

	if baseReq.WorkloadID == "" {
		baseReq.WorkloadID = uuid.New().String()
	}

	// Algorithms we want to race against each other
	algorithms := []string{"FCFS", "SJF", "RR", "MLFQ"}
	
	// Channels and WaitGroups for concurrent execution
	var wg sync.WaitGroup
	resultChan := make(chan *models.SimulationResult, len(algorithms))
	errorChan := make(chan error, len(algorithms))

	// 1. Fan-Out: Launch a goroutine for each algorithm
	for _, algo := range algorithms {
		wg.Add(1)
		
		go func(algorithm string) {
			defer wg.Done()
			
			// Create a specific request for this algorithm
			req := baseReq
			req.Settings.Algorithm = algorithm
			
			// Use a bounded context to prevent hanging goroutines
			ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
			defer cancel()

			res, err := h.Bridge.Execute(ctx, req)
			if err != nil {
				errorChan <- err
				return
			}
			resultChan <- res
		}(algo)
	}

	// 2. Wait for all executions to finish
	wg.Wait()
	close(resultChan)
	close(errorChan)

	// Check if any errors occurred during the fan-out
	if len(errorChan) > 0 {
		err := <-errorChan
		c.JSON(http.StatusInternalServerError, gin.H{"error": "One or more simulations failed", "details": err.Error()})
		return
	}

	// 3. Fan-In: Collect and aggregate results
	var results []models.SimulationResult
	for res := range resultChan {
		results = append(results, *res)
	}

	// 4. Rank the algorithms based on Average Turnaround Time (lowest is best)
	sort.Slice(results, func(i, j int) bool {
		return results[i].AvgTurnaroundTime < results[j].AvgTurnaroundTime
	})

	// Optional: Flag the "Winner"
	winner := results[0].AlgorithmUsed

	c.JSON(http.StatusOK, gin.H{
		"workload_id": baseReq.WorkloadID,
		"tasks_count": len(baseReq.Tasks),
		"winner":      winner,
		"rankings":    results,
	})
}