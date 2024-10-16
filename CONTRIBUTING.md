# Contributing

Before submitting your pull request, please ensure you have successfully built the project from a clean solution (`make clean && make`) and passed all the tests.

## Main branch

The main branch is the primary development branch and shouldn't be pushed into directly, you should always prefer a PR.

## Feature Branch Names

Branch names should be in lowercase and separated by dashes ('-') if they contain multiple words. Example:
```bash
git branch create-mmu
```

## Commits Messages

Use conventional commit types, which can be found [here](https://github.com/pvdlg/conventional-changelog-metahub?tab=readme-ov-file#commit-types).

Example:
```
chore: update dependencies to latest versions
feat: add MMU module
```

- Messages should start with a lowercase character.

The expected format for a git message is as follows:
```
conventional_type: short explanation of the commit

Longer explanation explaining exactly what's changed, what bugs were fixed (with bug tracker reference if applicable).

Closes/Fixes #1234
```

- Always remember to make commit messages descriptive and don't forget to add a description if needed:
```bash
git commit -m "conventional_type: message" -m "description"
```

## Bugs Reports

If you're reporting a bug make sure to list:
1. The necessary steps to reproduce the issue.
2. Expected outcome (i.e. correct output).
3. Description of the behavior.
4. Screenshots; if relevant.

If the issue includes a crash, you should also include:
1. Snippet of the crash log; if relevant.
2. Backtrace obtained with GDB.

## Features and Enhancements

For a feature request make sure to list:
1. What you're trying to achieve.
2. Estimated difficulty level.

For a code refactor/rewrite request make sure to list:
1. Link to the problematic code section/file.
2. Why is the code problematic.
3. Affected systems.
4. Potential gains.
