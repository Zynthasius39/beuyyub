FROM debian:unstable-slim
LABEL org.opencontainers.image.authors="Zynt, super.alekberov@proton.me"
RUN apt update && apt upgrade -y && apt install curl cmake g++ libmpg123-dev libcurl4-openssl-dev libopus-dev libogg-dev libjsoncpp-dev libopusfile-dev -y
RUN curl -L https://github.com/brainboxdotcc/DPP/releases/download/v10.0.30/libdpp-10.0.30-linux-x64.deb -o /tmp/libdpp.deb
RUN apt install ./tmp/libdpp.deb -y
COPY src /bot/src
COPY cmake /bot/cmake
COPY CMakeLists.txt /bot
WORKDIR /bot
RUN ["cmake","-B","/bot/build"]
RUN ["cmake","--build","/bot/build"]

FROM debian:unstable-slim
LABEL org.opencontainers.image.authors="Zynt, super.alekberov@proton.me"
RUN apt update && apt upgrade -y && apt install curl yt-dlp bzip2 fontconfig libcurl4 libopus0 libogg0 libjsoncpp2* libopusfile0 -y
RUN curl -L https://bitbucket.org/ariya/phantomjs/downloads/phantomjs-2.1.1-linux-x86_64.tar.bz2 -o /tmp/phantomjs.tar.bz2; tar -xjf /tmp/phantomjs.tar.bz2; mv phantomjs* /usr/local/share/phantomjs; ln -sf /usr/local/share/phantomjs/bin/phantomjs /usr/local/bin; mkdir -p /bot/cache /bot/media /bot/playlists
COPY lang /bot/lang
WORKDIR /bot
COPY --from=0 /tmp/libdpp.deb /tmp/libdpp.deb
RUN apt install /tmp/libdpp.deb -y && rm /tmp/libdpp.deb
COPY --from=0 /bot/build/beuyyub /bot
CMD ["/bot/beuyyub"]
