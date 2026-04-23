package models

import (
	"time"
	"github.com/google/uuid"
	"gorm.io/gorm"
)

// --- API Request/Response Payloads (Matches C++ JSON) ---

type Task struct {
	ID          string  `json:"id"`
	ArrivalTime float64 `json:"arrival_time"`
	BurstTime   float64 `json:"burst_time"`
	Priority    int     `json:"priority"`
	TaskType    string  `json:"task_type"`
	Deadline    float64 `json:"deadline"`
}

type WorkloadSettings struct {
	Algorithm            string  `json:"algorithm"`
	Quantum              float64 `json:"quantum"`
	ContextSwitchPenalty float64 `json:"context_switch_penalty"`
	NumCores             int     `json:"num_cores"`
}

type SimulationRequest struct {
	WorkloadID string           `json:"workload_id"`
	Settings   WorkloadSettings `json:"settings"`
	Tasks      []Task           `json:"tasks"`
}

type SimulationResult struct {
	WorkloadID        string          `json:"workload_id"`
	AlgorithmUsed     string          `json:"algorithm_used"`
	AvgWaitingTime    float64         `json:"avg_waiting_time"`
	AvgTurnaroundTime float64         `json:"avg_turnaround_time"`
	Throughput        float64         `json:"throughput"`
	TotalTime         float64         `json:"total_time"`
	Status            string          `json:"status"`
	Message           string          `json:"message"` // For error handling
}

// --- PostgreSQL Database Models ---

type RunRecord struct {
	ID                uuid.UUID `gorm:"type:uuid;primary_key;"`
	Algorithm         string    `gorm:"type:varchar(50)"`
	TotalTasks        int
	AvgWaitingTime    float64
	AvgTurnaroundTime float64
	Throughput        float64
	TotalTime         float64
	CreatedAt         time.Time
}

func (r *RunRecord) BeforeCreate(tx *gorm.DB) (err error) {
	if r.ID == uuid.Nil {
		r.ID = uuid.New()
	}
	return
}