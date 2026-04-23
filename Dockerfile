
FROM ubuntu:22.04 AS cpp-builder

# Install C++ toolchain and JSON library
RUN apt-get update && apt-get install -y cmake g++ nlohmann-json3-dev

WORKDIR /build-cpp
COPY engine-cpp/ ./engine-cpp/

# Compile Engine
RUN mkdir -p engine-cpp/build && cd engine-cpp/build && \
    cmake .. && make


FROM golang:1.21 AS go-builder

WORKDIR /build-go
COPY api-go/go.mod api-go/go.sum ./
RUN go mod download

COPY api-go/ ./
# Build statically linked Go binary
RUN CGO_ENABLED=0 GOOS=linux go build -o /schedulrx-api cmd/server/main.go


FROM ubuntu:22.04

# Install basic certificates and clean up cache to reduce image size
RUN apt-get update && apt-get install -y ca-certificates && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy binaries from previous stages
COPY --from=cpp-builder /build-cpp/engine-cpp/build/engine_main /app/engine_main
COPY --from=go-builder /schedulrx-api /app/schedulrx-api

# Set permissions and Environment variables
RUN chmod +x /app/engine_main /app/schedulrx-api
ENV ENGINE_PATH=/app/engine_main
ENV GIN_MODE=release

EXPOSE 8080

# Run the Go Orchestrator
CMD ["/app/schedulrx-api"]