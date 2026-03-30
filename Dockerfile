FROM rockylinux:9 AS builder

RUN dnf install -y epel-release \
    && dnf config-manager --set-enabled crb \
    && dnf install -y \
        gcc gcc-c++ cmake rpm-build vim-common \
        boost-devel libsodium-devel libsodium-static sqlite-devel \
    && dnf clean all

COPY . /src
WORKDIR /build

RUN cmake -S /src -B . \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_MUA=OFF \
    -DSMTP_TESTING=OFF

RUN cmake --build . --parallel "$(nproc)"
RUN cpack -G RPM

FROM registry.access.redhat.com/ubi9-minimal AS runtime

RUN microdnf install -y sqlite-libs && microdnf clean all

COPY --from=builder /build/*.rpm /tmp/
RUN rpm -ivh /tmp/*.rpm && rm -f /tmp/*.rpm

RUN mkdir -p /srv/smtp/data/mailboxes /srv/smtp/imap-server
COPY default_config.json /srv/smtp/default_config.json
COPY imap-server/imap.config /srv/smtp/imap-server/imap.config
