FROM debian:buster-slim

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends apt-utils && \
    apt-get install -y --no-install-recommends sudo ca-certificates pkg-config curl wget bzip2 xz-utils make bsdtar doxygen gnupg && \
    apt-get install -y --no-install-recommends git git-restore-mtime && \
    apt-get install -y --no-install-recommends gdebi-core && \
    apt-get install -y --no-install-recommends cmake zip unzip && \
    apt-get install -y --no-install-recommends locales && \
    apt-get install -y p7zip-full && \
    sed -i -e 's/# en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' /etc/locale.gen && \
    dpkg-reconfigure --frontend=noninteractive locales && \
    update-locale LANG=en_US.UTF-8 && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

ENV LANG en_US.UTF-8

RUN mkdir opt

RUN curl -k -L https://cdn.discordapp.com/attachments/917922918963482644/919720393999282206/devkitpro.7z -o devkitpro.7z

RUN 7z x *.7z -oopt

RUN rm *.7z

ENV DEVKITPRO=/opt/devkitpro

ENV DEVKITPPC=${DEVKITPRO}/devkitPPC

ENV DEVKITARM=${DEVKITPRO}/devkitARM