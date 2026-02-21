#include "io.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "../objs.h"
#include "../throwables.h"

static std::string expect_string_arg(const std::vector<Value> &args, const std::size_t index, const std::string &fn_name) {
    if (index >= args.size()) {
        throw ArityError(fn_name + "() missing argument.");
    }

    const Value &value = args[index];
    if (value.type != ValueType::Object || value.as.object->get_type() != ObjType::String) {
        throw TypeError(fn_name + "() argument must be a string.");
    }

    return std::static_pointer_cast<ObjString>(value.as.object)->chars;
}

static Value io_write(const std::vector<Value> &args) {
    const std::string content = expect_string_arg(args, 0, "io.write");
    std::cout << content;
    std::cout.flush();
    return Value::boolean(true);
}

static Value io_input(const std::vector<Value> &args) {
    const std::string prompt = expect_string_arg(args, 0, "io.input");
    std::cout << prompt;
    std::cout.flush();

    std::string line;
    std::getline(std::cin, line);
    return Value::object(std::make_shared<ObjString>(line));
}

static Value io_read_file(const std::vector<Value> &args) {
    const std::string path = expect_string_arg(args, 0, "io.read_file");
    std::ifstream in(path);
    if (!in.is_open()) {
        throw NameError("io.read_file() could not open '" + path + "'.");
    }

    std::ostringstream buffer;
    buffer << in.rdbuf();
    return Value::object(std::make_shared<ObjString>(buffer.str()));
}

static Value io_write_file(const std::vector<Value> &args) {
    const std::string path = expect_string_arg(args, 0, "io.write_file");
    const std::string content = expect_string_arg(args, 1, "io.write_file");

    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) {
        throw NameError("io.write_file() could not open '" + path + "'.");
    }

    out << content;
    return Value::boolean(true);
}

static Value io_append_file(const std::vector<Value> &args) {
    const std::string path = expect_string_arg(args, 0, "io.append_file");
    const std::string content = expect_string_arg(args, 1, "io.append_file");

    std::ofstream out(path, std::ios::app);
    if (!out.is_open()) {
        throw NameError("io.append_file() could not open '" + path + "'.");
    }

    out << content;
    return Value::boolean(true);
}

static Value io_exists(const std::vector<Value> &args) {
    const std::string path = expect_string_arg(args, 0, "io.exists");
    return Value::boolean(std::filesystem::exists(path));
}

static Value io_remove_file(const std::vector<Value> &args) {
    const std::string path = expect_string_arg(args, 0, "io.remove");
    return Value::boolean(std::filesystem::remove(path));
}

static Value io_mkdir_p(const std::vector<Value> &args) {
    const std::string path = expect_string_arg(args, 0, "io.mkdir_p");
    return Value::boolean(std::filesystem::create_directories(path));
}

static Value io_list_dir(const std::vector<Value> &args) {
    const std::string path = expect_string_arg(args, 0, "io.list_dir");

    if (!std::filesystem::exists(path)) {
        throw NameError("io.list_dir() path does not exist: '" + path + "'.");
    }

    std::vector<std::string> names;
    for (const auto &entry : std::filesystem::directory_iterator(path)) {
        names.push_back(entry.path().filename().string());
    }

    std::sort(names.begin(), names.end());

    std::vector<Value> values;
    values.reserve(names.size());
    for (const auto &name : names) {
        values.push_back(Value::object(std::make_shared<ObjString>(name)));
    }

    return Value::object(std::make_shared<List>(values));
}

std::unordered_map<std::string, Value> create_io_stdlib() {
    std::unordered_map<std::string, Value> exports;
    exports.insert_or_assign("write", Value::object(std::make_shared<ObjNative>(1, io_write)));
    exports.insert_or_assign("input", Value::object(std::make_shared<ObjNative>(1, io_input)));
    exports.insert_or_assign("read_file", Value::object(std::make_shared<ObjNative>(1, io_read_file)));
    exports.insert_or_assign("write_file", Value::object(std::make_shared<ObjNative>(2, io_write_file)));
    exports.insert_or_assign("append_file", Value::object(std::make_shared<ObjNative>(2, io_append_file)));
    exports.insert_or_assign("exists", Value::object(std::make_shared<ObjNative>(1, io_exists)));
    exports.insert_or_assign("remove", Value::object(std::make_shared<ObjNative>(1, io_remove_file)));
    exports.insert_or_assign("mkdir_p", Value::object(std::make_shared<ObjNative>(1, io_mkdir_p)));
    exports.insert_or_assign("list_dir", Value::object(std::make_shared<ObjNative>(1, io_list_dir)));
    return exports;
}
