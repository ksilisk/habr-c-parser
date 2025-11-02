# Habr C Parser

A C11 project that parses the HTML markup of Habr article cards and outputs the results in CSV format.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Fixture Mode

```bash
./build/habr_parser --input tests/fixtures/habr_example.html > out.csv
```

## Search Mode

```bash
./build/habr_parser -q "golang" --max 100 --delay-ms 300 --timeout 15 --lang en > out.csv
```

## Docker

```bash
docker build -t habr-c-parser .
docker run --rm habr-c-parser
```

To pass custom arguments, use:

```bash
docker run --rm habr-c-parser ./build/habr_parser -q "ai" --max 100
```
