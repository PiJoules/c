set -e

BUILD_DIR=build
SRC_DIR=$(pwd)
OUTPUT_BIN=c

CC=${CC:-clang}
COMPILE_FLAGS="-fsanitize=address -fsanitize=undefined -fPIC -Wall -Werror \
  -Wconversion -Wno-builtin-requires-header -std=c2x -g -Wimplicit-fallthrough \
  -ftrivial-auto-var-init=pattern -Wno-incompatible-library-redeclaration \
  -fsanitize=bounds -fuse-ld=lld -fno-sanitize-recover=all"

LLVM_CONFIG=${LLVM_CONFIG:-llvm-config}
LLVM_CONFIG_LD_FLAGS=$(${LLVM_CONFIG} --ldflags)
LLVM_CONFIG_SYSTEM_LIBS=$(${LLVM_CONFIG} --system-libs)
LLVM_CONFIG_CORE_LIBS=$(${LLVM_CONFIG} --libs core)

mkdir -p build

${CC} -o ${BUILD_DIR}/${OUTPUT_BIN} ${SRC_DIR}/compiler.c ${COMPILE_FLAGS} \
  ${LLVM_CONFIG_LD_FLAGS} ${LLVM_CONFIG_SYSTEM_LIBS} ${LLVM_CONFIG_CORE_LIBS}

if [ "$1" = "stage2" ] || [ "$1" = "stage3" ]; then
  echo "Building Stage 2 compiler"
  ${BUILD_DIR}/${OUTPUT_BIN} ${SRC_DIR}/compiler.c
  ${CC} -o ${BUILD_DIR}/${OUTPUT_BIN}.stage2 "out.obj" \
    ${LLVM_CONFIG_LD_FLAGS} ${LLVM_CONFIG_SYSTEM_LIBS} ${LLVM_CONFIG_CORE_LIBS}
fi

if [ "$1" = "stage3" ]; then
  echo "Building Stage 3 compiler"
  ${BUILD_DIR}/${OUTPUT_BIN}.stage2 ${SRC_DIR}/compiler.c
  ${CC} -o ${BUILD_DIR}/${OUTPUT_BIN}.stage3 "out.obj" \
    ${LLVM_CONFIG_LD_FLAGS} ${LLVM_CONFIG_SYSTEM_LIBS} ${LLVM_CONFIG_CORE_LIBS}
fi
