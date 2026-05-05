FROM --platform=linux/arm64 arm64v8/ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    cmake \
    desktop-file-utils \
    file \
    git \
	libxkbcommon-dev \
    qml6-module-qtqml \
    qml6-module-qtqml-models \
    qml6-module-qtquick \
    qml6-module-qtquick-controls \
    qml6-module-qtquick-layouts \
	qml6-module-qtqml-workerscript \
	qml6-module-qtquick-templates \
    libqt6quick6 \
    libqt6qml6 \
    libqt6gui6 \
    libqt6core6 \
    libqt6network6 \
	libsodium-dev \
    pkg-config \
    ninja-build \
    patchelf \
    pkg-config \
    python3 \
    qt6-base-dev \
    qt6-base-dev-tools \
    qt6-declarative-dev \
    qt6-declarative-dev-tools \
    qt6-tools-dev \
    qt6-tools-dev-tools \
    squashfs-tools \
    unzip \
    wget \
    xz-utils \
 && rm -rf /var/lib/apt/lists/*

COPY scripts/entrypoint.sh /usr/local/bin/entrypoint.sh
COPY cmake/QtArm64Bundle.cmake /opt/qt-arm64-bundle/QtArm64Bundle.cmake

RUN chmod +x /usr/local/bin/entrypoint.sh

WORKDIR /work
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]