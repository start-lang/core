FROM alpine:edge

RUN apk add --no-cache \
    curl \
    gnupg \
    gcc \
    valgrind \
    clang22 \
    compiler-rt \
    lld22 \
    llvm22 \
    libc-dev \
    bash \
    python3 \
    nodejs \
    git \
    make