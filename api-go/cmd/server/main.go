package main

import (
	"context"
	"log"
	"os"
	"runtime"

	"github.com/gin-gonic/gin"
	"github.com/redis/go-redis/v9"
	"gorm.io/driver/postgres"
	"gorm.io/gorm"

	"github.com/Shivam-1827/Schedulrx/internal/handlers"
	"github.com/Shivam-1827/Schedulrx/internal/models"
	"github.com/Shivam-1827/Schedulrx/internal/queue"
	"github.com/Shivam-1827/Schedulrx/internal/worker"
	"github.com/prometheus/client_golang/prometheus/promhttp"
)

func main() {
	// 1. Configs
	dsn := os.Getenv("DATABASE_URL")
	if dsn == "" {
		dsn = "host=localhost user=postgres password=postgres dbname=schedulrx port=5432 sslmode=disable"
	}
	redisAddr := os.Getenv("REDIS_URL")
	if redisAddr == "" {
		redisAddr = "localhost:6379"
	}
	enginePath := os.Getenv("ENGINE_PATH")
	if enginePath == "" {
		enginePath = "../engine-cpp/build/engine_main"
	}

	// 2. Initialize PostgreSQL
	db, err := gorm.Open(postgres.Open(dsn), &gorm.Config{})
	if err != nil {
		log.Fatalf("Failed to connect to PG: %v", err)
	}
	db.AutoMigrate(&models.RunRecord{})

	// 3. Initialize Redis
	rdb := redis.NewClient(&redis.Options{Addr: redisAddr})
	if err := rdb.Ping(context.Background()).Err(); err != nil {
		log.Fatalf("Failed to connect to Redis: %v", err)
	}
	jobQueue := queue.NewRedisQueue(rdb)

	// 4. Start Worker Pool
	bridge := worker.NewEngineBridge(enginePath)
	workerCores := runtime.NumCPU() // Optimize pool size based on available cores
	pool := worker.NewPoolManager(rdb, jobQueue, bridge, db, workerCores)
	
	// Start pool in the background
	ctx := context.Background() // In a real app, use signal.NotifyContext for graceful shutdown
	go pool.Start(ctx)

	asyncHandler := &handlers.AsyncHandler{Queue: jobQueue}
	syncHandler := &handlers.APIHandler{DB: db, Bridge: bridge}
	compareHandler := &handlers.CompareHandler{Bridge: bridge}
	recommendHandler := &handlers.RecommendHandler{}
	
	r := gin.Default()

	r.GET("/metrics", gin.WrapH(promhttp.Handler()))

	v1 := r.Group("/api/v1")
	{
		v1.GET("/health", syncHandler.HealthCheck)
		v1.POST("/simulate", syncHandler.Simulate) // Sync
		v1.POST("/simulate/async", asyncHandler.SubmitJob) // Async
		v1.GET("/results/:job_id", asyncHandler.PollResult) // Polling
		v1.POST("/compare", compareHandler.CompareAlgorithms) // Parallel Fan-out
		v1.POST("/recommend", recommendHandler.RecommendAlgorithm)
	}

	port := os.Getenv("PORT")
	if port == "" {
		port = "8080"
	}
	log.Printf("API Gateway active on port %s", port)
	r.Run(":" + port)
}