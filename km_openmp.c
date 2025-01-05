#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include "km_openmp.h"

#define MAX_ITER 20

// Calculate Euclidean distance between two points
double euclidean_distance(float *point1, float *point2, int D)
{
    double sum = 0.0;
    for (int i = 0; i < D; i++)
    {
        sum += (point1[i] - point2[i]) * (point1[i] - point2[i]);
    }
    return sqrt(sum);
}

// Assign each point to the closest medoid in parallel
void assign_clusters(float (*data_points)[2], float (*medoids)[2], int *assignments, int N, int K, int D)
{
#pragma omp parallel for
    for (int i = 0; i < N; i++)
    {
        int closest = 0;
        double min_distance = euclidean_distance(data_points[i], medoids[0], D);
        for (int j = 1; j < K; j++)
        {
            double distance = euclidean_distance(data_points[i], medoids[j], D);
            if (distance < min_distance)
            {
                closest = j;
                min_distance = distance;
            }
        }
        assignments[i] = closest;
    }
}

// Get current time using OpenMP
double monotonic_seconds()
{
    return omp_get_wtime();
}

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        printf("Usage: %s <data_file> <num_clusters> <num_threads>\n", argv[0]);
        return 1;
    }

    // Read arguments
    char *filename = argv[1];
    int num_clusters = atoi(argv[2]);
    int num_threads = atoi(argv[3]);

    // Set the number of threads for OpenMP
    omp_set_num_threads(num_threads);

    // Open the data file
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Error opening file");
        return 1;
    }

    int num_points, num_dimensions;
    fscanf(file, "%d %d", &num_points, &num_dimensions);
    float data_points[num_points][num_dimensions];
    for (int i = 0; i < num_points; i++)
    {
        for (int j = 0; j < num_dimensions; j++)
        {
            fscanf(file, "%f", &data_points[i][j]);
        }
    }
    fclose(file);

    // Initialize medoids (use the first K points as initial medoids)
    float medoids[num_clusters][num_dimensions];
    for (int i = 0; i < num_clusters; i++)
    {
        for (int j = 0; j < num_dimensions; j++)
        {
            medoids[i][j] = data_points[i][j];
        }
    }

    // Array to store cluster assignments
    int assignments[num_points];

    // Start timing
    double start_time = monotonic_seconds();

    // Main k-medoids clustering loop
    for (int iter = 0; iter < MAX_ITER; iter++)
    {
        // Assign points to clusters
        assign_clusters(data_points, medoids, assignments, num_points, num_clusters, num_dimensions);

        // Update medoids
        int medoid_changed = 0;
#pragma omp parallel for reduction(+:medoid_changed)
        for (int k = 0; k < num_clusters; k++)
        {
            double min_distance_sum = INFINITY;
            int best_medoid = -1;

            for (int i = 0; i < num_points; i++)
            {
                if (assignments[i] == k)
                {
                    double distance_sum = 0.0;
                    for (int j = 0; j < num_points; j++)
                    {
                        if (assignments[j] == k)
                        {
                            distance_sum += euclidean_distance(data_points[i], data_points[j], num_dimensions);
                        }
                    }

                    if (distance_sum < min_distance_sum)
                    {
                        min_distance_sum = distance_sum;
                        best_medoid = i;
                    }
                }
            }

            if (best_medoid != -1)
            {
                for (int j = 0; j < num_dimensions; j++)
                {
                    medoids[k][j] = data_points[best_medoid][j];
                }
                medoid_changed = 1;
            }
        }

        // Stop if medoids no longer change
        if (!medoid_changed)
            break;
    }

    // End timing
    double end_time = monotonic_seconds();
    printf("k-medoids clustering time: %.4f seconds\n", end_time - start_time);

    // Output cluster statistics
    // printf("\nCluster Statistics:\n");

    // int cluster_sizes[num_clusters];
    // float cluster_centroids[num_clusters][num_dimensions];

    // // Initialize cluster sizes and centroids
    // for (int i = 0; i < num_clusters; i++)
    // {
    //     cluster_sizes[i] = 0;
    //     for (int j = 0; j < num_dimensions; j++)
    //     {
    //         cluster_centroids[i][j] = 0.0;
    //     }
    // }

    // // Calculate cluster sizes and centroids
    // for (int i = 0; i < num_points; i++)
    // {
    //     int cluster = assignments[i];
    //     cluster_sizes[cluster]++;
    //     for (int j = 0; j < num_dimensions; j++)
    //     {
    //         cluster_centroids[cluster][j] += data_points[i][j];
    //     }
    // }

    // // Print cluster sizes and centroids
    // for (int k = 0; k < num_clusters; k++)
    // {
    //     printf("Cluster %d:\n", k);
    //     printf("  Size: %d points\n", cluster_sizes[k]);
    //     printf("  Centroid: ");
    //     for (int j = 0; j < num_dimensions; j++)
    //     {
    //         cluster_centroids[k][j] /= cluster_sizes[k];
    //         printf("%.3f ", cluster_centroids[k][j]);
    //     }
    //     printf("\n");
    // }

    // // Calculate and print average distance to medoid
    // for (int k = 0; k < num_clusters; k++)
    // {
    //     double total_distance = 0.0;
    //     for (int i = 0; i < num_points; i++)
    //     {
    //         if (assignments[i] == k)
    //         {
    //             total_distance += euclidean_distance(data_points[i], medoids[k], num_dimensions);
    //         }
    //     }
    //     printf("Average distance to medoid for Cluster %d: %.3f\n", k, total_distance / cluster_sizes[k]);
    // }

    // Output clusters and medoids to files
    FILE *clusters_file = fopen("clusters_openmp.txt", "w");
    for (int i = 0; i < num_points; i++)
    {
        fprintf(clusters_file, "%d\n", assignments[i]);
    }
    fclose(clusters_file);

    FILE *medoids_file = fopen("medoids_openmp.txt", "w");
    for (int k = 0; k < num_clusters; k++)
    {
        for (int j = 0; j < num_dimensions; j++)
        {
            fprintf(medoids_file, "%.3f ", medoids[k][j]);
        }
        fprintf(medoids_file, "\n");
    }
    fclose(medoids_file);

    return 0;
}
