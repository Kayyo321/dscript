#include "math.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "../objs.h"
#include "../throwables.h"

namespace {

double expect_number_arg(const std::vector<Value> &args, const std::size_t index, const std::string &fn_name) {
    if (index >= args.size()) {
        throw ArityError(fn_name + "() missing argument.");
    }

    if (args[index].type != ValueType::Number) {
        throw TypeError(fn_name + "() argument must be a number.");
    }

    return args[index].as.number;
}

Value math_abs(const std::vector<Value> &args) {
    return Value::number(std::fabs(expect_number_arg(args, 0, "math.abs")));
}

Value math_sign(const std::vector<Value> &args) {
    const double value = expect_number_arg(args, 0, "math.sign");
    if (value > 0.0) {
        return Value::number(1.0);
    }
    if (value < 0.0) {
        return Value::number(-1.0);
    }
    return Value::number(0.0);
}

Value math_floor(const std::vector<Value> &args) {
    return Value::number(std::floor(expect_number_arg(args, 0, "math.floor")));
}

Value math_ceil(const std::vector<Value> &args) {
    return Value::number(std::ceil(expect_number_arg(args, 0, "math.ceil")));
}

Value math_round(const std::vector<Value> &args) {
    return Value::number(std::round(expect_number_arg(args, 0, "math.round")));
}

Value math_trunc(const std::vector<Value> &args) {
    return Value::number(std::trunc(expect_number_arg(args, 0, "math.trunc")));
}

Value math_min(const std::vector<Value> &args) {
    const double a = expect_number_arg(args, 0, "math.min");
    const double b = expect_number_arg(args, 1, "math.min");
    return Value::number(std::fmin(a, b));
}

Value math_max(const std::vector<Value> &args) {
    const double a = expect_number_arg(args, 0, "math.max");
    const double b = expect_number_arg(args, 1, "math.max");
    return Value::number(std::fmax(a, b));
}

Value math_clamp(const std::vector<Value> &args) {
    const double value = expect_number_arg(args, 0, "math.clamp");
    const double low = expect_number_arg(args, 1, "math.clamp");
    const double high = expect_number_arg(args, 2, "math.clamp");
    if (high < low) {
        throw TypeError("math.clamp() max must be >= min.");
    }
    return Value::number(std::clamp(value, low, high));
}

Value math_sin(const std::vector<Value> &args) {
    return Value::number(std::sin(expect_number_arg(args, 0, "math.sin")));
}

Value math_cos(const std::vector<Value> &args) {
    return Value::number(std::cos(expect_number_arg(args, 0, "math.cos")));
}

Value math_tan(const std::vector<Value> &args) {
    return Value::number(std::tan(expect_number_arg(args, 0, "math.tan")));
}

Value math_asin(const std::vector<Value> &args) {
    const double value = expect_number_arg(args, 0, "math.asin");
    if (value < -1.0 || value > 1.0) {
        throw TypeError("math.asin() argument must be in [-1, 1].");
    }
    return Value::number(std::asin(value));
}

Value math_acos(const std::vector<Value> &args) {
    const double value = expect_number_arg(args, 0, "math.acos");
    if (value < -1.0 || value > 1.0) {
        throw TypeError("math.acos() argument must be in [-1, 1].");
    }
    return Value::number(std::acos(value));
}

Value math_atan(const std::vector<Value> &args) {
    return Value::number(std::atan(expect_number_arg(args, 0, "math.atan")));
}

Value math_atan2(const std::vector<Value> &args) {
    const double y = expect_number_arg(args, 0, "math.atan2");
    const double x = expect_number_arg(args, 1, "math.atan2");
    return Value::number(std::atan2(y, x));
}

Value math_sinh(const std::vector<Value> &args) {
    return Value::number(std::sinh(expect_number_arg(args, 0, "math.sinh")));
}

Value math_cosh(const std::vector<Value> &args) {
    return Value::number(std::cosh(expect_number_arg(args, 0, "math.cosh")));
}

Value math_tanh(const std::vector<Value> &args) {
    return Value::number(std::tanh(expect_number_arg(args, 0, "math.tanh")));
}

Value math_sqrt(const std::vector<Value> &args) {
    const double value = expect_number_arg(args, 0, "math.sqrt");
    if (value < 0.0) {
        throw TypeError("math.sqrt() argument must be >= 0.");
    }
    return Value::number(std::sqrt(value));
}

Value math_cbrt(const std::vector<Value> &args) {
    return Value::number(std::cbrt(expect_number_arg(args, 0, "math.cbrt")));
}

Value math_pow(const std::vector<Value> &args) {
    const double base = expect_number_arg(args, 0, "math.pow");
    const double exponent = expect_number_arg(args, 1, "math.pow");
    return Value::number(std::pow(base, exponent));
}

Value math_hypot(const std::vector<Value> &args) {
    const double x = expect_number_arg(args, 0, "math.hypot");
    const double y = expect_number_arg(args, 1, "math.hypot");
    return Value::number(std::hypot(x, y));
}

Value math_exp(const std::vector<Value> &args) {
    return Value::number(std::exp(expect_number_arg(args, 0, "math.exp")));
}

Value math_exp2(const std::vector<Value> &args) {
    return Value::number(std::exp2(expect_number_arg(args, 0, "math.exp2")));
}

Value math_log(const std::vector<Value> &args) {
    const double value = expect_number_arg(args, 0, "math.log");
    if (value <= 0.0) {
        throw TypeError("math.log() argument must be > 0.");
    }
    return Value::number(std::log(value));
}

Value math_log10(const std::vector<Value> &args) {
    const double value = expect_number_arg(args, 0, "math.log10");
    if (value <= 0.0) {
        throw TypeError("math.log10() argument must be > 0.");
    }
    return Value::number(std::log10(value));
}

Value math_log2(const std::vector<Value> &args) {
    const double value = expect_number_arg(args, 0, "math.log2");
    if (value <= 0.0) {
        throw TypeError("math.log2() argument must be > 0.");
    }
    return Value::number(std::log2(value));
}

Value math_fmod(const std::vector<Value> &args) {
    const double x = expect_number_arg(args, 0, "math.fmod");
    const double y = expect_number_arg(args, 1, "math.fmod");
    if (y == 0.0) {
        throw TypeError("math.fmod() divisor must not be zero.");
    }
    return Value::number(std::fmod(x, y));
}

Value math_deg2rad(const std::vector<Value> &args) {
    constexpr double pi = 3.141592653589793238462643383279502884;
    const double degrees = expect_number_arg(args, 0, "math.deg2rad");
    return Value::number(degrees * pi / 180.0);
}

Value math_rad2deg(const std::vector<Value> &args) {
    constexpr double pi = 3.141592653589793238462643383279502884;
    const double radians = expect_number_arg(args, 0, "math.rad2deg");
    return Value::number(radians * 180.0 / pi);
}

Value math_is_nan(const std::vector<Value> &args) {
    return Value::boolean(std::isnan(expect_number_arg(args, 0, "math.is_nan")));
}

Value math_is_inf(const std::vector<Value> &args) {
    return Value::boolean(std::isinf(expect_number_arg(args, 0, "math.is_inf")));
}

Value math_is_finite(const std::vector<Value> &args) {
    return Value::boolean(std::isfinite(expect_number_arg(args, 0, "math.is_finite")));
}

} // namespace

