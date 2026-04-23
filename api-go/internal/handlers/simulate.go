package handlers

import (
	"net/http"

	"github.com/gin-gonic/gin"
	"github.com/google/uuid"
	"gorm.io/gorm"

	"github.com/Shivam-1827/Schedulrx/internal/models"
	"github.com/Shivam-1827/Schedulrx/internal/worker"
)

type APIHandler struct {
	DB     *gorm.DB
	Bridge *worker.EngineBridge
}

func (h *APIHandler) HealthCheck(c *gin.Context) {
	c.JSON(http.StatusOK, gin.H{"status": "orchestrator_online"})
}

func (h *APIHandler) Simulate(c *gin.Context) {
	var req models.SimulationRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "Invalid JSON payload", "details": err.Error()})
		return
	}

	// Generate a Workload ID if not provided
	if req.WorkloadID == "" {
		req.WorkloadID = uuid.New().String()
	}

	// 1. Delegate to the C++ Engine Worker
	result, err := h.Bridge.Execute(c.Request.Context(), req)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "Simulation execution failed", "details": err.Error()})
		return
	}

	// 2. Save Results to PostgreSQL
	record := models.RunRecord{
		Algorithm:         result.AlgorithmUsed,
		TotalTasks:        len(req.Tasks),
		AvgWaitingTime:    result.AvgWaitingTime,
		AvgTurnaroundTime: result.AvgTurnaroundTime,
		Throughput:        result.Throughput,
		TotalTime:         result.TotalTime,
	}

	if err := h.DB.Create(&record).Error; err != nil {
		// Log the error, but still return the simulation result to the user
		c.JSON(http.StatusMultiStatus, gin.H{
			"warning": "Simulation succeeded but failed to save to database",
			"result":  result,
		})
		return
	}

	// 3. Return Success
	c.JSON(http.StatusOK, gin.H{
		"message": "Simulation completed successfully",
		"record_id": record.ID,
		"data":    result,
	})
}