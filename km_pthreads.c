#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <time.h>

#define MAX_ITER 20

// Structure to pass data to threads
typedef struct {
    float (*data_points)[2];
    float (*medoids)[2];
    int *assignments;
    int start;
    int end;
    int num_clusters;
    int num_dimensions;
} thread_data_t;

// Calculate Euclidean distance
double euclidean_distance(float* point1, float* point2, int D) {
    double sum = 0.0;
    for (int i = 0; i < D; i++) {
        sum += (point1[i] - point2[i]) * (point1[i] - point2[i]);
    }
    return sqrt(sum);
}

// Assign each point to the closest medoid
void* assign_clusters(void* arg) {
    thread_data_t* t_data = (thread_data_t*)arg;
    for (int i = t_data->start; i < t_data->end; i++) {
        int closest = 0;
        double min_distance = euclidean_distance(t_data->data_points[i], t_data->medoids[0], t_data->num_dimensions);
        for (int j = 1; j < t_data->num_clusters; j++) {
            double distance = euclidean_distance(t_data->data_points[i], t_data->medoids[j], t_data->num_dimensions);
            if (distance < min_distance) {
                closest = j;
                min_distance = distance;
            }
        }
        t_data->assignments[i] = closest;
    }
    pthread_exit(NULL);
}

// Get current monotonic time
double monotonic_seconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main(int argc, char** argv) {
    if (argc != 4) {
        printf("Usage: %s <data_file> <num_clusters> <num_threads>\n", argv[0]);
        return 1;
    }

    // Read arguments
    char* filename = argv[1];
    int num_clusters = atoi(argv[2]);
    int num_threads = atoi(argv[3]);

    // Open the file and read data points
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("File open failed");
        return 1;
    }

    int num_points, num_dimensions;
    fscanf(file, "%d %d", &num_points, &num_dimensions);

    // Dynamic memory allocation
    float (*data_points)[2] = malloc(num_points * sizeof(float[2]));
    float (*medoids)[2] = malloc(num_clusters * sizeof(float[2]));
    int *assignments = malloc(num_points * sizeof(int));

    if (!data_points || !medoids || !assignments) {
        perror("Memory allocation failed");
        fclose(file);
        return 1;
    }

    for (int i = 0; i < num_points; i++) {
        for (int j = 0; j < num_dimensions; j++) {
            fscanf(file, "%f", &data_points[i][j]);
        }
    }
    fclose(file);

    // Initialize medoids (first K points as medoids)
    for (int i = 0; i < num_clusters; i++) {
        for (int j = 0; j < num_dimensions; j++) {
            medoids[i][j] = data_points[i][j];
        }
    }

    // Start timer
    double start_time = monotonic_seconds();

    pthread_t threads[num_threads];
    thread_data_t thread_data[num_threads];

    // Main clustering loop (k-medoids)
    for (int iter = 0; iter < MAX_ITER; iter++) {
        int points_per_thread = num_points / num_threads;
        for (int t = 0; t < num_threads; t++) {
            thread_data[t].data_points = data_points;
            thread_data[t].medoids = medoids;
            thread_data[t].assignments = assignments;
            thread_data[t].start = t * points_per_thread;
            thread_data[t].end = (t == num_threads - 1) ? num_points : (t + 1) * points_per_thread;
            thread_data[t].num_clusters = num_clusters;
            thread_data[t].num_dimensions = num_dimensions;
            pthread_create(&threads[t], NULL, assign_clusters, &thread_data[t]);
        }

        // Wait for all threads to complete
        for (int t = 0; t < num_threads; t++) {
            pthread_join(threads[t], NULL);
        }

        // Update medoids (for simplicity, keeping the medoid update part sequential)
        int medoid_change = 0;
        for (int k = 0; k < num_clusters; k++) {
            double min_distance_sum = INFINITY;
            int best_medoid = -1;
            for (int i = 0; i < num_points; i++) {
                if (assignments[i] == k) {
                    double distance_sum = 0;
                    for (int j = 0; j < num_points; j++) {
                        if (assignments[j] == k) {
                            distance_sum += euclidean_distance(data_points[i], data_points[j], num_dimensions);
                        }
                    }
                    if (distance_sum < min_distance_sum) {
                        min_distance_sum = distance_sum;
                        best_medoid = i;
                    }
                }
            }
            if (best_medoid != -1) {
                for (int j = 0; j < num_dimensions; j++) {
                    medoids[k][j] = data_points[best_medoid][j];
                }
                medoid_change = 1;
            }
        }

        // Stop if medoids no longer change
        if (!medoid_change) break;
    }

    // End timer
    double end_time = monotonic_seconds();
    printf("k-medoids clustering time: %0.04fs\n", end_time - start_time);

    // Output post-clustering statistics
    // printf("\nCluster Statistics:\n");

    // // Array to store cluster sizes and centroids
    // int cluster_sizes[num_clusters];
    // float cluster_centroids[num_clusters][num_dimensions];
    // for (int i = 0; i < num_clusters; i++) {
    //     cluster_sizes[i] = 0;
    //     for (int j = 0; j < num_dimensions; j++) {
    //         cluster_centroids[i][j] = 0.0;
    //     }
    // }

    // // Calculate cluster sizes and centroids
    // for (int i = 0; i < num_points; i++) {
    //     int cluster = assignments[i];
    //     cluster_sizes[cluster]++;
    //     for (int j = 0; j < num_dimensions; j++) {
    //         cluster_centroids[cluster][j] += data_points[i][j];
    //     }
    // }

    // // Calculate centroids and print cluster sizes
    // for (int k = 0; k < num_clusters; k++) {
    //     printf("Cluster %d:\n", k);
    //     printf("  Size: %d points\n", cluster_sizes[k]);
    //     printf("  Centroid: ");
    //     for (int j = 0; j < num_dimensions; j++) {
    //         cluster_centroids[k][j] /= cluster_sizes[k];
    //         printf("%.3f ", cluster_centroids[k][j]);
    //     }
    //     printf("\n");
    // }

    // // Calculate and print the spread (average distance from medoid)
    // for (int k = 0; k < num_clusters; k++) {
    //     double total_distance = 0.0;
    //     int cluster_size = cluster_sizes[k];
    //     for (int i = 0; i < num_points; i++) {
    //         if (assignments[i] == k) {
    //             total_distance += euclidean_distance(data_points[i], medoids[k], num_dimensions);
    //         }
    //     }
    //     printf("Average distance to medoid for Cluster %d: %.3f\n", k + 1, total_distance / cluster_size);
    // }

    // Output clusters and medoids
    FILE* clusters_file = fopen("clusters_pthreads.txt", "w");
    for (int i = 0; i < num_points; i++) {
        fprintf(clusters_file, "%d\n", assignments[i]);
    }
    fclose(clusters_file);

    FILE* medoids_file = fopen("medoids_pthreads.txt", "w");
    for (int k = 0; k < num_clusters; k++) {
        for (int j = 0; j < num_dimensions; j++) {
            fprintf(medoids_file, "%.3f ", medoids[k][j]);
        }
        fprintf(medoids_file, "\n");
    }
    fclose(medoids_file);

    // Free allocated memory
    free(data_points);
    free(medoids);
    free(assignments);

    return 0;
}
