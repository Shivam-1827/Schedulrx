package handlers

import (
	"net/http"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/google/uuid"

	"github.com/Shivam-1827/Schedulrx/internal/models"
	"github.com/Shivam-1827/Schedulrx/internal/queue"
)

type AsyncHandler struct {
	Queue *queue.RedisQueue
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
		"message": "Job successfully queued",
		"job_id":  jobID,
		"status":  queue.StatusQueued,
		"poll_url": "/api/v1/results/" + jobID,
	})
}

// GET /results/:job_id
func (h *AsyncHandler) PollResult(c *gin.Context) {
	jobID := c.Param("job_id")

	job, err := h.Queue.GetState(c.Request.Context(), jobID)
	if err != nil {
		c.JSON(http.StatusNotFound, gin.H{"error": "Job not found or expired"})
		return
	}

	response := gin.H{
		"job_id": job.JobID,
		"status": job.Status,
	}

	if job.Status == queue.StatusCompleted {
		response["result"] = job.Result
	} else if job.Status == queue.StatusFailed {
		response["error"] = job.Error
	}

	c.JSON(http.StatusOK, response)
}