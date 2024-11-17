#
# Dockerfile for Sega Dreamcast Toolchains Maker (dc-chain)
#
# usage:
#   - build one of the images:
#       docker build -t dcchain:stable --build-arg dc_chain=stable .
#       docker build -t dcchain:9.3.0-legacy --build-arg dc_chain=9.3.0-legacy --build-arg makejobs=4 .
#   - create and run a container, e.g. for stable:
#       docker run -it --name containername dcchain:stable /bin/bash

# Stage 1
FROM alpine:latest AS build
LABEL maintainer="KallistiOS (Unchained)"

# Installing prerequisites
RUN apk --update add --no-cache \
        build-base \
        gmp-dev \
        mpfr-dev \
        mpc1-dev \
        patch \
        bash \
        texinfo \
        flex \
        curl \
        wget \
        git \
        subversion \
        elfutils-dev \
        libjpeg-turbo-dev \
        libpng-dev \
        python3 \
        && rm -rf /var/cache/apk/*

# Making Sega Dreamcast toolchains
# You may adapt the KallistiOS repository URL if needed
ARG dc_chain=stable
ARG makejobs=2
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

# Stage 2
# Optimize image size
FROM scratch
COPY --from=build / /
