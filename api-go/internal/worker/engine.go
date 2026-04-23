package worker

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"os/exec"
	"time"

	"github.com/Shivam-1827/Schedulrx/internal/models"
)

type EngineBridge struct {
	BinaryPath string
}

func NewEngineBridge(binaryPath string) *EngineBridge {
	return &EngineBridge{BinaryPath: binaryPath}
}

// Execute simulates the workload by piping JSON to the C++ binary
func (eb *EngineBridge) Execute(ctx context.Context, req models.SimulationRequest) (*models.SimulationResult, error) {
	// 1. Enforce a strict 5-second timeout on the C++ execution
	execCtx, cancel := context.WithTimeout(ctx, 5*time.Second)
	defer cancel()

	// 2. Marshal the request to JSON
	inputData, err := json.Marshal(req)
	if err != nil {
		return nil, fmt.Errorf("failed to marshal request: %w", err)
	}

	// 3. Prepare the command
	cmd := exec.CommandContext(execCtx, eb.BinaryPath)
	cmd.Stdin = bytes.NewReader(inputData)
	
	var stdout bytes.Buffer
	var stderr bytes.Buffer
	cmd.Stdout = &stdout
	cmd.Stderr = &stderr

	// 4. Run the C++ binary
	if err := cmd.Run(); err != nil {
		// Check if it was killed by our timeout context
		if execCtx.Err() == context.DeadlineExceeded {
			return nil, fmt.Errorf("C++ engine timed out after 5 seconds")
		}
		return nil, fmt.Errorf("C++ engine failed: %v | stderr: %s", err, stderr.String())
	}

	// 5. Parse the JSON result from stdout
	var result models.SimulationResult
	if err := json.Unmarshal(stdout.Bytes(), &result); err != nil {
		return nil, fmt.Errorf("failed to parse engine output: %w | raw: %s", err, stdout.String())
	}

	if result.Status == "error" {
		return nil, fmt.Errorf("engine reported error: %s", result.Message)
	}

	return &result, nil
}