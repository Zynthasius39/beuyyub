FROM debian:unstable-slim
LABEL org.opencontainers.image.authors="Zynt, super.alekberov@proton.me"
RUN apt update && apt upgrade -y && apt install curl cmake g++ libmpg123-dev libcurl4-openssl-dev libopus-dev libogg-dev libjsoncpp-dev libopusfile-dev -y
RUN curl https://dl.dpp.dev/latest -o /tmp/libdpp.deb
RUN apt install ./tmp/libdpp.deb -y
COPY src /bot/src
COPY cmake /bot/cmake
COPY CMakeLists.txt /bot
WORKDIR /bot
RUN ["cmake","-B","/bot/build"]
RUN ["cmake","--build","/bot/build"]

FROM debian:unstable-slim
LABEL org.opencontainers.image.authors="Zynt, super.alekberov@proton.me"
RUN apt update && apt upgrade -y && apt install yt-dlp libcurl4 libopus0 libogg0 libjsoncpp25 libopusfile0 -y
COPY --from=0 /tmp/libdpp.deb /tmp/libdpp.deb
RUN apt install ./tmp/libdpp.deb -y && rm /tmp/libdpp.deb
RUN mkdir -p /bot/cache /bot/media /bot/playlists
COPY lang /bot/lang
COPY --from=0 /bot/build/beuyyub /bot
WORKDIR /bot
CMD ["/bot/beuyyub"]
