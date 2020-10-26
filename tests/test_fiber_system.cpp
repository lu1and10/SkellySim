#include <iostream>
#include <mpi.h>

#ifdef NDEBUG
#undef NDEBUG
#include <cassert>
#define NDEBUG
#else
#include <cassert>
#endif

#include <fiber.hpp>

#include <omp.h>

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    const int n_pts = 48;
    const int n_fib_per_rank = 3000 / size;
    const int n_time = 1;
    const double eta = 1.0;
    const double length = 1.0;
    const double bending_rigidity = 0.1;
    const double dt = 0.005;

    FiberContainer fibs(n_fib_per_rank, n_pts, bending_rigidity, length);
    Eigen::MatrixXd f_fib = Eigen::MatrixXd::Zero(3, n_pts * n_fib_per_rank);

    for (int i = 0; i < n_fib_per_rank; ++i) {
        fibs.fibers[i].translate({10 * drand48() - 5, 10 * drand48() - 5, 10 * drand48() - 5});
        for (int j = 0; j < n_pts; ++j)
            f_fib(2, i * n_pts + j) = 1.0;
        fibs.fibers[i].length_ = 1.0;
    }

    double st = omp_get_wtime();
    for (int i = 0; i < n_time; ++i)
        fibs.update_stokeslets(eta);
    if (rank == 0)
        std::cout << omp_get_wtime() - st << std::endl;

    auto vel = fibs.flow(f_fib, eta);
    st = omp_get_wtime();
    for (int i = 0; i < n_time; ++i)
        vel = fibs.flow(f_fib, eta);
    if (rank == 0)
        std::cout << omp_get_wtime() - st << std::endl;

    st = omp_get_wtime();
    for (int i = 0; i < n_time; ++i)
        fibs.form_linear_operators(dt, eta);
    if (rank == 0)
        std::cout << omp_get_wtime() - st << std::endl;

    if (rank == 0)
        std::cout << "Test passed\n";

    MPI_Finalize();

    return 0;
}
