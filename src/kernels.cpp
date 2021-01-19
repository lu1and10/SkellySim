#include <kernels.hpp>

#include <STKFMM/STKFMM.hpp>

Eigen::MatrixXd kernels::oseen_tensor_contract_direct(const Eigen::Ref<const Eigen::MatrixXd> &r_src,
                                                      const Eigen::Ref<const Eigen::MatrixXd> &r_trg,
                                                      const Eigen::Ref<const Eigen::MatrixXd> &density, double eta,
                                                      double reg, double epsilon_distance) {
    using namespace Eigen;
    const int N_src = r_src.size() / 3;
    const int N_trg = r_trg.size() / 3;

    // # Compute matrix of size 3N \times 3N
    MatrixXd res = MatrixXd::Zero(3, N_trg);

    const double factor = 1.0 / (8.0 * M_PI * eta);
    const double reg2 = std::pow(reg, 2);
    for (int i_trg = 0; i_trg < N_trg; ++i_trg) {
        for (int i_src = 0; i_src < N_src; ++i_src) {
            double fr, gr;
            double dx = r_src(0, i_src) - r_trg(0, i_trg);
            double dy = r_src(1, i_src) - r_trg(1, i_trg);
            double dz = r_src(2, i_src) - r_trg(2, i_trg);
            double dr2 = dx * dx + dy * dy + dz * dz;
            double dr = sqrt(dr2);

            if (dr == 0.0)
                continue;

            if (dr > epsilon_distance) {
                fr = factor / dr;
                gr = factor / std::pow(dr, 3);
            } else {
                double denom_inv = 1.0 / sqrt(std::pow(dr, 2) + reg2);
                fr = factor * denom_inv;
                gr = factor * std::pow(denom_inv, 3);
            }

            double Mxx = fr + gr * dx * dx;
            double Mxy = gr * dx * dy;
            double Mxz = gr * dx * dz;
            double Myy = fr + gr * dy * dy;
            double Myz = gr * dy * dz;
            double Mzz = fr + gr * dz * dz;

            res(0, i_trg) += Mxx * density(0, i_src) + Mxy * density(1, i_src) + Mxz * density(2, i_src);
            res(1, i_trg) += Mxy * density(0, i_src) + Myy * density(1, i_src) + Myz * density(2, i_src);
            res(2, i_trg) += Mxz * density(0, i_src) + Myz * density(1, i_src) + Mzz * density(2, i_src);
        }
    }

    return res;
}

// Build the Oseen tensor for N points (sources == targets).
// Set to zero diagonal terms.
//
// G = f(r) * I + g(r) * (r.T*r)
//
// Input:
//   r_vectors = coordinates.
//   eta = (default 1.0) viscosity
//   reg = (default 5e-3) regularization term
//   epsilon_distance = (default 1e-10) set elements to zero for distances < epsilon_distance.
//
// Output:
//   G = Oseen tensor with dimensions (3*num_points) x (3*num_points).
Eigen::MatrixXd kernels::oseen_tensor_direct(const Eigen::Ref<const Eigen::MatrixXd> &r_src,
                                             const Eigen::Ref<const Eigen::MatrixXd> &r_trg, double eta, double reg,
                                             double epsilon_distance) {

    using namespace Eigen;
    const int N_src = r_src.size() / 3;
    const int N_trg = r_trg.size() / 3;

    // # Compute matrix of size 3N \times 3N
    MatrixXd G = MatrixXd::Zero(r_trg.size(), r_src.size());

    const double factor = 1.0 / (8.0 * M_PI * eta);
    const double reg2 = std::pow(reg, 2);
    for (int i_src = 0; i_src < N_trg; ++i_src) {
        for (int i_trg = 0; i_trg < N_src; ++i_trg) {
            double fr, gr;
            double dx = r_src(0, i_src) - r_trg(0, i_trg);
            double dy = r_src(1, i_src) - r_trg(1, i_trg);
            double dz = r_src(2, i_src) - r_trg(2, i_trg);
            double dr2 = dx * dx + dy * dy + dz * dz;

            if (dr2 == 0.0)
                continue;

            double dr = sqrt(dr2);

            if (dr > epsilon_distance) {
                fr = factor / dr;
                gr = factor / std::pow(dr, 3);
            } else {
                double denom_inv = 1.0 / sqrt(std::pow(dr, 2) + reg2);
                fr = factor * denom_inv;
                gr = factor * std::pow(denom_inv, 3);
            }

            G(i_trg * 3 + 0, i_src * 3 + 0) = fr + gr * dx * dx;
            G(i_trg * 3 + 0, i_src * 3 + 1) = gr * dx * dy;
            G(i_trg * 3 + 0, i_src * 3 + 2) = gr * dx * dz;

            G(i_trg * 3 + 1, i_src * 3 + 0) = gr * dy * dx;
            G(i_trg * 3 + 1, i_src * 3 + 1) = fr + gr * dy * dy;
            G(i_trg * 3 + 1, i_src * 3 + 2) = gr * dy * dz;

            G(i_trg * 3 + 2, i_src * 3 + 0) = gr * dz * dx;
            G(i_trg * 3 + 2, i_src * 3 + 1) = gr * dz * dy;
            G(i_trg * 3 + 2, i_src * 3 + 2) = fr + gr * dz * dz;
        }
    }

    return G;
}

