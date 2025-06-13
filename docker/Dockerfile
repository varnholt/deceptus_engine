
FROM archlinux:latest

# Install required packages
RUN pacman -Syu --noconfirm \
    base-devel \
    cmake \
    ninja \
    git \
    gcc \
    clang \
    glew \
    glm \
    libx11 \
    libxrandr \
    libxinerama \
    libxcursor \
    libxi \
    mesa \
    alsa-lib \
    pulseaudio \
    libglvnd \
    libvorbis \
    libogg \
    openal \
    freetype2 \
    libpng \
    && pacman -Scc --noconfirm

# Create build user
RUN useradd -m builder
USER builder
WORKDIR /home/builder

# Clone the repository
RUN git clone https://github.com/varnholt/deceptus_engine.git
WORKDIR deceptus_engine

# Create build directory
RUN mkdir -p build
WORKDIR build

# Run cmake with bundled SDL3/SFML/lua
RUN cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
RUN ninja

# Volume for game data
VOLUME ["/home/builder/game_data"]

CMD ["bash"]
