# MCP server image for CCExtractor
#
# This image expects that /ccextractor is available, either by
# starting FROM the main ccextractor image or by copying in the
# binary at build-time.

FROM ccextractor:latest AS base

# Install Node.js (Debian/Ubuntu-based image assumed)
RUN apt-get update && apt-get install -y --no-install-recommends \
    nodejs npm \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy MCP server sources
COPY package.json tsconfig.json ./
COPY src ./src

RUN npm install --omit=dev && npm run build

ENV CCEXTRACTOR_PATH=/ccextractor \
    CCEXTRACTOR_WORKDIR=/data

WORKDIR /data

ENTRYPOINT ["node", "/app/dist/server.js"]
