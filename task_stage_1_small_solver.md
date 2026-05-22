# Задание разработчику: этап 1 — точный решатель для малых размеров

## 1. Цель этапа

Реализовать первую исследовательскую версию C++-проекта `dandelions-solver`, которая умеет точно определять победителя игры «Одуванчики и семена» для малых размеров поля и собирать статистику поиска.

Основная цель этапа — не максимальная производительность на `6 × 6`, а проверка корректности модели, архитектуры и базовых оптимизаций на малых `n, k`.

Ориентироваться на README проекта, особенно на разделы:

- `1. Краткое описание игры`
- `2. Точные правила, которые используются в проекте`
- `3. Математическая постановка`
- `5. Представление поля и координат`
- `6. Предрасчёт распространения семян`
- `7. Minimax и alpha-beta-подобное отсечение`
- `8. Мемоизация`
- `9. Канонизация состояния`
- `12. Статистика поиска`
- `15. Тестирование`
- `16. Исследовательский план`
- `21. Критерии готовности первой исследовательской версии`

---

## 2. Объём работ на этом этапе

Нужно реализовать:

1. C++-проект с CMake.
2. Модель поля `n × n` для `n ≤ 8`.
3. Хранение состояния через битовые маски `uint64_t`.
4. Предрасчёт лучей `ray[cell][dir]`.
5. Применение посадки одуванчика.
6. Применение ветра.
7. Проверку победы.
8. Точный рекурсивный minimax-решатель.
9. Мемоизацию состояний.
10. Alpha-beta-подобные булевские отсечения.
11. Канонизацию состояния через 8 симметрий квадратного поля.
12. Сбор и вывод базовой статистики поиска.
13. CLI-запуск для одного набора параметров.
14. CLI-запуск серии малых экспериментов.
15. Unit-тесты для геометрии, правил и solver-а на малых случаях.

На этом этапе не требуется:

- визуализация игры;
- интерактивный интерфейс;
- полноценное восстановление дерева стратегии;
- оптимизация под `6 × 6, k = 7`;
- GPU;
- поддержка `n > 8`;
- поддержка `k > 8`.

---

## 3. Ожидаемая структура проекта

Необходимо создать следующую структуру:

```text
dandelions-solver/
  README.md
  CMakeLists.txt
  src/
    main.cpp
    geometry.h
    geometry.cpp
    state.h
    state.cpp
    canonical.h
    canonical.cpp
    solver.h
    solver.cpp
    stats.h
    stats.cpp
  tests/
    test_geometry.cpp
    test_rules.cpp
    test_solver_small.cpp
  results/
    .gitkeep
```

На этом этапе модули `strategy.*` можно не реализовывать. В README они указаны для будущих этапов.

---

## 4. Наполнение файлов

### 4.1. `CMakeLists.txt`

Должен собирать:

1. Основной исполняемый файл:

```text
dandelions_solver
```

2. Тестовый исполняемый файл:

```text
dandelions_tests
```

Требования:

- стандарт C++17 или C++20;
- сборка через CMake;
- без тяжёлых внешних зависимостей на первом этапе;
- тестовый фреймворк можно выбрать простой: Catch2, doctest или GoogleTest.

Если используется внешний тестовый фреймворк, способ его подключения должен быть описан в комментарии или отдельном коротком разделе README.

---

### 4.2. `src/geometry.h` / `src/geometry.cpp`

Отвечают за геометрию поля.

Реализовать:

```cpp
struct Coord {
    int x;
    int y;
};
```

Класс или набор функций для:

- хранения размера поля `n`;
- перевода `(x, y)` в `cell_id`;
- перевода `cell_id` в `(x, y)`;
- проверки попадания координат в поле;
- описания 8 направлений ветра;
- предрасчёта `ray[cell][dir]`;
- предрасчёта симметрий клеток;
- предрасчёта симметрий направлений;
- получения `full_board_mask`.

Минимальный интерфейс может выглядеть так:

```cpp
class Geometry {
public:
    explicit Geometry(int n);

    int size() const;
    int cellCount() const;
    uint64_t fullBoardMask() const;

    int cellId(int x, int y) const;
    Coord coord(int cell_id) const;
    bool inside(int x, int y) const;

    uint64_t ray(int cell_id, int dir) const;

    int transformCell(int sym, int cell_id) const;
    int transformDir(int sym, int dir) const;
    int inverseSymmetry(int sym) const;
};
```

