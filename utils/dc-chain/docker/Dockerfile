#
# Dockerfile for Sega Dreamcast Toolchains Maker (dc-chain)
#
# usage:
#   - build one of the images:
#       docker build -t dcchain:stable --build-arg dc_chain=stable --build-arg default_precision=m4-single .
#       docker build -t dcchain:legacy --build-arg dc_chain=legacy --build-arg makejobs=4 .
#       docker build -t dcchain:dev --build-arg dc_chain=dev --build-arg makejobs=4 --build-arg enable_objc=0 --build-arg enable_objcpp=0 .
#   - create and run a container, e.g. for stable:
#       docker run -it --name containername dcchain:stable /bin/bash

# Stage 1
FROM alpine:latest AS build
LABEL maintainer="KallistiOS (Unchained)"

# Installing bare minimum for toolchain compilation
RUN apk add --no-cache \
        build-base \
        gmp-dev \
        mpfr-dev \
        mpc1-dev \
        patch \
        texinfo \
        git \
        curl \
        wget \
        bash \
        flex \
        bison

# Making Sega Dreamcast toolchains
# You may adapt the KallistiOS repository URL if needed
ARG dc_chain=stable
ARG makejobs=2
ARG verbose=1
ARG enable_cpp=1
ARG enable_objc=1
ARG enable_objcpp=1
ARG enable_d=0
ARG enable_ada=0
ARG enable_rust=0
ARG enable_libgccjit=0
ARG precision_modes=m4-single-only,m4-single
ARG default_precision=m4-single-only
ARG thread_model=kos
ARG use_kos_patches=1

RUN mkdir -p /opt/toolchains/dc \
        && git clone --depth=1 https://github.com/KallistiOSUnchained/KallistiOS.git /opt/toolchains/dc/kos \
        && cd /opt/toolchains/dc/kos/utils/dc-chain \
        && make build \
        toolchain_profile=$dc_chain \
        makejobs=$makejobs \
        verbose=$verbose \
        enable_cpp=$enable_cpp \
        enable_objc=$enable_objc \
        enable_objcpp=$enable_objcpp \
        enable_d=$enable_d \
        enable_ada=$enable_ada \
        enable_rust=$enable_rust \
        enable_libgccjit=$enable_libgccjit \
        precision_modes=$precision_modes \
        default_precision=$default_precision \
        thread_model=$thread_model \
        use_kos_patches=$use_kos_patches \
        && rm -rf /opt/toolchains/dc/kos

# Stage 2: Minimal Runtime Environment
FROM alpine:latest

# Copy compiled toolchains from the build stage
COPY --from=build /opt/toolchains/dc /opt/toolchains/dc

# Create the bin directory to avoid missing directory errors
RUN mkdir -p /opt/toolchains/dc/bin

# Installing prerequisites for compiling KOS, kos-ports, utils
RUN apk add --no-cache \
        build-base \
        cmake \
        git \
        curl \
        wget \
        bash \
        elfutils-dev \
        libjpeg-turbo-dev \
        libpng-dev \
        python3 \
        ruby-rake \
        libisofs-dev \
        meson \
        coreutils

# Set default working directory
WORKDIR /opt/toolchains/dc

# Set BASH_ENV so it works for non interactive as well
# example: docker run --rm $TAG "make"
ENV BASH_ENV="/opt/toolchains/dc/kos/environ.sh.master"

# Set Shell to Bash so environ.sh.master will be sourced
# in every following run command
SHELL ["/bin/bash", "-c"]

# Set entry point to bash so arg to docker run can be
# run as a command.
ENTRYPOINT ["/bin/bash", "-c"]

# If run with no parameters just start bash
CMD ["/bin/bash"]
