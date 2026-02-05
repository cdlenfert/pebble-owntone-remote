FROM ubuntu:18.04

# Prevent interactive prompts during build
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    python2.7 \
    python-pip \
    python-virtualenv \
    curl \
    wget \
    git \
    build-essential \
    libreadline-dev \
    libffi-dev \
    libssl-dev \
    zlib1g-dev \
    libjpeg-dev \
    libfreetype6-dev \
    liblcms2-dev \
    libopenjp2-7-dev \
    libtiff-dev \
    libwebp-dev \
    && rm -rf /var/lib/apt/lists/*

# Set Python 2.7 as default
RUN update-alternatives --install /usr/bin/python python /usr/bin/python2.7 1

# Install Pebble SDK
WORKDIR /tmp
RUN wget https://github.com/pebble/pebble-tool/archive/v4.5.tar.gz && \
    tar -xzf v4.5.tar.gz && \
    cd pebble-tool-4.5 && \
    pip install --upgrade pip setuptools && \
    pip install -r requirements.txt && \
    python setup.py install && \
    cd .. && \
    rm -rf pebble-tool-4.5 v4.5.tar.gz

# Install Pebble SDK dependencies
RUN pip install freetype-py pillow sh colorama httplib2 oauth2client pyserial pypng requests semantic_version six websocket-client

# Download and install Pebble SDK
# The SDK will be auto-downloaded on first use, but we set up the environment
RUN mkdir -p /root/.pebble-sdk && touch /root/.pebble-sdk/NO_TRACKING

# Create working directory
WORKDIR /pebble-project

# Set up environment
ENV PEBBLE_PHONE=192.168.1.100
ENV PATH="/root/.pebble-sdk/SDKs/current/bin:${PATH}"

# Default command
CMD ["/bin/bash"]