Порядок направлений должен соответствовать README, раздел `2.3`.

---

### 4.3. `src/state.h` / `src/state.cpp`

Отвечают за представление состояния и базовые операции игры.

Реализовать структуру:

```cpp
struct State {
    uint64_t dandelions;
    uint64_t occupied;
    uint8_t used_dirs;
};
```

Реализовать функции:

```cpp
bool isValidState(const State& state, const Geometry& geometry);
bool isTerminalWin(const State& state, const Geometry& geometry);
bool isTerminalLoss(const State& state, int k);
int roundIndex(const State& state);
uint64_t plantableMask(const State& state, const Geometry& geometry);
State applyPlant(const State& state, int cell_id);
uint64_t blowMask(const State& state, int dir, const Geometry& geometry);
State applyWind(const State& state, int dir, const Geometry& geometry);
```

Важное правило:

```text
plantable = full_board_mask & ~dandelions
```

То есть посадка на семя разрешена, посадка на одуванчик запрещена.

---

### 4.4. `src/canonical.h` / `src/canonical.cpp`

Отвечают за канонизацию состояния.

Реализовать:

```cpp
struct StateKey {
    uint64_t dandelions;
    uint64_t occupied;
    uint8_t used_dirs;

    bool operator==(const StateKey& other) const;
};
```

Реализовать hash для `StateKey`.

Реализовать:

```cpp
struct CanonicalResult {
    StateKey key;
    int symmetry;
};

StateKey makeKey(const State& state);
State transformState(const State& state, int sym, const Geometry& geometry);
CanonicalResult canonicalize(const State& state, const Geometry& geometry);
```

Лексикографический порядок сравнения — как в README, раздел `9.5`:

1. `occupied`
2. `dandelions`
3. `used_dirs`

Канонизация должна преобразовывать не только клетки, но и направления `used_dirs`.

---

### 4.5. `src/stats.h` / `src/stats.cpp`

Отвечают за статистику поиска.

Реализовать структуру:

```cpp
struct SearchStats {
    int n = 0;
    int k = 0;

    uint64_t recursive_calls = 0;
    uint64_t memo_hits = 0;
    uint64_t unique_states_stored = 0;
    uint64_t canonicalization_calls = 0;

    uint64_t terminal_win_hits = 0;
    uint64_t terminal_loss_hits = 0;

    uint64_t plant_moves_considered = 0;
    uint64_t wind_moves_considered = 0;

    uint64_t plant_cutoffs = 0;
    uint64_t wind_cutoffs = 0;

    int max_depth = 0;
    long long elapsed_ms = 0;
};
```

Реализовать:

```cpp
void printStatsHumanReadable(const SearchStats& stats, bool winner);
void appendStatsCsv(const std::string& path, const SearchStats& stats, bool winner);
```

CSV должен содержать минимум:

```text
n,k,winner,elapsed_ms,recursive_calls,unique_states,memo_hits,plant_cutoffs,wind_cutoffs,max_depth
```

---

### 4.6. `src/solver.h` / `src/solver.cpp`

Основной точный решатель.

Реализовать класс:

```cpp
struct SolverConfig {
    int n;
    int k;
    bool use_memo = true;
    bool use_symmetry = true;
    bool use_move_ordering = true;
};

struct SolveResult {
    bool first_wins;
    SearchStats stats;
};

class Solver {
public:
    explicit Solver(SolverConfig config);

    SolveResult solve();

private:
    bool canFirstWin(const State& state);
};
```

В `canFirstWin` реализовать логику из README, раздел `7.4`.

Обязательно:

- проверять `occupied == full_board_mask`;
- проверять достижение `k` раундов;
- использовать memo, если `use_memo = true`;
- использовать canonicalize, если `use_symmetry = true`;
- считать статистику;
- применять отсечения:
  - остановка перебора посадок при найденной выигрышной посадке;
  - остановка перебора ветров при найденном опровергающем ветре.

На этом этапе достаточно хранить в memo только результат `bool`.

Сохранение стратегии не требуется.

---

### 4.7. `src/main.cpp`

CLI для запуска решателя.

Минимальные режимы:

#### Один запуск

```bash
./dandelions_solver --n 4 --k 3
```

Вывод:

```text
n = 4
k = 3
winner = P1/P2
elapsed_ms = ...
recursive_calls = ...
unique_states = ...
memo_hits = ...
...
```

#### Серия запусков

