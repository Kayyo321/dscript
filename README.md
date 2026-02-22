# dscript

A small, expressive scripting language in C++ focused on personalization.

## Why dscript is cool

- **Custom keyword-style callables** with `def` (write APIs that read like language keywords)
- **Pass expressions and blocks as arguments** with `%expr` and `$block`
- Named block arguments like `pass: { ... }`, `fail: { ... }`
- Functions, classes, inheritance, loops, imports, and stdlib I/O

## 30-second quick start

```bash
./build.sh
./build/dscript examples/18_def_keyword_showcase.dsr
```

## Signature feature #1: custom keywords with `def`

`def` lets you build callables that are invoked in keyword-like style:

```dsr
def shout(msg) {
    log('shout: ' + msg);
}

shout 'hello';
```

You can create APIs that read like mini-language constructs:

```dsr
def guard(cond) (pass: $pass_block, fail: $fail_block) {
    if cond then {
        if $pass_block is not none then $pass_block();
    } else {
        if $fail_block is not none then $fail_block();
    }
}

guard energy > 3 pass: {
    log('allowed');
} fail: {
    log('blocked');
}
```

## Signature feature #2: pass blocks (and expressions) as args

dscript supports expression and block parameters directly:

```dsr
fn x(%expr) ($block) {
    %expr();
    $block();
}

x i = 10 {
    log(i);
}
```

- `%name` receives an expression/callable argument
- `$name` receives a block argument
- Named blocks can be optional (`none`) and checked at runtime

## Feature tour (examples)

- Literals/operators: `examples/01_literals_and_ops.dsr`
- Variables/scope: `examples/02_variables_assignment_scope.dsr`
- Conditionals/loops: `examples/03_if_else_and_logic.dsr` to `examples/06_functions_and_return.dsr`
- `def` keyword callables: `examples/07_def_callable_keyword.dsr`, `examples/18_def_keyword_showcase.dsr`
- Expr/block identifiers: `examples/10_expr_and_block_identifiers.dsr`
- Named blocks: `examples/12_named_blocks_and_none.dsr`
- Classes/inheritance: `examples/08_classes_fields_methods.dsr`, `examples/09_inheritance_super_self.dsr`
- Imports/modules: `examples/23_imports_and_modules.dsr`
- Stdlib I/O: `examples/24_stdlib_io.dsr`

## Build options

- Dev build: `./build.sh [BuildType] [BuildDir]`
- Release matrix build: `./build_release.sh [BuildRoot]`

## Project layout

- `lexing/` tokenization
- `parsing/` AST + parser
- `resolving/` name/scope resolution
- `runtime/` VM, values, objects, stdlibs