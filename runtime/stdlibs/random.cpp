#include "random.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <random>

#include "../objs.h"
#include "../throwables.h"

static std::mt19937 &rng_engine() {
    static std::mt19937 engine(std::random_device{}());
    return engine;
}

static double expect_number_arg(const std::vector<Value> &args, const std::size_t index, const std::string &fn_name) {
    if (index >= args.size()) {
        throw ArityError(fn_name + "() missing argument.");
    }

    if (args[index].type != ValueType::Number) {
        throw TypeError(fn_name + "() argument must be a number.");
    }

    return args[index].as.number;
}

static std::shared_ptr<Obj> expect_object_arg(const std::vector<Value> &args, const std::size_t index, const std::string &fn_name) {
    if (index >= args.size()) {
        throw ArityError(fn_name + "() missing argument.");
    }

    if (args[index].type != ValueType::Object || args[index].as.object == nullptr) {
        throw TypeError(fn_name + "() argument must be an object.");
    }

    return args[index].as.object;
}

static Value random_random(const std::vector<Value> &args) {
    (void) args;
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return Value::number(dist(rng_engine()));
}

static Value random_seed(const std::vector<Value> &args) {
    const double seed_number = expect_number_arg(args, 0, "random.seed");

    if (!std::isfinite(seed_number)) {
        throw TypeError("random.seed() argument must be a finite number.");
    }

    const std::uint32_t seed = static_cast<std::uint32_t>(
        std::fmod(std::fabs(seed_number), static_cast<double>(std::numeric_limits<std::uint32_t>::max()))
    );
    rng_engine().seed(seed);
    return Value::boolean(true);
}

static Value random_uniform(const std::vector<Value> &args) {
    const double min_value = expect_number_arg(args, 0, "random.uniform");
    const double max_value = expect_number_arg(args, 1, "random.uniform");

    if (max_value < min_value) {
        throw TypeError("random.uniform() max must be >= min.");
    }

    std::uniform_real_distribution<double> dist(min_value, max_value);
    return Value::number(dist(rng_engine()));
}

static Value random_randint(const std::vector<Value> &args) {
    const double min_value_raw = expect_number_arg(args, 0, "random.randint");
    const double max_value_raw = expect_number_arg(args, 1, "random.randint");

    if (std::floor(min_value_raw) != min_value_raw || std::floor(max_value_raw) != max_value_raw) {
        throw TypeError("random.randint() bounds must be integers.");
    }

    const auto min_value = static_cast<long long>(min_value_raw);
    const auto max_value = static_cast<long long>(max_value_raw);

    if (max_value < min_value) {
        throw TypeError("random.randint() max must be >= min.");
    }

    std::uniform_int_distribution<long long> dist(min_value, max_value);
    return Value::number(static_cast<double>(dist(rng_engine())));
}

static Value random_choice(const std::vector<Value> &args) {
    const std::shared_ptr<Obj> object = expect_object_arg(args, 0, "random.choice");

    if (object->get_type() == ObjType::List) {
        const auto list = std::static_pointer_cast<List>(object);
        if (list->elements.empty()) {
            throw TypeError("random.choice() cannot choose from an empty list.");
        }

        std::uniform_int_distribution<std::size_t> dist(0, list->elements.size() - 1);
        return list->elements[dist(rng_engine())];
    }

    if (object->get_type() == ObjType::String) {
        const auto text = std::static_pointer_cast<ObjString>(object);
        if (text->chars.empty()) {
            throw TypeError("random.choice() cannot choose from an empty string.");
        }

        std::uniform_int_distribution<std::size_t> dist(0, text->chars.size() - 1);
        const char ch = text->chars[dist(rng_engine())];
        return Value::object(std::make_shared<ObjString>(std::string(1, ch)));
    }

    throw TypeError("random.choice() expects a list or string.");
}

std::unordered_map<std::string, Value> create_random_stdlib() {
    std::unordered_map<std::string, Value> exports;
    exports.insert_or_assign("random", Value::object(std::make_shared<ObjNative>(0, random_random)));
    exports.insert_or_assign("seed", Value::object(std::make_shared<ObjNative>(1, random_seed)));
    exports.insert_or_assign("uniform", Value::object(std::make_shared<ObjNative>(2, random_uniform)));
    exports.insert_or_assign("randint", Value::object(std::make_shared<ObjNative>(2, random_randint)));
    exports.insert_or_assign("choice", Value::object(std::make_shared<ObjNative>(1, random_choice)));
    return exports;
}
