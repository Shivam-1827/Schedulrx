package queue

import (
	"context"
	"encoding/json"
	"fmt"
	"time"

	"github.com/redis/go-redis/v9"
	"github.com/Shivam-1827/Schedulrx/internal/models"
)

const (
	QueueName      = "schedulrx:job_queue"
	JobStatusPrefix = "schedulrx:job_status:"
)

type JobStatus string

const (
	StatusQueued     JobStatus = "QUEUED"
	StatusProcessing JobStatus = "PROCESSING"
	StatusCompleted  JobStatus = "COMPLETED"
	StatusFailed     JobStatus = "FAILED"
)

// AsyncJob wraps the request with tracking metadata
type AsyncJob struct {
	JobID     string                   `json:"job_id"`
	Status    JobStatus                `json:"status"`
	Request   models.SimulationRequest `json:"request"`
	Result    *models.SimulationResult `json:"result,omitempty"`
	Error     string                   `json:"error,omitempty"`
	CreatedAt time.Time                `json:"created_at"`
}

type RedisQueue struct {
	client *redis.Client
}

func NewRedisQueue(client *redis.Client) *RedisQueue {
	return &RedisQueue{client: client}
}

// Enqueue pushes a job to the list and sets its initial status
func (rq *RedisQueue) Enqueue(ctx context.Context, job AsyncJob) error {
	jobData, err := json.Marshal(job)
	if err != nil {
		return err
	}

	// Use a pipeline for atomicity
	pipe := rq.client.Pipeline()
	pipe.Set(ctx, JobStatusPrefix+job.JobID, jobData, 24*time.Hour) // Ephemeral state expires in 24h
	pipe.LPush(ctx, QueueName, jobData)
	_, err = pipe.Exec(ctx)
	
	return err
}

// UpdateState modifies the job's current status in Redis
func (rq *RedisQueue) UpdateState(ctx context.Context, job AsyncJob) error {
	jobData, err := json.Marshal(job)
	if err != nil {
		return err
	}
	return rq.client.Set(ctx, JobStatusPrefix+job.JobID, jobData, 24*time.Hour).Err()
}

// GetState retrieves the current status of a job for polling
func (rq *RedisQueue) GetState(ctx context.Context, jobID string) (*AsyncJob, error) {
	val, err := rq.client.Get(ctx, JobStatusPrefix+jobID).Result()
	if err != nil {
		if err == redis.Nil {
			return nil, fmt.Errorf("job not found")
		}
		return nil, err
	}

	var job AsyncJob
	if err := json.Unmarshal([]byte(val), &job); err != nil {
		return nil, err
	}
	return &job, nil
}