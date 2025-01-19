# My hand at a self-hosted C compiler

![Build Status w/Clang](https://github.com/PiJoules/c/actions/workflows/build.yml/badge.svg)

This does the bare minimum of being able to compile itself.

## Deps

- [LLVM v16.0.6](https://github.com/llvm/llvm-project/releases/tag/llvmorg-16.0.6)

## Building

```sh
$ bash -x build.sh  # Just build the first stage compiler
$ bash -x build.sh stage2  # Build the first and second stage compilers
$ bash -x build.sh stage3  # Build the first and second and third stage compilers
```

## Test

```sh
$ python3 test.py  # Run tests against all 3 stages of compiler.
```

### Reproducing the github actions builders

The github actions can also be tested locally via docker containers using
[act](https://github.com/nektos/act).

```
$ act --workflows .github/workflows/build.yml
```
