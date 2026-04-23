package worker

import (
	"context"
	"encoding/json"
	"log"
	"time"

	"github.com/redis/go-redis/v9"
	"gorm.io/gorm"

	"github.com/Shivam-1827/Schedulrx/internal/models"
	"github.com/Shivam-1827/Schedulrx/internal/queue"
)

type PoolManager struct {
	redisClient *redis.Client
	queue       *queue.RedisQueue
	bridge      *EngineBridge
	db          *gorm.DB
	workerCount int
}

func NewPoolManager(rdb *redis.Client, q *queue.RedisQueue, bridge *EngineBridge, db *gorm.DB, workers int) *PoolManager {
	return &PoolManager{
		redisClient: rdb,
		queue:       q,
		bridge:      bridge,
		db:          db,
		workerCount: workers,
	}
}

// Start boots up the goroutines. It takes a context for graceful shutdown.
func (pm *PoolManager) Start(ctx context.Context) {
	log.Printf("Starting Worker Pool with %d concurrent workers...", pm.workerCount)
	for i := 1; i <= pm.workerCount; i++ {
		go pm.workerLoop(ctx, i)
	}
}

func (pm *PoolManager) workerLoop(ctx context.Context, workerID int) {
	for {
		select {
		case <-ctx.Done():
			log.Printf("Worker %d shutting down.", workerID)
			return
		default:
			// BRPOP blocks until a job is available or timeout (2 seconds)
			result, err := pm.redisClient.BRPop(ctx, 2*time.Second, queue.QueueName).Result()
			if err != nil {
				if err != redis.Nil {
					log.Printf("Worker %d Redis error: %v", workerID, err)
				}
				continue
			}

			// result[0] is the queue name, result[1] is the JSON payload
			var job queue.AsyncJob
			if err := json.Unmarshal([]byte(result[1]), &job); err != nil {
				log.Printf("Worker %d failed to unmarshal job: %v", workerID, err)
				continue
			}

			pm.processJob(ctx, workerID, job)
		}
	}
}

func (pm *PoolManager) processJob(ctx context.Context, workerID int, job queue.AsyncJob) {
	log.Printf("Worker %d processing Job %s (Algo: %s)", workerID, job.JobID, job.Request.Settings.Algorithm)

	// Update state to PROCESSING
	job.Status = queue.StatusProcessing
	pm.queue.UpdateState(ctx, job)

	// Execute via C++ Bridge
	engineCtx, cancel := context.WithTimeout(context.Background(), 10*time.Second) // generous timeout for processing
	defer cancel()

	simResult, err := pm.bridge.Execute(engineCtx, job.Request)

	if err != nil {
		job.Status = queue.StatusFailed
		job.Error = err.Error()
		pm.queue.UpdateState(ctx, job)
		log.Printf("Worker %d failed Job %s: %v", workerID, job.JobID, err)
		return
	}

	// Update state to COMPLETED
	job.Status = queue.StatusCompleted
	job.Result = simResult
	pm.queue.UpdateState(ctx, job)

	// Persist permanent results to PostgreSQL
	record := models.RunRecord{
		Algorithm:         simResult.AlgorithmUsed,
		TotalTasks:        len(job.Request.Tasks),
		AvgWaitingTime:    simResult.AvgWaitingTime,
		AvgTurnaroundTime: simResult.AvgTurnaroundTime,
		Throughput:        simResult.Throughput,
		TotalTime:         simResult.TotalTime,
	}
	if err := pm.db.Create(&record).Error; err != nil {
		log.Printf("Worker %d failed to save Job %s to DB: %v", workerID, job.JobID, err)
	}

	log.Printf("Worker %d successfully completed Job %s", workerID, job.JobID)
}