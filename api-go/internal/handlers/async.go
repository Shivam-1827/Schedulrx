package handlers

import (
	"net/http"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/google/uuid"
	"gorm.io/gorm"

	"github.com/Shivam-1827/Schedulrx/internal/models"
	"github.com/Shivam-1827/Schedulrx/internal/queue"
)

type AsyncHandler struct {
	Queue *queue.RedisQueue
	DB    *gorm.DB
}

// POST /simulate/async
func (h *AsyncHandler) SubmitJob(c *gin.Context) {
	var req models.SimulationRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "Invalid payload", "details": err.Error()})
		return
	}

	jobID := uuid.New().String()
	if req.WorkloadID == "" {
		req.WorkloadID = jobID // Sync workload ID if not provided
	}

	job := queue.AsyncJob{
		JobID:     jobID,
		Status:    queue.StatusQueued,
		Request:   req,
		CreatedAt: time.Now(),
	}

	if err := h.Queue.Enqueue(c.Request.Context(), job); err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "Failed to enqueue job"})
		return
	}

	c.JSON(http.StatusAccepted, gin.H{
		"message":  "Job successfully queued",
		"job_id":   jobID,
		"status":   queue.StatusQueued,
		"poll_url": "/api/v1/results/" + jobID,
	})
}

// GET /results/:job_id
func (h *AsyncHandler) PollResult(c *gin.Context) {
	jobID := c.Param("job_id")

	// 1. Try fast-path: Redis
	job, err := h.Queue.GetState(c.Request.Context(), jobID)
	if err == nil {
		response := gin.H{"job_id": job.JobID, "status": job.Status}
		if job.Status == queue.StatusCompleted {
			response["result"] = job.Result
		}
		if job.Status == queue.StatusFailed {
			response["error"] = job.Error
		}
		c.JSON(http.StatusOK, response)
		return
	}

	// 2. Try slow-path fallback: PostgreSQL (if it expired from Redis)
	var record models.RunRecord
	if err := h.DB.Where("id = ?", jobID).First(&record).Error; err == nil {
		c.JSON(http.StatusOK, gin.H{
			"job_id": jobID,
			"status": "COMPLETED", // If it's in the DB, it finished successfully
			"result": record,
		})
		return
	}

	c.JSON(http.StatusNotFound, gin.H{"error": "Job not found or expired"})
}