```bash
./dandelions_solver --batch-small --stats-csv results/summary.csv
```

Должен запускать набор малых тестов, например:

```text
n = 1..4
k = 1..min(8, n*n)
```

Можно ограничить сетку, если какие-то случаи начинают считаться слишком долго, но это должно быть явно видно в коде или параметрах.

Полезные флаги:

```bash
--no-memo
--no-symmetry
--no-move-ordering
--stats-csv path
```

На первом этапе можно реализовать простой ручной парсинг аргументов без внешних библиотек.

---

## 5. Упорядочивание ходов

Реализовать простую сортировку, если `use_move_ordering = true`.

### 5.1. Посадки игрока 1

Сначала проверять посадки с большим потенциальным покрытием.

Простая оценка:

```text
score_plant(p) = min over available dirs of
                 popcount(blow(D ∪ {p}, dir) & ~occupied)
```

Сортировать по убыванию `score_plant`.

### 5.2. Ветры игрока 2

Сначала проверять ветры, которые дают минимальный прирост покрытия.

Оценка:

```text
score_dir(dir) = popcount(blow(D, dir) & ~occupied)
```

Сортировать по возрастанию `score_dir`.

---

## 6. Тесты

### 6.1. `tests/test_geometry.cpp`

Проверить:

- корректность `cellId` и `coord`;
- корректность `fullBoardMask`;
- лучи для прямых направлений;
- лучи для диагоналей;
- симметрии клеток;
- симметрии направлений;
- обратные симметрии.

### 6.2. `tests/test_rules.cpp`

Проверить:

- посадка в пустую клетку разрешена;
- посадка в клетку с семенем разрешена;
- посадка в клетку с одуванчиком запрещена через `plantableMask`;
- ветер засевает клетки до края поля;
- повторное направление не должно выбираться solver-ом;
- после `k` раундов игра завершается.

### 6.3. `tests/test_solver_small.cpp`

Проверить solver на малых случаях:

```text
n = 1, k = 1
n = 2, k = 1
n = 2, k = 2
n = 3, k = 1
n = 3, k = 2
```

Для этих тестов главное — не заранее угадать все математические ответы, а проверить стабильность:

- solver завершается;
- результат одинаковый с memo и без memo;
- результат одинаковый с symmetry и без symmetry;
- результат одинаковый с move ordering и без move ordering.

То есть для каждого малого случая сравнить несколько конфигураций решателя.

---

## 7. Критерии приёмки

Этап считается выполненным, если:

1. Проект собирается через CMake.
2. Основной бинарник `dandelions_solver` запускается из CLI.
3. Можно запустить один расчёт через `--n` и `--k`.
4. Можно запустить серию малых расчётов через `--batch-small`.
5. Состояние хранится через `uint64_t dandelions`, `uint64_t occupied`, `uint8_t used_dirs`.
6. Реализованы `ray[cell][dir]`.
7. Реализован точный minimax.
8. Реализована memoization.
9. Реализована canonicalization по 8 симметриям.
10. Реализованы булевские отсечения.
11. Выводится человекочитаемая статистика.
12. CSV со статистикой создаётся при указании `--stats-csv`.
13. Unit-тесты проходят.
14. Для малых `n, k` результаты совпадают при включении и отключении memo/symmetry/move ordering.
15. Код структурирован по файлам, описанным в этом задании.

---

## 8. Что важно не делать на этом этапе

Не нужно преждевременно усложнять проект.

Не реализовывать в первом этапе:

- GUI;
- визуализацию поля;
- восстановление полного дерева стратегии;
- многопоточность;
- GPU;
- поддержку больших bitset-ов;
- сложные эвристические отсечения;
- внешнюю базу данных;
- web-интерфейс.

Главный фокус этапа:

```text
корректная модель + точный solver + базовая статистика + проверка на малых размерах
```

---

## 9. Ожидаемый результат работы разработчика

В результате должен быть передан репозиторий/папка проекта, в которой можно выполнить:

```bash
cmake -S . -B build
cmake --build build --config Release
```

Затем:

```bash
./build/dandelions_solver --n 3 --k 2
```

и:

```bash
./build/dandelions_solver --batch-small --stats-csv results/summary.csv
```

А также запустить тесты:

```bash
ctest --test-dir build
```

И получить:

- ответ о победителе;
- статистику поиска;
- CSV-файл результатов для серии малых запусков;
- проходящие unit-тесты.

