FROM fedora:28
MAINTAINER Maxime Arthaud <maxime.arthaud@nasa.gov>
ARG njobs=2
ARG build_type=Release

# Installs the following versions (note that it might be out of date):
# cmake 3.11.2
# gmp 6.1.2
# boost 1.66.0
# python 2.7.15
# sqlite 3.22.0
# llvm 7.0.1
# clang 7.0.1
# gcc 8.2.1

# Upgrade
RUN dnf -y update

# Install all dependencies
RUN dnf install -y which findutils patch bzip2 xz zlib-devel libedit-devel \
        gcc gcc-c++ make cmake gmp-devel boost-devel python python-pygments sqlite-devel

# Add ikos source code
ADD . /root/ikos

# Use the bootstrap script to install ikos and run the tests
WORKDIR /root/ikos/script
RUN ./bootstrap -vvf \
        --prefix="/opt/ikos" \
        --builddir="/root/ikos-build" \
        --build-type="$build_type" \
        --jobs="$njobs"

# Done
WORKDIR /
