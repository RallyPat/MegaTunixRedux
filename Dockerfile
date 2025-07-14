# MegaTunix Redux 2025 Development Environment
FROM ubuntu:22.04

LABEL maintainer="MegaTunix Redux Development Team"
LABEL description="Development environment for MegaTunix Redux 2025"

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    # Build tools
    build-essential \
    meson \
    ninja-build \
    pkg-config \
    cmake \
    git \
    # GTK4 and dependencies
    libgtk-4-dev \
    libglib2.0-dev \
    libxml2-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    # Parser generators
    flex \
    bison \
    # Internationalization
    gettext \
    # Documentation
    doxygen \
    graphviz \
    # Analysis tools
    valgrind \
    cppcheck \
    clang-format \
    clang-tidy \
    # Development tools
    vim \
    nano \
    gdb \
    # X11 for GUI testing
    xvfb \
    x11-apps \
    # Cleanup
    && rm -rf /var/lib/apt/lists/*

# Create development user
ARG USERNAME=developer
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    && apt-get update \
    && apt-get install -y sudo \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME \
    && rm -rf /var/lib/apt/lists/*

# Set up workspace
WORKDIR /workspace
RUN chown $USERNAME:$USERNAME /workspace

# Switch to development user
USER $USERNAME

# Set environment variables
ENV DISPLAY=:99
ENV XDG_RUNTIME_DIR=/tmp/runtime-$USERNAME

# Create runtime directory
RUN mkdir -p $XDG_RUNTIME_DIR && chmod 700 $XDG_RUNTIME_DIR

# Default command
CMD ["/bin/bash"]

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD pkg-config --exists gtk4 || exit 1
