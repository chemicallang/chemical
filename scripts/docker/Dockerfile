# syntax=docker/dockerfile:1.4
ARG BASE_IMAGE=ubuntu:24.04
FROM ${BASE_IMAGE} AS builder

ARG RELEASE_PLATFORM=linux # linux | linux-alpine | macos | windows    
ARG VARIANT=             # empty or tcc or lsp
ARG VERSION=v0.0.28
ARG ARCH_OVERRIDE=       # optional override: amd64 | arm64 | x64
ARG TARGETARCH

# Expose ARGs to run
ENV VERSION="${VERSION}" \
    RELEASE_PLATFORM="${RELEASE_PLATFORM}" \
    VARIANT="${VARIANT}" \
    ARCH_OVERRIDE="${ARCH_OVERRIDE:-${TARGETARCH}}"

SHELL ["/bin/sh", "-euxc"]

# Quick package installation depending on distro
RUN if grep -q -i alpine /etc/os-release 2>/dev/null; then \
      apk add --no-cache wget unzip bash build-base libc6-compat ; \
    else \
      apt-get update && apt-get install -y wget unzip ca-certificates build-essential libc6-dev --no-install-recommends && rm -rf /var/lib/apt/lists/* ; \
    fi

# download and run official installer script
RUN wget -qO- https://raw.githubusercontent.com/chemicallang/chemical/main/scripts/download.sh | bash -s

CMD ["chemical", "--help"]