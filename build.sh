set -e

BUILD_DIR=build
INCLUDE_DIR=include
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

SRCS=(src/compiler.c src/vector.c src/tree-map.c src/cstring.c src/istream.c \
      src/ifstream.c src/sstream.c src/lexer.c src/type.c src/parser.c src/expr.c \
      src/top-level-node.c src/stmt.c src/sema.c src/source-location.c \
      src/argparse.c src/ast-dump.c src/preprocessor.c src/path.c \
      src/peekable-istream.c)
PREPROCESSED_SRCS=()

# Preprocess separately bc we can't preprocess on our own yet.
for SRC in ${SRCS[@]}; do
  PP=${BUILD_DIR}/${SRC}.preprocessed.c
  mkdir -p $(dirname ${PP})
  ${CC} -o ${PP} -E -P ${SRC} -I${LLVM_CONFIG_INCLUDE_DIR} -I${INCLUDE_DIR}
  PREPROCESSED_SRCS+=(${PP})
done

build() {
  local LOCAL_CC=$1
  local OUTPUT=$2
  local SRC_SET_VAR=$3
  local ARGS=${@:4}
  local OBJS=()

  local SRC_SET="${SRC_SET_VAR}[@]"
  local SRC_SET2=${!SRC_SET}

  for SRC in ${SRC_SET2[@]}; do
    # Do the actual compile.
    OBJ=${BUILD_DIR}/${SRC}.o
    mkdir -p $(dirname ${OBJ})
    
    echo "Compiling: ${LOCAL_CC} -o ${OBJ} ${SRC} ${ARGS}"
    ${LOCAL_CC} -o ${OBJ} ${SRC} ${ARGS}
    OBJS+=(${OBJ})
  done

  # Link
  echo "Linking: ${CC} -o ${OUTPUT} ${OBJS[*]} ${COMPILE_FLAGS} \
    ${LLVM_CONFIG_LD_FLAGS} ${LLVM_CONFIG_SYSTEM_LIBS} ${LLVM_CONFIG_CORE_LIBS} \
    -I${LLVM_CONFIG_INCLUDE_DIR}"
  ${CC} -o ${OUTPUT} ${OBJS[*]} ${COMPILE_FLAGS} \
    ${LLVM_CONFIG_LD_FLAGS} ${LLVM_CONFIG_SYSTEM_LIBS} ${LLVM_CONFIG_CORE_LIBS} \
    -I${LLVM_CONFIG_INCLUDE_DIR}
}

build ${CC} ${BUILD_DIR}/${OUTPUT_BIN} "SRCS" \
      ${COMPILE_FLAGS} ${LLVM_CONFIG_SYSTEM_LIBS} -I${LLVM_CONFIG_INCLUDE_DIR} -c \
      -Wno-unused-function -I${INCLUDE_DIR}

if [ "$1" = "stage2" ] || [ "$1" = "stage3" ]; then
  echo "Building Stage 2 compiler"
  build ${BUILD_DIR}/${OUTPUT_BIN} ${BUILD_DIR}/${OUTPUT_BIN}.stage2 "PREPROCESSED_SRCS" \
    -I/usr/include -I/usr/include/linux -c
fi

if [ "$1" = "stage3" ]; then
  echo "Building Stage 3 compiler"
  build ${BUILD_DIR}/${OUTPUT_BIN}.stage2 ${BUILD_DIR}/${OUTPUT_BIN}.stage3 "PREPROCESSED_SRCS" \
    -I/usr/include -I/usr/include/linux -c

  # These should have the same contents for the same compilation.
  diff ${BUILD_DIR}/${OUTPUT_BIN}.stage2 ${BUILD_DIR}/${OUTPUT_BIN}.stage3
fi
