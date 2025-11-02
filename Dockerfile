FROM alpine:3.19

RUN apk add --no-cache build-base cmake curl-dev

WORKDIR /app

COPY . .

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --config Release

CMD ["sh", "-c", "./build/habr_parser --input tests/fixtures/habr_example.html"]
