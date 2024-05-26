# Tape [![C++ CI](https://github.com/as3rg/tape-sort/actions/workflows/cpp.yml/badge.svg)](https://github.com/as3rg/tape-sort/actions/workflows/cpp.yml)
## Описание

Проект представляет из себя эмулятор устройства хранения типа 
[лента](https://ru.wikipedia.org/wiki/%D0%9C%D0%B0%D0%B3%D0%BD%D0%B8%D1%82%D0%BD%D0%B0%D1%8F_%D0%BB%D0%B5%D0%BD%D1%82%D0%B0). 
Данный тип устройств хранения представляет из себя головку, двигающуюся по ленте, на которой хранится информация. Возможны следующие операции с лентой:
- Чтение данных из ячейки под головкой
- Запись данных в ячейку под головкой
- Перемещение головки

Проект состоит из 4 разделов:
- [lib](./lib) &mdash; разделяемая библиотека, содержащая эмулятор ленты и функции сортировки
- [tests](./tests) &mdash; тесты к библиотеке [lib](./lib)
- [util](./util) &mdash; утилита, выполняющая сортировку переданной ей ленты
- [utilities](./utilities) &mdash; разделяемая библиотека, содержащая некоторые вспомогательные инструменты

### [Библиотека](./lib)

Данная библиотека состоит из двух частей:
- [Эмулятор магнитной ленты](./lib/include/tape.h)
- [Реализация](./lib/include/sorter.h) сортировки [quick sort](https://ru.wikipedia.org/wiki/%D0%91%D1%8B%D1%81%D1%82%D1%80%D0%B0%D1%8F_%D1%81%D0%BE%D1%80%D1%82%D0%B8%D1%80%D0%BE%D0%B2%D0%BA%D0%B0) на магнитных лентах

#### [Эмулятор магнитной ленты](./lib/include/tape.h)
Эмулятор магнитной ленты (класс `tape::tape`) позволяет создавать магнитную ленту на основе потоков (`std::istream` и `std::ostream`). 
Элементами ленты выступают `int32_t`.
- При создании ленты на основе `std::istream` пользователю доступны операции перемещения головки и чтения данных
- При создании ленты на основе `std::ostream` пользователю доступны операции перемещения головки и записи данных
- При создании ленты на основе класса, являющегося потомком `std::istream` и `std::ostream` (к примеру, `std::stringstream` или `std::fstream`), пользователю доступны все виды операций

Поддерживаются только потоки с возможностью перемещения указателя (поддержкой операции `seek`).

Размер ленты указывается при создании ленты и не может быть изменен.

Возможна настройка следующих параметров:
- Начальное положение головки на ленте
- Положение первого элемента в переданном потоке (отступ)

Лента может эмулировать задержки при выполнении операций. Величину задержек при различных видах операций можно настроить, передав в эмулятор экземпляр класса `tape::delay_config`.

#### [Сортировка](./lib/include/sorter.h)
В качестве алгоритма сортировки был выбран алгоритм [quick sort](https://ru.wikipedia.org/wiki/%D0%91%D1%8B%D1%81%D1%82%D1%80%D0%B0%D1%8F_%D1%81%D0%BE%D1%80%D1%82%D0%B8%D1%80%D0%BE%D0%B2%D0%BA%D0%B0) с использованием 3 дополнительных лент.

В данном случае магнитные ленты выступают в роли [стеков](https://ru.wikipedia.org/wiki/%D0%A1%D1%82%D0%B5%D0%BA).

Алгоритм работает следующим образом:
- На первом шаге данные кладутся на временную ленту `tmp1`
- Среди элементов выбирается случайный ключ `key`
- Элементы ленты `tmp1` разбиваются на два класса:
  - Элементы `< key` перемещаются на ленту `tmp2`
  - Элементы `>= key` перемещаются на ленту `tmp3`
- К элементам `tmp2` и `tmp3` применяется аналогичный алгоритм сортировки
- В случае, если размер данных не превосходит `chunk_size = M / 4`, сортировка проводится в оперативной памяти с использованием `std::sort`

Данный алгоритм использует факт, что на каждом шаге рекурсии вспомогательные ленты необязательно должны быть пустыми &mdash; 
достаточно запомнить, сколько элементов мы положили на последнем шаге.

Алгоритм не пишет во входную ленту и читает из выходной. 
Это позволяет открывать входную ленту только для чтения, а выходную &mdash; только для записи.

Алгоритм поддерживает произвольные компараторы. По умолчанию используется `std::less<int32_t>`.

Асимптотика алгоритма соответствует асимптотике классической быстрой сортировки.
На каждой итерации выполняется `n` операций чтения, `n` операций записи и `2n` операций перемещения головки на соседнюю позицию.

- Средняя сложность &mdash; `O(n log n)`
- Среднее количество операций чтения  &mdash;`O(n log n)`
- Среднее количество операций записи  &mdash;`O(n log n)`
- Среднее количество операций перемещения головки &mdash; `O(n log n)`
- Среднее пройденное головкой расстояние &mdash; `O(n log n)`

### [Утилита](./util)
Утилита позволяет создавать эмуляторы лент на основе переданных файлов и применять к ним алгоритм сортировки. 

Аргументы утилиты:
- **input-file** &mdash; путь ко входному файлу (открывается в режиме _read-only_)
- **output-file** &mdash; путь к выходному файлу (открывается в режиме _write-only_)
- **input-tape-size** [опционально] &mdash; размер входных данных. (если не указано, считается автоматически)
- **memory-limit** [опционально] &mdash; ограничение на количество используемой памяти, байты (по умолчанию 0)

В ходе работы программы утилита может создавать до трех файлов в директории `./tmp/`. Файлы открываются в режиме _read-write_.

Для эмуляции задержек необходимо создать конфигурационный файл `config.txt` со следующим форматом:
```
read-delay 0
write-delay 0
rewind-step-delay 0
rewind-delay 0 
next-delay 0
```
Вместо `0` необходимо указать задержки в наносекундах. 
Некоторые параметры могут быть не указаны (в таком случае они считаются равными `0`).


Данные во входных и выходных файлах интерпретируются как последовательности байт &mdash; по 4 байта на ячейку ленты.

К примеру, рассмотрим файл со следующим содержанием:
```
aaaabbbb11112222
```
Данный файл будет интерпретирован как последовательность байт:
```
0x61 0x61 0x61 0x61 0x62 0x62 0x62 0x62 0x31 0x31 0x31 0x31 0x32 0x32 0x32 0x32
```
Данному файлу соответствует следующая лента:
```
0x61616161 0x62626262 0x31313131 0x32323232
```
или
```
1633771873 1650614882 825307441 842150450
```

## Запуск
Требования:
- CMake
- Make
- Git & GTest/Vcpkg (для запуска тестов)

### Запуск утилиты

Чтобы собрать утилиту, введите: 
```
make build
```

После сборки программу можно запустить одной из следующих команд (вместо `...` подставьте аргументы утилиты): 

```
(1) ./build/util/tape-util ...
(2) make run args="..."
```

### Запуск тестов

Сборка тестов:

- Если у вас уже установлен GTest (к примеру, в виде разделаемой библиотеки):
  ```
  make build-tests
  ```
- Если у вас не установлен GTest, но установлен Vcpkg (вместо `...` укажите путь до директории, содержащей Vcpkg): 
  ```
  make build-tests VCPKG="..."
  ```
- Иначе:
  ```
  git clone https://github.com/microsoft/vcpkg && \
  ./vcpkg/bootstrap-vcpkg.sh -disableMetrics && \
  make build-tests VCPKG=.
  ```

Тесты можно запустить одной из следующих: 
```
(1) ./build/tests/tape-tests
(2) make tests
```

### Очистка

Чтобы удалить результаты сборки, введите:
```
make clean
```