FROM debian:unstable-slim
LABEL org.opencontainers.image.authors="Zynt, super.alekberov@proton.me"
RUN apt update && apt upgrade -y && apt install curl libopus0 libogg0 libjsoncpp25 libopusfile0 yt-dlp -y
RUN curl https://dl.dpp.dev/latest -o /tmp/libdpp.deb
RUN apt install ./tmp/libdpp.deb -y && rm /tmp/libdpp.deb
RUN mkdir -p /bot/cache
RUN mkdir -p /bot/playlists
COPY build/beuyyub /bot/beuyyub
COPY lang /bot/lang
ENV BOT_TOKEN="NONE"
WORKDIR /bot
CMD echo $BOT_TOKEN > /tmp/token && /bot/beuyyub < /tmp/token