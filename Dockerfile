FROM alpine:edge

RUN apk add --no-cache \
    gcc \
    valgrind \
    clang19 \
    compiler-rt \
    lld \
    llvm \
    libc-dev \
    bash \
    python3 \
    nodejs \
    git \
    make