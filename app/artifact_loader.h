#ifndef DSCRIPT_ARTIFACT_LOADER_H
#define DSCRIPT_ARTIFACT_LOADER_H

#include <string>
#include <unordered_map>
#include <vector>

#include "../parsing/expr.h"

struct ArtifactModule {
    std::vector<StmtPtr> ast;
    std::unordered_map<const Expr *, int> resolved_locals;
    std::unordered_map<std::string, std::string> import_map;
    std::string source_path;
};

struct ArtifactProgram {
    std::string schema;
    std::string version;
    std::string entry_module;
    std::unordered_map<std::string, ArtifactModule> modules;
};

bool load_artifact_program(const std::string &artifact_path, ArtifactProgram &out_program, std::string &error);

#endif //DSCRIPT_ARTIFACT_LOADER_H