// Build the Stresslet tensor contracted with a vector for N points (sources and targets).
// Set to zero diagonal terms.
//
// S_ij = sum_k -(3/(4*pi)) * r_i * r_j * r_k
//
// Input:
//    r_vectors = coordinates.
//    normal = vector used to contract the Stresslet (in general this will be the normal vector of a surface).
//    eta = (default 1.0) viscosity
//    reg = (default 5e-3) regularization term
//    epsilon_distance = (default 1e-10) set elements to zero for distances < epsilon_distance.
//
// Output:
//    S_normal = Stresslet tensor contracted with a vector with dimensions (3*num_points) x (3*num_points).
// Output with format
//               | S_normal11 S_normal12 ...|
//    S_normal = | S_normal21 S_normal22 ...|
//               | ...                      |
//     with S_normal12 the stresslet between points r_1 and r_2.
//     S_normal12 has dimensions 3 x 3.
Eigen::MatrixXd kernels::stresslet_times_normal(const Eigen::Ref<const Eigen::MatrixXd> &r_src,
                                                const Eigen::Ref<const Eigen::MatrixXd> &normals, double eta,
                                                double reg, double epsilon_distance) {
    const double factor = -3.0 / (4.0 * M_PI * eta);
    const double reg2 = reg * reg;
    const int N = r_src.cols();
    Eigen::MatrixXd Snormal = Eigen::MatrixXd::Zero(3 * N, 3 * N);

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (i == j)
                continue;

            const Eigen::Vector3d dr = r_src.col(i) - r_src.col(j);
            double r_norm = dr.norm();
            if (r_norm < epsilon_distance)
                r_norm = sqrt(r_norm * r_norm + reg2);

            const double r_inv5 = 1.0 / std::pow(r_norm, 5);
            Snormal.block(3 * i, 3 * j, 3, 3) = (factor * dr.dot(normals.col(j)) * r_inv5) * dr * dr.transpose();
        }
    }

    return Snormal;
}

// Build the Stresslet tensor contracted with two vectors for N points (sources and targets).
// Set to diagonal terms to zero.
// S_i = sum_jk -(3/(4*pi)) * r_i * r_j * r_k * density_j * normal_k / r**5
// Input:
//   r_vectors = coordinates.
//   normal = vector used to contract the Stresslet (in general this will be the normal vector of a surface).
//   density = vector used to contract the Stresslet (in general this will be a double layer potential).
//   eta = (default 1.0) viscosity
//   reg = (default 5e-3) regularization term
//   epsilon_distance = (default 1e-10) set elements to zero for distances < epsilon_distance.
// Output:
//   S_normal = Stresslet tensor contracted with two vectors with dimensions (3, num_points).
Eigen::MatrixXd kernels::stresslet_times_normal_times_density(const Eigen::Ref<const Eigen::MatrixXd> &r_src,
                                                              const Eigen::Ref<const Eigen::MatrixXd> &normals,
                                                              const Eigen::Ref<const Eigen::MatrixXd> &density,
                                                              double eta, double reg, double epsilon_distance) {
    const int N = r_src.size() / 3;
    const double factor = -3.0 / (4.0 * M_PI * eta);
    const double reg2 = reg * reg;
    Eigen::MatrixXd Sdn = Eigen::MatrixXd::Zero(3, r_src.cols());

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (i == j)
                continue;

            const Eigen::Vector3d dr = r_src.col(i) - r_src.col(j);
            double r_norm = dr.norm();
            if (r_norm < epsilon_distance)
                r_norm = sqrt(r_norm * r_norm + reg2);

            const double r_inv5 = 1.0 / std::pow(r_norm, 5);
            const double f0 = factor * dr.dot(density.col(j)) * dr.dot(normals.col(j)) * r_inv5;
            Sdn.col(i) += f0 * dr;
        }
    }

    return Sdn;
}

Eigen::MatrixXd kernels::stokes_vel_fmm(const int n_trg, const Eigen::Ref<const Eigen::MatrixXd> &f_sl,
                                        const Eigen::Ref<const Eigen::MatrixXd> &f_dl, stkfmm::STKFMM *fmmPtr) {
    Eigen::MatrixXd res = Eigen::MatrixXd::Zero(3, n_trg);
    fmmPtr->clearFMM(stkfmm::KERNEL::Stokes);
    fmmPtr->evaluateFMM(stkfmm::KERNEL::Stokes, f_sl.size() / 3, f_sl.data(), n_trg, res.data(), f_dl.size() / 3,
                        f_dl.data());
    return res;
}

Eigen::MatrixXd kernels::stokes_pvel_fmm(const int n_trg, const Eigen::Ref<const Eigen::MatrixXd> &f_sl,
                                         const Eigen::Ref<const Eigen::MatrixXd> &f_dl, stkfmm::STKFMM *fmmPtr) {
    Eigen::MatrixXd res = Eigen::MatrixXd::Zero(4, n_trg);
    fmmPtr->clearFMM(stkfmm::KERNEL::PVel);
    fmmPtr->evaluateFMM(stkfmm::KERNEL::PVel, f_sl.size() / 4, f_sl.data(), n_trg, res.data(), f_dl.size() / 9,
                        f_dl.data());
    return res;
}
