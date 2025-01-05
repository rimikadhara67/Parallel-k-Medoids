# Compiler and flags
CC = gcc
CFLAGS = -Wall -O3 -pthread -fopenmp
LDFLAGS = -lm
PTHREADS_EXEC = km_pthreads
OPENMP_EXEC = km_openmp

# File paths
PTHREADS_SRC = km_pthreads.c
OPENMP_SRC = km_openmp.c
DATASET_PATH = ../../../../export/scratch/CSCI-5451/assignment-1/tenth_large_cpd.txt

# Default rule to compile both executables
all: $(PTHREADS_EXEC) $(OPENMP_EXEC)

# Rule to compile Pthreads version
$(PTHREADS_EXEC): $(PTHREADS_SRC)
	$(CC) $(CFLAGS) -o $(PTHREADS_EXEC) $(PTHREADS_SRC) $(LDFLAGS)

# Rule to compile OpenMP version
$(OPENMP_EXEC): $(OPENMP_SRC)
	$(CC) $(CFLAGS) -o $(OPENMP_EXEC) $(OPENMP_SRC) $(LDFLAGS)

# Parameters for clusters and threads
CLUSTERS = 256 512 1024
THREADS = 1 2 4 8 16

# Run all tests
run: $(PTHREADS_EXEC) $(OPENMP_EXEC)
	@echo "Running tests for Pthreads..."
	@for clusters in $(CLUSTERS); do \
		for threads in $(THREADS); do \
			echo "Running ./$(PTHREADS_EXEC) with Clusters = $$clusters, Threads = $$threads"; \
			./$(PTHREADS_EXEC) $(DATASET_PATH) $$clusters $$threads; \
		done \
	done
	@echo "Running tests for OpenMP..."
	@for clusters in $(CLUSTERS); do \
		for threads in $(THREADS); do \
			echo "Running ./$(OPENMP_EXEC) with Clusters = $$clusters, Threads = $$threads"; \
			./$(OPENMP_EXEC) $(DATASET_PATH) $$clusters $$threads; \
		done \
	done

# Clean generated files
clean:
	rm -f $(PTHREADS_EXEC) $(OPENMP_EXEC) clusters_pthreads.txt medoids_pthreads.txt clusters_openmp.txt medoids_openmp.txt
