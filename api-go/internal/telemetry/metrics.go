package telemetry

import (
	"github.com/prometheus/client_golang/prometheus"
	"github.com/prometheus/client_golang/prometheus/promauto"
)

var (
	// Track total API requests
	HttpRequestsTotal = promauto.NewCounterVec(
		prometheus.CounterOpts{
			Name: "schedulrx_http_requests_total",
			Help: "Total number of HTTP requests processed",
		},
		[]string{"path", "method", "status"},
	)

	// Track simulation execution time
	SimulationDuration = promauto.NewHistogramVec(
		prometheus.HistogramOpts{
			Name:    "schedulrx_simulation_duration_seconds",
			Help:    "Time taken to execute a C++ simulation",
			Buckets: prometheus.DefBuckets,
		},
		[]string{"algorithm"},
	)

	// Track queued jobs
	JobsQueued = promauto.NewGauge(
		prometheus.GaugeOpts{
			Name: "schedulrx_jobs_queued_current",
			Help: "Current number of jobs in the async queue",
		},
	)
)