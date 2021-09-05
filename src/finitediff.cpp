// Functions to compute gradients using finite difference.
// Based on the functions in https://github.com/PatWie/CppNumericalSolvers
// and rewritten to use Eigen
#include "finitediff.hpp"

#include <array>
#include <vector>

#include <spdlog/spdlog.h>

namespace fd {

// Compute the gradient of a function at a point using finite differences.
void finite_gradient(
    const Eigen::VectorXd& x,
    const std::function<double(const Eigen::VectorXd&)>& f,
    Eigen::VectorXd& grad,
    const AccuracyOrder accuracy,
    const double eps)
{
    // Create an array of the coefficients for finite differences.
    // See: https://en.wikipedia.org/wiki/Finite_difference_coefficient
    // clang-format off
    // The external coefficients, c1, in c1 * f(x + c2).
    static const std::array<std::vector<double>, 4> coeff =
    { { {1, -1}, {1, -8, 8, -1}, {-1, 9, -45, 45, -9, 1}, {3, -32, 168, -672, 672, -168, 32, -3} } };
    // The internal coefficients, c2, in c1 * f(x + c2).
    static const std::array<std::vector<double>, 4> coeff2 =
    { { {1, -1}, {-2, -1, 1, 2}, {-3, -2, -1, 1, 2, 3}, {-4, -3, -2, -1, 1, 2, 3, 4} } };
    // clang-format on
    // The denominators of the finite difference.
    static const std::array<double, 4> dd = { { 2, 12, 60, 840 } };

    grad.resize(x.rows());

    const size_t innerSteps = 2 * (accuracy + 1);
    const double ddVal = dd[accuracy] * eps;

    Eigen::VectorXd xx = x;
    for (long d = 0; d < x.rows(); d++) {
        grad[d] = 0;
        for (size_t s = 0; s < innerSteps; ++s) {
            xx[d] += coeff2[accuracy][s] * eps;
            grad[d] += coeff[accuracy][s] * f(xx);
            xx[d] = x[d];
        }
        grad[d] /= ddVal;
    }
}

void finite_jacobian(
    const Eigen::VectorXd& x,
    const std::function<Eigen::VectorXd(const Eigen::VectorXd&)>& f,
    Eigen::MatrixXd& jac,
    const AccuracyOrder accuracy,
    const double eps)
{
    // Create an array of the coefficients for finite differences.
    // See: https://en.wikipedia.org/wiki/Finite_difference_coefficient
    // clang-format off
    // The external coefficients, c1, in c1 * f(x + c2).
    static const std::array<std::vector<double>, 4> coeff =
    { { {1, -1}, {1, -8, 8, -1}, {-1, 9, -45, 45, -9, 1}, {3, -32, 168, -672, 672, -168, 32, -3} } };
    // The internal coefficients, c2, in c1 * f(x + c2).
    static const std::array<std::vector<double>, 4> coeff2 =
    { { {1, -1}, {-2, -1, 1, 2}, {-3, -2, -1, 1, 2, 3}, {-4, -3, -2, -1, 1, 2, 3, 4} } };
    // clang-format on
    // The denominators of the finite difference.
    static const std::array<double, 4> dd = { { 2, 12, 60, 840 } };

    jac.resize(f(x).rows(), x.rows());

    const size_t innerSteps = 2 * (accuracy + 1);
    const double ddVal = dd[accuracy] * eps;

    Eigen::VectorXd xx = x;
    for (long d = 0; d < x.rows(); d++) {
        jac.col(d).setZero();
        for (size_t s = 0; s < innerSteps; ++s) {
            xx[d] += coeff2[accuracy][s] * eps;
            jac.col(d) += coeff[accuracy][s] * f(xx);
            xx[d] = x[d];
        }
        jac.col(d) /= ddVal;
    }
}

void finite_hessian(
    const Eigen::VectorXd& x,
    const std::function<double(const Eigen::VectorXd&)>& f,
    Eigen::MatrixXd& hess,
    // const AccuracyOrder accuracy,
    const double eps)
{
    hess.resize(x.rows(), x.rows());

    Eigen::VectorXd xx = x;
    for (long i = 0; i < x.rows(); i++) {
        for (long j = 0; j < x.rows(); j++) {
            double f4 = f(xx);
            xx[i] += eps;
            xx[j] += eps;
            double f1 = f(xx);
            xx[j] -= eps;
            double f2 = f(xx);
            xx[j] += eps;
            xx[i] -= eps;
            double f3 = f(xx);
            hess(i, j) = (f1 - f2 - f3 + f4) / (eps * eps);

            xx[i] = x[i];
            xx[j] = x[j];
        }
    }
}

// Compare if two gradients are close enough.
bool compare_gradient(
    const Eigen::VectorXd& x,
    const Eigen::VectorXd& y,
    const double test_eps,
    const std::string& msg)
{
    assert(x.rows() == y.rows());

    bool same = true;
    for (long d = 0; d < x.rows(); ++d) {
        double scale = std::max(std::max(abs(x[d]), abs(y[d])), double(1.0));
        double abs_diff = abs(x[d] - y[d]);

        if (abs_diff > test_eps * scale) {
            spdlog::debug(
                "{} eps={:.3e} r={} x={:.3e} y={:.3e} |x-y|={:.3e} "
                "|x-y|/|x|={:.3e} |x-y|/|y|={:3e}",
                msg, test_eps, d, x(d), y(d), abs_diff, abs_diff / abs(x(d)),
                abs_diff / abs(y(d)));
            same = false;
        }
    }
    return same;
}

// Compare if two jacobians are close enough.
bool compare_jacobian(
    const Eigen::MatrixXd& x,
    const Eigen::MatrixXd& y,
    const double test_eps,
    const std::string& msg)
{
    assert(x.rows() == y.rows());
    assert(x.cols() == y.cols());

    bool same = true;
    for (long d = 0; d < x.rows(); ++d) {
        for (long c = 0; c < x.cols(); ++c) {
            double scale =
                std::max(std::max(abs(x(d, c)), abs(y(d, c))), double(1.0));

            double abs_diff = abs(x(d, c) - y(d, c));

            if (abs_diff > test_eps * scale) {
                spdlog::debug(
                    "{} eps={:.3e} r={} c={} x={:.3e} y={:.3e} "
                    "|x-y|={:.3e} |x-y|/|x|={:.3e} |x-y|/|y|={:3e}",
                    msg, test_eps, d, c, x(d, c), y(d, c), abs_diff,
                    abs_diff / abs(x(d, c)), abs_diff / abs(y(d, c)));
                same = false;
            }
        }
    }
    return same;
}

// Compare if two hessians are close enough.
bool compare_hessian(
    const Eigen::MatrixXd& x,
    const Eigen::MatrixXd& y,
    const double test_eps,
    const std::string& msg)
{
    return compare_jacobian(x, y, test_eps, msg);
}

} // namespace fd
