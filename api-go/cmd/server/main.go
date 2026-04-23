package main

import (
	"log"
	"os"

	"github.com/gin-gonic/gin"
	"gorm.io/driver/postgres"
	"gorm.io/gorm"

	"github.com/Shivam-1827/Schedulrx/internal/handlers"
	"github.com/Shivam-1827/Schedulrx/internal/models"
	"github.com/Shivam-1827/Schedulrx/internal/worker"
)

func main() {
	// 1. Configuration via Environment Variables
	dsn := os.Getenv("DATABASE_URL")
	if dsn == "" {
		// Fallback for local development
		dsn = "host=localhost user=postgres password=postgres dbname=schedulrx port=5432 sslmode=disable"
	}

	enginePath := os.Getenv("ENGINE_PATH")
	if enginePath == "" {
		// Fallback assuming we run this from the api-go directory and compiled C++ in engine-cpp/build
		enginePath = "../engine-cpp/build/engine_main"
	}

	// 2. Initialize Database
	db, err := gorm.Open(postgres.Open(dsn), &gorm.Config{})
	if err != nil {
		log.Fatalf("Failed to connect to database: %v", err)
	}

	// Auto-migrate the database schema
	err = db.AutoMigrate(&models.RunRecord{})
	if err != nil {
		log.Fatalf("Failed to migrate database: %v", err)
	}

	// 3. Initialize Worker Bridge & Handlers
	bridge := worker.NewEngineBridge(enginePath)
	api := &handlers.APIHandler{
		DB:     db,
		Bridge: bridge,
	}

	// 4. Setup Gin Router
	r := gin.Default()
	
	// Middlewares
	r.Use(gin.Recovery())
	r.Use(gin.Logger())

	// Routes
	v1 := r.Group("/api/v1")
	{
		v1.GET("/health", api.HealthCheck)
		v1.POST("/simulate", api.Simulate)
	}

	// 5. Start Server
	port := os.Getenv("PORT")
	if port == "" {
		port = "8080"
	}
	
	log.Printf("Starting SchedulrX API Gateway on port %s...", port)
	if err := r.Run(":" + port); err != nil {
		log.Fatalf("Server forced to shutdown: %v", err)
	}
}