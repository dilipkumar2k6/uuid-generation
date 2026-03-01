FROM alpine:latest
RUN apk add --no-cache g++
WORKDIR /app
COPY app.cpp .
RUN g++ -o app app.cpp
CMD ["./app"]
