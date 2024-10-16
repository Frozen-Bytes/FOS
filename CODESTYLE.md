# Code Style

The original JOS source code was written in an old C style, while newly added code (FOS code) does not follow a clear style.
So due to the fact that JOS' code provides a more consistent and coherent style, we will opt to use it.

## Clang Formatter

New code should be formatted automatically using `clang-format` and a `.clang-format` file.

- Use [`clangd`](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd) VSCode Plugin by LLVM to replace the default formatting engine.

## 1. General Guidelines

- Aim for readability and maintainability.
- Line length is `80` characters.
- Functions that take no arguments are declared `f(void)` not `f()`.
    - `f()` is incorrect in modern `C` and should be avoided.

For VSCode editor:

- To set a visual ruler at 80 characters, add the following line to your `settings.json`:
```json
"editor.rulers": [ 80 ],
```

## 2. Naming Conventions

- **Variables**: Use `snake_case` for variable names (e.g., `my_variable`).
- **Functions**: Use `snake_case` for function names (e.g., `my_function`).
- **Types**: Use `PascalCase` for type names (e.g., `MyStruct`).
- Preprocessor macros are always `UPPER_SNAKE_CASE`.
  - There are a few grandfathered exceptions: `assert`, `panic`, `static_assert`, `offsetof`.

## 3. Indentation and Spacing

- Use tabs for indentation (typically 4 spaces wide).
- Use a single space before and after operators (e.g., `a + b`).
- Do not add space after function names before the opening parenthesis (e.g., `functionName(arg)`).
- Pointer types have spaces (e.g., `uint32 *` not `uint32*`)
- No trailing whitespace

## 4. Block Structure

Every indented statement is braced; even if the block contains just one
statement. The opening brace is on the line that contains the control
flow statement that introduces the new block; the closing brace is on the
same line as the else keyword, or on a line by itself if there is no else
keyword. Example:

```C
// Wrong (no space after `if`)
if(condition) {
  do_something();
}

// Correct
if (condition) {
  do_something();
}

// Wrong
if (condition)
    print("true.\n");

// Correct
if (condition) {
    print("true.\n");
}

if (a == 5) {
    printf("a was 5.\n");
} else if (a == 6) {
    printf("a was 6.\n");
} else {
    printf("a was something else entirely.\n");
}
```

An exception is the opening brace for a function; for reasons of tradition
and clarity it comes on a line by itself:

```C
void
a_function(void)
{
    do_something();
}
```

Note also that the return type is on a separate line. The reasoning behind this is to allow for long return type names while honoring the 80 character-wide line length. Example:

```C
static struct Type *
a_function_with_long_name(int a, int b, int c)
{
   do_something();
}
```

## 5. Comments

- Comments in the original JOS code are usually C /* ... */ comments.
- Prefer C++ style // comments in new code.
- Use comments to explain complex code.
