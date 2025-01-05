# Parallelizing k-Medoids Algorithm with PThreads and OpenMP

With this project, I hope to parallelize the k-medoids clustering algorithm using two parallelization platforms: PThreads and OpenMP. The goal was to break down the computational tasks into smaller, parallelizable components to improve performance when dealing with large datasets. This was my primary strategy and point of view for the homework.
The k-medoids algorithm involves two primary steps that are computationally expensive, but parallelizable:
* Assigning each data point to the nearest medoid (cluster assignment).
* Updating the medoids based on the new cluster assignments.
Both steps are independent for each data point or cluster, making them perfect for parallelization.

## Algorithm and Method
The process of assigning clusters was divided among multiple threads. In the assign_clusters() function, each thread was responsible for a subset of the data points. These threads operated independently, calculating the Euclidean distance between each point and all medoids. The distance calculations were memoized for later use in the medoid update phase. 
Once the cluster assignments were made, the next step was updating the medoids. Each medoid was updated by finding the point in the cluster that minimized the total distance to all other points. 

### Parallelization Using PThreads:

Here, the memoized distances were utilized to speed up the computation. While this step was executed sequentially in some cases, improvements were made by parallelizing the update operation, which was still computationally expensive.

* **Thread Distribution**: The dataset was divided into chunks based on the number of threads, ensuring each thread received an approximately equal number of points to process.
* **Memoization**: To avoid recalculating distances repeatedly, a 2D array was used to store the distances between points and medoids.
* **Synchronization**: As the medoids update was a critical section, care was taken to avoid race conditions by ensuring proper synchronization between threads.

### Parallelization Using OpenMP
In the OpenMP implementation, the assign_clusters() function was parallelized using *#pragma omp parallel* for. This directive automatically created multiple threads, each responsible for a portion of the data points. The Euclidean distance calculation and cluster assignment process ran concurrently across these threads, reducing the time required for this phase.
The medoid update step was also parallelized. Similar to the PThreads implementation, the update involved finding the best medoid for each cluster by minimizing the total distance to other points in the cluster. The *#pragma omp parallel* for directive was used, along with a critical section (#pragma omp critical) to prevent race conditions when updating the medoids. The use of OpenMP significantly simplified the parallelization of this step.

## Results

## Challenges
* **Threading Overhead**: In the case of small numbers of clusters (e.g., 256), the OpenMP implementation exhibited significant overhead and the timings are terrible, resulting in much longer runtimes compared to larger cluster sizes. This was probably due to the lack of work per thread, leading to inefficient thread utilization.
* **Synchronization Costs**: During medoid updates, thread synchronization using critical sections introduced bottlenecks, especially as the number of threads increased. However, using parallel reductions in OpenMP helped mitigate this issue to some extent.
