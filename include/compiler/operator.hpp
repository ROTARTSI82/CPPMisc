#pragma once

#include <vector>
#include <memory>
#include <functional>

#include "compiler/ast.hpp"

class Operator {
public:
    // two callbacks: on_encounter, on_apply
    // use on_encounter to create new blocks to implement short-circuiting
};
