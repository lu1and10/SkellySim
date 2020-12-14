#ifndef FIBER_HPP
#define FIBER_HPP
#include <Eigen/Dense>
#include <Eigen/LU>
#include <unordered_map>
#include <vector>

class Fiber {
  public:
    enum BC { Force, Torque, Velocity, AngularVelocity, Position, Angle };

    int num_points_;
    double length_;
    double bending_rigidity_;
    double penalty_param_ = 500.0;
    // FIXME: Magic numbers in linear operator calculation
    double beta_tstep_ = 1.0;
    double epsilon_ = 1E-3;
    double v_length_ = 0.0;
    double stall_force_;
    double c_0_, c_1_;

    std::pair<BC, BC> bc_minus_ = {BC::Velocity, BC::AngularVelocity};
    std::pair<BC, BC> bc_plus_ = {BC::Force, BC::Torque};

    Eigen::MatrixXd x_;
    Eigen::MatrixXd xs_;
    Eigen::MatrixXd xss_;
    Eigen::MatrixXd xsss_;
    Eigen::MatrixXd xssss_;
    Eigen::MatrixXd stokeslet_;

    Eigen::MatrixXd A_;
    Eigen::PartialPivLU<Eigen::MatrixXd> A_LU_;
    Eigen::MatrixXd force_operator_;
    Eigen::VectorXd RHS_;

    typedef struct {
        Eigen::ArrayXd alpha;
        Eigen::ArrayXd alpha_roots;
        Eigen::ArrayXd alpha_tension;
        Eigen::ArrayXd weights_0;
        Eigen::MatrixXd D_1_0;
        Eigen::MatrixXd D_2_0;
        Eigen::MatrixXd D_3_0;
        Eigen::MatrixXd D_4_0;
        Eigen::MatrixXd P_X;
        Eigen::MatrixXd P_T;
        Eigen::MatrixXd P_downsample_bc;
    } fib_mat_t;
    const static std::unordered_map<int, fib_mat_t> matrices_;

    Fiber(int num_points, double bending_rigidity, double stall_force, double eta)
        : num_points_(num_points), bending_rigidity_(bending_rigidity), stall_force_(stall_force) {
        x_ = Eigen::MatrixXd::Zero(3, num_points);
        x_.row(0) = Eigen::ArrayXd::LinSpaced(num_points, 0, 1.0).transpose();
        xs_.resize(3, num_points);
        xss_.resize(3, num_points);
        xsss_.resize(3, num_points);
        xssss_.resize(3, num_points);

        c_0_ = -log(M_E * std::pow(epsilon_, 2)) / (8 * M_PI * eta);
        c_1_ = 2.0 / (8.0 * M_PI * eta);
    };

    void build_preconditioner();
    void form_force_operator();
    void compute_RHS(double dt, const Eigen::Ref<const Eigen::MatrixXd> flow,
                     const Eigen::Ref<const Eigen::MatrixXd> f_external);
    void form_linear_operator(double dt, double eta);
    void apply_bc_rectangular(double dt, const Eigen::Ref<const Eigen::MatrixXd> &v_on_fiber,
                              const Eigen::Ref<const Eigen::MatrixXd> &f_on_fiber);
    void translate(const Eigen::Vector3d &r) { x_.colwise() += r; };
    void update_derivatives();
    void update_stokeslet(double);
};

class FiberContainer {
  public:
    std::vector<Fiber> fibers;
    double slenderness_ratio;

    FiberContainer(std::string fiber_file, double stall_force, double eta);

    void update_derivatives();
    void update_stokeslets(double eta);
    void form_linear_operators(double dt, double eta);
    int get_total_fib_points() const {
        int tot = 0;
        for (auto &fib : fibers)
            tot += fib.num_points_;
        return tot;
    };

    Eigen::MatrixXd generate_constant_force(double force_scale) const;
    Eigen::MatrixXd get_r_vectors() const;
    Eigen::MatrixXd flow(const Eigen::Ref<const Eigen::MatrixXd> &forces,
                         const Eigen::Ref<const Eigen::MatrixXd> &r_trg_external, double eta) const;
    Eigen::VectorXd matvec(const Eigen::Ref<const Eigen::VectorXd> &x_all,
                           const Eigen::Ref<const Eigen::MatrixXd> &v_fib) const;
    Eigen::MatrixXd apply_fiber_force(const Eigen::Ref<const Eigen::VectorXd> &x_all) const;
    Eigen::VectorXd apply_preconditioner(const Eigen::Ref<const Eigen::VectorXd> &x_all) const;
};

#endif