std::unordered_map<std::string, Value> create_math_stdlib() {
    std::unordered_map<std::string, Value> exports;

    constexpr double pi = 3.141592653589793238462643383279502884;
    constexpr double tau = 6.283185307179586476925286766559005768;
    constexpr double e = 2.718281828459045235360287471352662498;

    exports.insert_or_assign("pi", Value::number(pi));
    exports.insert_or_assign("tau", Value::number(tau));
    exports.insert_or_assign("e", Value::number(e));
    exports.insert_or_assign("inf", Value::number(std::numeric_limits<double>::infinity()));
    exports.insert_or_assign("nan", Value::number(std::numeric_limits<double>::quiet_NaN()));

    exports.insert_or_assign("abs", Value::object(std::make_shared<ObjNative>(1, math_abs)));
    exports.insert_or_assign("sign", Value::object(std::make_shared<ObjNative>(1, math_sign)));
    exports.insert_or_assign("floor", Value::object(std::make_shared<ObjNative>(1, math_floor)));
    exports.insert_or_assign("ceil", Value::object(std::make_shared<ObjNative>(1, math_ceil)));
    exports.insert_or_assign("round", Value::object(std::make_shared<ObjNative>(1, math_round)));
    exports.insert_or_assign("trunc", Value::object(std::make_shared<ObjNative>(1, math_trunc)));

    exports.insert_or_assign("min", Value::object(std::make_shared<ObjNative>(2, math_min)));
    exports.insert_or_assign("max", Value::object(std::make_shared<ObjNative>(2, math_max)));
    exports.insert_or_assign("clamp", Value::object(std::make_shared<ObjNative>(3, math_clamp)));

    exports.insert_or_assign("sin", Value::object(std::make_shared<ObjNative>(1, math_sin)));
    exports.insert_or_assign("cos", Value::object(std::make_shared<ObjNative>(1, math_cos)));
    exports.insert_or_assign("tan", Value::object(std::make_shared<ObjNative>(1, math_tan)));
    exports.insert_or_assign("asin", Value::object(std::make_shared<ObjNative>(1, math_asin)));
    exports.insert_or_assign("acos", Value::object(std::make_shared<ObjNative>(1, math_acos)));
    exports.insert_or_assign("atan", Value::object(std::make_shared<ObjNative>(1, math_atan)));
    exports.insert_or_assign("atan2", Value::object(std::make_shared<ObjNative>(2, math_atan2)));

    exports.insert_or_assign("sinh", Value::object(std::make_shared<ObjNative>(1, math_sinh)));
    exports.insert_or_assign("cosh", Value::object(std::make_shared<ObjNative>(1, math_cosh)));
    exports.insert_or_assign("tanh", Value::object(std::make_shared<ObjNative>(1, math_tanh)));

    exports.insert_or_assign("sqrt", Value::object(std::make_shared<ObjNative>(1, math_sqrt)));
    exports.insert_or_assign("cbrt", Value::object(std::make_shared<ObjNative>(1, math_cbrt)));
    exports.insert_or_assign("pow", Value::object(std::make_shared<ObjNative>(2, math_pow)));
    exports.insert_or_assign("hypot", Value::object(std::make_shared<ObjNative>(2, math_hypot)));

    exports.insert_or_assign("exp", Value::object(std::make_shared<ObjNative>(1, math_exp)));
    exports.insert_or_assign("exp2", Value::object(std::make_shared<ObjNative>(1, math_exp2)));
    exports.insert_or_assign("log", Value::object(std::make_shared<ObjNative>(1, math_log)));
    exports.insert_or_assign("log10", Value::object(std::make_shared<ObjNative>(1, math_log10)));
    exports.insert_or_assign("log2", Value::object(std::make_shared<ObjNative>(1, math_log2)));

    exports.insert_or_assign("fmod", Value::object(std::make_shared<ObjNative>(2, math_fmod)));
    exports.insert_or_assign("deg2rad", Value::object(std::make_shared<ObjNative>(1, math_deg2rad)));
    exports.insert_or_assign("rad2deg", Value::object(std::make_shared<ObjNative>(1, math_rad2deg)));

    exports.insert_or_assign("is_nan", Value::object(std::make_shared<ObjNative>(1, math_is_nan)));
    exports.insert_or_assign("is_inf", Value::object(std::make_shared<ObjNative>(1, math_is_inf)));
    exports.insert_or_assign("is_finite", Value::object(std::make_shared<ObjNative>(1, math_is_finite)));

    return exports;
}
