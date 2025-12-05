set -e

BUILD_DIR=build
SRC_DIR=$(pwd)
OUTPUT_BIN=c

CC=${CC:-clang}
COMPILE_FLAGS="-fsanitize=address -fsanitize=undefined -fPIC -Wall -Werror \
  -Wconversion -std=c2x -g -Wimplicit-fallthrough \
  -ftrivial-auto-var-init=pattern \
  -fsanitize=bounds -fno-sanitize-recover=all"

LLVM_CONFIG=${LLVM_CONFIG:-llvm-config}
LLVM_CONFIG_LD_FLAGS=$(${LLVM_CONFIG} --ldflags)
LLVM_CONFIG_INCLUDE_DIR=$(${LLVM_CONFIG} --includedir)
LLVM_CONFIG_SYSTEM_LIBS=$(${LLVM_CONFIG} --system-libs)
LLVM_CONFIG_CORE_LIBS=$(${LLVM_CONFIG} --libs core)

mkdir -p build

# Preprocess separately bc we can't preprocess on our own yet.
${CC} -o ${BUILD_DIR}/compiler.preprocessed.c -E -P ${SRC_DIR}/compiler.c \
  -I${LLVM_CONFIG_INCLUDE_DIR}

# Then do the actual compile.
${CC} -o ${BUILD_DIR}/${OUTPUT_BIN} ${SRC_DIR}/compiler.c ${COMPILE_FLAGS} \
  ${LLVM_CONFIG_LD_FLAGS} ${LLVM_CONFIG_SYSTEM_LIBS} ${LLVM_CONFIG_CORE_LIBS} \
  -I${LLVM_CONFIG_INCLUDE_DIR}

if [ "$1" = "stage2" ] || [ "$1" = "stage3" ]; then
  echo "Building Stage 2 compiler"
  ${BUILD_DIR}/${OUTPUT_BIN} ${BUILD_DIR}/compiler.preprocessed.c -I/usr/include -v
  ${CC} -o ${BUILD_DIR}/${OUTPUT_BIN}.stage2 "out.obj" \
    ${LLVM_CONFIG_LD_FLAGS} ${LLVM_CONFIG_SYSTEM_LIBS} ${LLVM_CONFIG_CORE_LIBS}
fi

if [ "$1" = "stage3" ]; then
  echo "Building Stage 3 compiler"
  ${BUILD_DIR}/${OUTPUT_BIN}.stage2 ${BUILD_DIR}/compiler.preprocessed.c -I/usr/include -v
  ${CC} -o ${BUILD_DIR}/${OUTPUT_BIN}.stage3 "out.obj" \
    ${LLVM_CONFIG_LD_FLAGS} ${LLVM_CONFIG_SYSTEM_LIBS} ${LLVM_CONFIG_CORE_LIBS}

  # These should have the same contents for the same compilation.
  diff ${BUILD_DIR}/${OUTPUT_BIN}.stage2 ${BUILD_DIR}/${OUTPUT_BIN}.stage3
fi
