#include "cnpy.hpp"
#include <Eigen/Core>
#include <iostream>
#include <fstream>
#include <mpi.h>
#include <system.hpp>

#ifdef NDEBUG
#undef NDEBUG
#include <cassert>
#define NDEBUG
#else
#include <cassert>
#endif

#include <body.hpp>

template <typename DerivedA, typename DerivedB>
bool allclose(
    const Eigen::DenseBase<DerivedA> &a, const Eigen::DenseBase<DerivedB> &b,
    const typename DerivedA::RealScalar &rtol = Eigen::NumTraits<typename DerivedA::RealScalar>::dummy_precision(),
    const typename DerivedA::RealScalar &atol = Eigen::NumTraits<typename DerivedA::RealScalar>::epsilon()) {
    return ((a.derived() - b.derived()).array().abs() <= (atol + rtol * b.derived().array().abs())).all();
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    std::string config_file("test_body.toml");
    toml::table config = toml::parse_file(config_file);
    Params params(config.get_as<toml::table>("params"));
    toml::array *body_configs = config.get_as<toml::array>("bodies");
    Body body(body_configs->get_as<toml::table>(0), params);

    System sys(config_file);
    for (auto &fiber : sys.fc_.fibers) {
        auto [i_body, i_site] = fiber.binding_site_;
        auto &body = sys.bc_.bodies[i_body];

        if (i_site > 0)
            std::cout << i_body << " " << i_site << " [" << body.nucleation_sites_ref_.col(i_site).transpose() << "] ["
                      << fiber.x_.col(0).transpose() << "]" << std::endl;
    }

    MPI_Finalize();

    std::cout << "Test passed\n";
    return 0;
}
