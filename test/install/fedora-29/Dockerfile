FROM fedora:29
MAINTAINER Maxime Arthaud <maxime.arthaud@nasa.gov>
ARG njobs=2
ARG build_type=Release

# Installs the following versions (note that it might be out of date):
# cmake 3.12.1
# gmp 6.1.2
# boost 1.66.0
# python 2.7.15
# sqlite 3.26.0
# llvm 7.0.0
# clang 7.0.0
# gcc 8.2.1

# Upgrade
RUN dnf -y update

# Install all dependencies
RUN dnf install -y gcc gcc-c++ make cmake gmp-devel boost-devel \
        python python-pygments sqlite-devel zlib-devel \
        llvm llvm-devel llvm-static clang

# Add ikos source code
ADD . /root/ikos

# Build ikos
RUN rm -rf /root/ikos/build && mkdir /root/ikos/build
WORKDIR /root/ikos/build
ENV MAKEFLAGS "-j$njobs"
RUN cmake \
        -DCMAKE_INSTALL_PREFIX="/opt/ikos" \
        -DCMAKE_BUILD_TYPE="$build_type" \
        ..
RUN make
RUN make install

# Run the tests
RUN make check

# Add ikos to the path
ENV PATH "/opt/ikos/bin:$PATH"

# Done
WORKDIR /
