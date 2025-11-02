# Habr C Parser

Проект на C11, который парсит HTML-разметку карточек статей Habr и выводит результаты в CSV.

## Сборка

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Режим фикстуры

```bash
./build/habr_parser --input tests/fixtures/habr_example.html > out.csv
```

## Режим поиска

```bash
./build/habr_parser -q "golang" --max 100 --delay-ms 300 --timeout 15 --lang en > out.csv
```

## Docker

```bash
docker build -t habr-c-parser .
docker run --rm habr-c-parser
```

Передать другие аргументы можно так:

```bash
docker run --rm habr-c-parser ./build/habr_parser -q "ai" --max 100
```
