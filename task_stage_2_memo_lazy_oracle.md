# ТЗ разработчику: этап 2 — компактная memo-таблица и lazy oracle утилита для анализа стратегий

## 1. Контекст проекта

Проект `dandelions-solver` — C++/CMake-решатель игры «Одуванчики и семена».

В первом этапе уже реализованы:

- C++/CMake-проект;
- модель поля `n × n`, `n <= 8`;
- битовые маски состояния;
- предрасчёт лучей `ray[cell][dir]`;
- правила посадки и ветра;
- minimax-решатель;
- memoization;
- canonicalization по 8 симметриям квадратного поля;
- move ordering;
- статистика поиска;
- CLI-бинарник `dandelions_solver.exe`;
- тестовый бинарник `dandelions_tests.exe`.

На текущем этапе нужно решить две задачи:

1. Снизить потребление памяти memo-таблицы, чтобы приблизиться к решению больших случаев вроде `6 × 6, k = 7`.
2. Добавить отдельную интерактивную exe-утилиту для исследования стратегий игры через lazy oracle.

---

## 2. Цели этапа

### 2.1. Оптимизация memo

Заменить текущую memo-таблицу на специализированную компактную hash table:

- без `std::unordered_map` в основном solver-е;
- с packed key;
- с SoA-хранением;
- с open addressing;
- с объединением `value` и `control` в один байт `meta`.

### 2.2. Сохранение и загрузка memo

Добавить бинарный формат сохранения memo-базы в файл `.dsmem`.

Выбранный формат хранения: **raw table snapshot**.

То есть сохраняются не только занятые entries, а вся внутренняя структура bucket-ов memo-таблицы, чтобы файл быстро загружался обратно.

### 2.3. Lazy oracle утилита

Добавить отдельный CLI-бинарник:

```text
dandelions_play.exe
```

Он должен имитировать ход партии, показывать пользователю текущее состояние, список возможных ходов и оценку каждого хода:

```text
P1 — после этого хода при оптимальной игре выигрывает игрок 1
P2 — после этого хода при оптимальной игре выигрывает игрок 2
UNKNOWN — состояние ещё не найдено в memo-базе
```

При заходе пользователя в `UNKNOWN`-состояние утилита должна запускать solver из этого состояния, сохранять результат в memo и показывать новое value текущего состояния.

---

## 3. Термины и модель состояния

Состояние игры хранится как:

```cpp
struct State {
    uint64_t dandelions;    // клетки с одуванчиками
    uint64_t occupied;      // все занятые клетки: одуванчики + семена
    uint8_t used_dirs;      // использованные направления ветра
};
```

Инвариант:

```text
dandelions ⊆ occupied
```

Раунд:

```text
round = popcount(used_dirs)
```

В игре есть две фазы:

```text
P1_TO_PLANT — ход игрока 1, нужно выбрать клетку для посадки
P2_TO_BLOW  — ход игрока 2, нужно выбрать направление ветра после посадки P1
```

Текущая memo-функция solver-а естественно хранит значения только для состояний фазы `P1_TO_PLANT`:

```text
Win(state) = true, если P1 выигрывает из состояния перед своей посадкой
Win(state) = false, если P2 выигрывает из состояния перед посадкой P1
```

Для фазы `P2_TO_BLOW` value вычисляется агрегацией по доступным ветрам:

```text
P2_TO_BLOW value = P1, если все доступные ветры ведут в Win(next_state) = true
P2_TO_BLOW value = P2, если хотя бы один доступный ветер ведёт в Win(next_state) = false
UNKNOWN, если для принятия решения не хватает memo-значений и lazy-досчёт не был выполнен
```

---

## 4. Оптимизированная memo-таблица

### 4.1. Общие требования

Memo-таблица должна быть специализированной под solver:

- insert-only;
- no erase;
- open addressing;
- capacity — степень двойки;
- lookup через `hash & mask`;
- поддержка `reserve` / начальной capacity;
- контроль load factor;
- корректный rehash при превышении load factor;
- возможность сериализации raw snapshot в файл;
- возможность загрузки raw snapshot из файла.

Минимальный API:

```cpp
enum class LookupResult {
    NotFound,
    FoundP1,
    FoundP2
};

class MemoTable {
public:
    LookupResult find(const PackedKey& key) const;
    void insert(const PackedKey& key, bool first_wins);

    size_t size() const;
    size_t capacity() const;
    double loadFactor() const;

    void reserve(size_t expected_entries);
    void clear();
};
```

Можно не использовать `virtual` в горячем пути. Предпочтительно реализовать через конкретные классы или template-обёртку, чтобы не платить за динамический dispatch.

---

### 4.2. Packed key

Нужно реализовать упаковку canonical state key.

#### Для `n <= 5`

Для `n <= 5` весь ключ помещается в `uint64_t`.

Максимум для `5 × 5`:

```text
occupied:    25 бит
dandelions: 25 бит
used_dirs:   8 бит
------------------
итого:      58 бит
```

Формат:

```text
bits 0..m-1       = occupied
bits m..2m-1      = dandelions
bits 2m..2m+7     = used_dirs
```

где:

```text
m = n * n
```

#### Для `n == 6`

Для `6 × 6` ключ занимает 80 бит.

```text
occupied:    36 бит
dandelions: 36 бит
used_dirs:   8 бит
------------------
итого:      80 бит
```

Использовать представление:

```cpp
struct Key80 {
    uint64_t lo;
    uint16_t hi;
};
```

Рекомендуемая упаковка:

```text
lo bits 0..35   = occupied
lo bits 36..63  = младшие 28 бит dandelions
hi bits 0..7    = старшие 8 бит dandelions
hi bits 8..15   = used_dirs
```

Примерная функция:

```cpp
Key80 packKey80(uint64_t occupied, uint64_t dandelions, uint8_t used_dirs) {
    Key80 key;

    key.lo =
        (occupied & ((1ULL << 36) - 1)) |
        ((dandelions & ((1ULL << 28) - 1)) << 36);

    key.hi =
        static_cast<uint16_t>((dandelions >> 28) & 0xFF) |
        static_cast<uint16_t>(static_cast<uint16_t>(used_dirs) << 8);

    return key;
}
```

Для `n > 6` на этом этапе можно оставить старое представление или явно вернуть ошибку `unsupported optimized key format`. Приоритет этапа — `n <= 6`.

---

### 4.3. SoA-хранение

Для `n == 6` memo должна храниться в SoA-массивы:

```cpp
std::vector<uint64_t> key_lo;
std::vector<uint16_t> key_hi;
std::vector<uint8_t>  meta;
```

Для `n <= 5`:

```cpp
std::vector<uint64_t> key_lo;
std::vector<uint8_t>  meta;
```

То есть `key_hi` нужен только для формата `key80`.

---

### 4.4. Однобайтная `meta`

`meta` объединяет control byte, fingerprint и value.

Формат:

```text
meta == 0 → empty bucket

meta != 0:
    bit 7      = value
    bits 0..6  = 7-bit fingerprint
```

Где:

```text
value = 1 → P1 выигрывает из состояния
value = 0 → P2 выигрывает из состояния
```

Fingerprint должен быть ненулевым, потому что `0` зарезервирован для empty bucket.

Пример:

```cpp
inline uint8_t makeFingerprint(uint64_t hash) {
    uint8_t fp = static_cast<uint8_t>((hash >> 57) & 0x7F);
    return fp == 0 ? 1 : fp;
}

inline uint8_t makeMeta(uint64_t hash, bool value) {
    uint8_t fp = makeFingerprint(hash);
    return static_cast<uint8_t>(fp | (value ? 0x80 : 0x00));
}

inline bool isEmpty(uint8_t meta) {
    return meta == 0;
}

inline uint8_t metaFingerprint(uint8_t meta) {
    return meta & 0x7F;
}

inline bool metaValue(uint8_t meta) {
    return (meta & 0x80) != 0;
}
```

Важно: fingerprint используется только для ускорения. Корректность должна обеспечиваться полным сравнением ключа.

---

### 4.5. Probing

Для первой версии достаточно linear probing:

```cpp
idx = hash & mask;

while bucket occupied:
    if fingerprint совпал и полный ключ совпал:
        found
    idx = (idx + 1) & mask;
```

Удаления не нужны, tombstone не нужен.

Рекомендуемый максимальный load factor:

```text
0.70–0.80
```

По умолчанию можно использовать `0.75`.

---

### 4.6. Hash-функция

Нужна быстрая стабильная hash-функция для packed key.

Требования:

- deterministic между запусками;
- версия hash-функции должна быть записана в `.dsmem` header;
- если hash-функция меняется, старый raw snapshot нельзя грузить как совместимый.

Можно использовать собственный mix на базе splitmix64.

Пример для `key64`:

```cpp
uint64_t hashKey64(uint64_t key) {
    return splitmix64(key ^ HASH_SEED);
}
```

Пример для `key80`:

```cpp
uint64_t hashKey80(uint64_t lo, uint16_t hi) {
    uint64_t h = splitmix64(lo ^ HASH_SEED);
    h ^= splitmix64(static_cast<uint64_t>(hi) + 0x9e3779b97f4a7c15ULL);
    return splitmix64(h);
}
```

---

## 5. Сохранение и загрузка memo-файлов `.dsmem`

### 5.1. Расширение и название файла

Расширение:

```text
.dsmem
```

Пример названия:

```text
memo_n06_k07_rules01_canon_key80_scope-lazy_v001.dsmem
```

Рекомендуемые элементы имени:

```text
n
k
rules version
canonicalization flag
key format
scope
file version
```

Важно: имя файла только помогает человеку. Источник истины — header внутри файла.

---

### 5.2. `.gitignore`

Обязательно добавить в `.gitignore` сохранённые memo-файлы и папки кэша.

Минимум:

```gitignore
# Solver memo/cache files
*.dsmem
cache/
memo/
results/*.dsmem
```

Если результаты CSV нужно хранить в git, не игнорировать весь `results/` целиком. Игнорировать только тяжёлые бинарные memo-файлы.

---

### 5.3. Header файла

Реализовать бинарный header фиксированного размера.

Пример:

```cpp
#pragma pack(push, 1)
struct MemoFileHeader {
    char magic[8];              // "DSMEM01\0" или аналогичный magic
    uint32_t file_version;      // версия формата файла

    uint8_t n;
    uint8_t k;

    uint8_t rules_version;      // версия правил игры
    uint8_t direction_scheme;   // версия порядка направлений
    uint8_t key_format;         // 1=key64, 2=key80
    uint8_t canonical_keys;     // 0/1
    uint8_t db_scope;           // 1=proof, 2=lazy, 3=full
    uint8_t complete;           // 0/1

    uint64_t rules_hash;        // hash правил/геометрии/направлений
    uint64_t hash_seed;         // seed hash-функции
    uint32_t hash_version;      // версия hash-функции

    uint64_t bucket_capacity;   // размер raw arrays
    uint64_t entries_count;     // занятых bucket-ов

    uint8_t root_result;        // 0=P2, 1=P1, 255=unknown/not_applicable
    uint8_t reserved[64];       // запас под будущие поля
};
#pragma pack(pop)
```

Можно скорректировать поля, но header обязательно должен позволять проверить совместимость загружаемой базы с текущими параметрами.

---

### 5.4. Проверки при загрузке

При загрузке `.dsmem` утилита должна проверить:

- `magic`;
- `file_version`;
- `n`;
- `k`;
- `rules_version`;
- `direction_scheme`;
- `key_format`;
- `canonical_keys`;
- `rules_hash`;
- `hash_seed`;
- `hash_version`;
- что `bucket_capacity` является степенью двойки;
- что размер файла соответствует ожидаемому размеру raw snapshot.

Если проверка не проходит, файл не загружать и вывести понятную ошибку.

---

### 5.5. Тело файла

Для `key64`:

```text
Header
key_lo[bucket_capacity] : uint64_t
meta[bucket_capacity]   : uint8_t
```

Для `key80`:

```text
Header
key_lo[bucket_capacity] : uint64_t
key_hi[bucket_capacity] : uint16_t
meta[bucket_capacity]   : uint8_t
```

Формат little-endian. На первом этапе можно считать целевой платформой Windows/x64 little-endian.

---

### 5.6. Сохранение частичной базы

Если solver остановлен по лимиту времени, памяти или числа состояний, он всё равно должен иметь возможность сохранить memo-файл с:

```text
complete = 0
```

Если solver завершился полностью:

```text
complete = 1
```

Для lazy oracle допустимо использовать неполную базу.

---

## 6. Изменения в основном solver CLI

Обновить `dandelions_solver.exe`.

Добавить флаги:

```bash
--save-memo path
--load-memo path
--memo-format optimized
--max-states N
--time-limit-sec T
--progress-every-sec S
--memory-limit-mb M
```

### 6.1. `--save-memo`

После завершения или остановки по лимиту сохранить memo-таблицу в `.dsmem`.

### 6.2. `--load-memo`

Загрузить существующий `.dsmem` и использовать его как стартовую memo-базу.

Если в процессе решения появляются новые состояния, они должны добавляться в таблицу.

### 6.3. Лимиты

При достижении лимитов программа должна завершаться корректно:

- не падать;
- вывести статистику;
- при наличии `--save-memo` сохранить частичную memo-базу;
- поставить `complete = 0`.

Минимально обязательные лимиты:

```text
--max-states
--time-limit-sec
--progress-every-sec
```

`--memory-limit-mb` желателен. Если сложно получить фактическую RAM кроссплатформенно, можно реализовать оценку памяти по capacity таблицы и вывести это явно.

---

## 7. Lazy oracle утилита `dandelions_play.exe`

### 7.1. Назначение

Утилита нужна для интерактивного исследования стратегий.

Она должна позволять пользователю проходить партию шаг за шагом, видеть текущее состояние, оценку текущего состояния и оценку каждого возможного хода.

---

### 7.2. CLI

Пример запуска:

```bash
dandelions_play.exe --n 5 --k 7 --memo cache/memo_n05_k07_rules01_canon_key64_scope-lazy_v001.dsmem
```

Флаги:

```bash
--n N
--k K
--memo path
--autosave
--save-on-exit
--time-limit-sec T
--max-states N
--progress-every-sec S
```

Если файл `--memo` существует, загрузить его.

Если файл `--memo` не существует, создать новую пустую memo-таблицу для указанных `n, k`.

При выходе по `save`, `quit` с `--save-on-exit` или при `--autosave` сохранять базу обратно в тот же файл.

---

### 7.3. Текущий игровой state в утилите

Утилита должна хранить:

```cpp
struct PlayNode {
    State state;
    Phase phase; // P1_TO_PLANT или P2_TO_BLOW
    optional<int> last_planted_cell; // полезно для P2 phase
};
```

Также нужна история для команды `back`:

```cpp
std::vector<PlayNode> history;
```

Команда `back` возвращает на предыдущее состояние.

---

### 7.4. Value текущего состояния

На каждом шаге утилита должна выводить value текущего состояния:

```text
current value = P1 / P2 / UNKNOWN
```

Для `P1_TO_PLANT`:

```text
value = Win(state)
```

Для `P2_TO_BLOW`:

```text
value = P1, если все доступные ветры ведут в P1
value = P2, если хотя бы один доступный ветер ведёт в P2
value = UNKNOWN, если для вывода не хватает данных
```

Если пользователь заходит в `UNKNOWN`-состояние, нужно запустить solver из этого состояния и после расчёта показать обновлённый value.

---

### 7.5. Lazy-досчёт

Если нужное состояние отсутствует в memo:

- при простом отображении списка ходов можно показывать `UNKNOWN`;
- если пользователь выбирает ход, ведущий в `UNKNOWN`, утилита должна запустить solver из нового текущего состояния;
- после завершения solver-а результат должен быть добавлен в memo;
- затем нужно снова вывести текущее состояние уже с value `P1` или `P2`, если расчёт завершился полностью.

Если solver был остановлен по лимиту, value может остаться `UNKNOWN`; нужно вывести понятное сообщение.

---

## 8. Отображение состояния ASCII-графикой

На каждом шаге выводить поле в консоль.

Символы:

```text
.  пустая клетка
*  семя
X  одуванчик
```

Пример:

```text
y=5  . . * . X .
y=4  . * . . . .
y=3  X * * * * *
y=2  . . . . . .
y=1  . . . * . .
y=0  . . . . . .
     0 1 2 3 4 5
```

Если в клетке одновременно есть seed и dandelion, отображать `X`.

Также выводить:

```text
n, k
round
phase
used directions
current value
occupied count / total cells
```

---

## 9. Список ходов и оценка ходов

### 9.1. Ход P1: посадка

Если текущая фаза `P1_TO_PLANT`, показать все допустимые посадки:

```text
cell     result     refuting_winds
----------------------------------
(0,0)    P2         E, NE
(0,1)    UNKNOWN    ?
(2,2)    P1         -
```

Посадка допустима, если в клетке нет одуванчика:

```text
plantable = full_board_mask & ~dandelions
```

Посадка на семя разрешена.

Оценка посадки:

1. Применить посадку.
2. Если поле заполнено после посадки — результат `P1`.
3. Иначе рассмотреть все доступные ветры P2.
4. Если все ветры ведут в `P1` — посадка имеет результат `P1`.
5. Если хотя бы один ветер ведёт в `P2` — посадка имеет результат `P2`.
6. Если есть неизвестные successor-состояния и недостаточно информации для вывода — `UNKNOWN`.

Для посадок с результатом `P2` вывести `refuting_winds` — направления ветра, после которых дальнейшее состояние выигрышно для P2.

Если среди ветров есть `UNKNOWN`, можно показывать их отдельно:

```text
unknown_winds: N, SW
```

---

### 9.2. Ход P2: ветер

Если текущая фаза `P2_TO_BLOW`, показать все доступные направления:

```text
dir     result
--------------
E       P1
W       P2
N       UNKNOWN
SW      P2
```

Оценка ветра:

1. Применить ветер.
2. Получить состояние фазы `P1_TO_PLANT`.
3. Посмотреть `Win(next_state)` в memo.
4. Если нет значения — показать `UNKNOWN`.

Если `result = P2`, это хороший/опровергающий ход для игрока 2.

---

## 10. Команды интерактивной утилиты

Минимальные команды:

```text
help              показать команды
state             показать текущее состояние и ASCII-поле
moves             показать возможные ходы с оценками
play <move>       сделать ход
back              вернуться на предыдущее состояние
save              сохранить memo-файл
quit              выйти
```

Рекомендуемые алиасы:

```text
q                 quit
b                 back
m                 moves
s                 state
```

Форматы `play <move>`:

Для P1:

```text
play 2 3
play (2,3)
play cell 2 3
```

Для P2:

```text
play E
play NE
play SW
```

После каждого `play` утилита должна:

1. Перейти в новое состояние.
2. Если новое состояние `UNKNOWN`, запустить lazy solve из него.
3. Вывести текущее состояние и current value.

---

## 11. API между solver и play-утилитой

Нужно вынести общий oracle API.

Пример:

```cpp
enum class PlayerResult {
    P1,
    P2,
    Unknown
};

struct MoveEvaluation {
    std::string move_label;
    PlayerResult result;
    std::vector<int> refuting_dirs;
    std::vector<int> unknown_dirs;
};

class SolverOracle {
public:
    PlayerResult evaluateP1ToMoveState(const State& state);
    PlayerResult solveP1ToMoveState(const State& state, SolveLimits limits);

    PlayerResult evaluateP2ToBlowState(const State& after_plant_state);
    PlayerResult solveP2ToBlowState(const State& after_plant_state, SolveLimits limits);

    std::vector<MoveEvaluation> listP1Moves(const State& state, bool lazy_solve_missing = false);
    std::vector<MoveEvaluation> listP2Moves(const State& after_plant_state, bool lazy_solve_missing = false);
};
```

`evaluate...` не должен запускать тяжёлый расчёт, только смотреть memo и возвращать `UNKNOWN`, если данных нет.

`solve...` может запускать solver.

---

## 12. Тестирование

### 12.1. Unit-тесты packed key

Проверить:

- упаковку/распаковку key64;
- упаковку/распаковку key80;
- отсутствие потери битов для `6 × 6`;
- разные состояния дают разные packed keys на наборе тестов;
- canonical state key после упаковки корректно ищется в memo.

### 12.2. Unit-тесты meta

Проверить:

- `meta == 0` означает empty;
- value `true/false` корректно кодируется и декодируется;
- fingerprint никогда не равен 0 для occupied bucket;
- совпадение fingerprint не заменяет полное сравнение ключа.

### 12.3. Unit-тесты MemoTable

Проверить:

- insert/find для key64;
- insert/find для key80;
- коллизии;
- rehash;
- load factor;
- отсутствие erase;
- корректность при большом количестве вставок;
- результат совпадает с baseline `std::unordered_map` на случайном наборе ключей.

### 12.4. Тесты сериализации `.dsmem`

Проверить:

- save/load key64;
- save/load key80;
- header validation;
- отказ загрузки при несовместимом `n/k`;
- отказ загрузки при неправильном magic;
- отказ загрузки при неправильном размере файла;
- сохранение и загрузку partial базы `complete = 0`.

### 12.5. Solver regression tests

Результаты solver-а должны совпадать с этапом 1 на известных случаях:

```text
1×1,k=1 -> P1
2×2,k=4 -> P1
3×3,k=5 -> P2
3×3,k=6 -> P1
4×4,k=6 -> P2
4×4,k=7 -> P1
5×5,k=6 -> P2
5×5,k=7 -> P1
6×6,k=6 -> P2
```

Для тяжёлого `6×6,k=6` тест можно не включать в обычный unit-test suite, но оставить как manual/performance regression.

### 12.6. Play-утилита smoke tests

Проверить:

- запуск без memo-файла создаёт новую базу;
- запуск с memo-файлом загружает базу;
- `state` показывает ASCII-поле;
- `moves` показывает список ходов;
- `play` меняет фазу;
- `back` возвращает назад;
- `save` создаёт `.dsmem`;
- при заходе в UNKNOWN запускается lazy solve.

---

## 13. Benchmark и критерии производительности

После замены memo-таблицы нужно сравнить с baseline этапа 1.

Обязательные benchmark cases:

```text
5×5,k=6
5×5,k=7
6×6,k=5
6×6,k=6
```

Сравнить:

- elapsed time;
- recursive calls;
- unique states;
- memo hits;
- peak memory / estimated memory;
- размер сохранённого `.dsmem` файла.

Ожидаемое поведение:

- results должны совпадать с baseline;
- memory usage должен заметно снизиться;
- возможное небольшое изменение времени допустимо;
- если время выросло сильно, нужно профилировать probing/load factor/hash.

---

## 14. Обновление структуры проекта

Добавить новые файлы примерно так:

```text
dandelions-solver/
  src/
    memo_key.h
    memo_key.cpp
    memo_table.h
    memo_table.cpp
    memo_file.h
    memo_file.cpp
    oracle.h
    oracle.cpp
    play_main.cpp
    ascii_render.h
    ascii_render.cpp
  tests/
    test_memo_key.cpp
    test_memo_table.cpp
    test_memo_file.cpp
    test_oracle.cpp
```

Обновить `CMakeLists.txt`, чтобы собирались:

```text
dandelions_core.lib
dandelions_solver.exe
dandelions_play.exe
dandelions_tests.exe
```

---

## 15. Критерии приёмки этапа

Этап считается выполненным, если:

1. Основной solver больше не использует `std::unordered_map` для основной memo-таблицы.
2. Для `n <= 5` используется packed key64.
3. Для `n == 6` используется packed key80.
4. Memo-таблица реализована как insert-only SoA open-addressing table.
5. `value` и `control/fingerprint` объединены в `uint8_t meta`.
6. Реализованы save/load `.dsmem` в формате raw table snapshot.
7. `.dsmem` header содержит параметры совместимости и проверяется при загрузке.
8. Добавлены `--save-memo` и `--load-memo` в `dandelions_solver.exe`.
9. Добавлены лимиты `--max-states`, `--time-limit-sec`, `--progress-every-sec`.
10. Добавлен бинарник `dandelions_play.exe`.
11. `dandelions_play.exe` показывает ASCII-поле.
12. `dandelions_play.exe` показывает current value текущего состояния.
13. `dandelions_play.exe` показывает список ходов с результатами `P1/P2/UNKNOWN`.
14. Для проигрышных посадок P1 выводятся refuting winds.
15. При заходе в UNKNOWN состояние запускается lazy solve.
16. После lazy solve memo обновляется и может быть сохранён.
17. Реализована команда `back`.
18. `.gitignore` игнорирует `*.dsmem` и папки memo/cache.
19. Unit-тесты проходят.
20. Результаты известных solver cases совпадают с результатами этапа 1.
21. Есть benchmark-сравнение памяти/времени до и после оптимизации хотя бы на `5×5,k=6`, `5×5,k=7`, `6×6,k=5`.

---

## 16. Что не нужно делать на этом этапе

Не требуется:

- GUI;
- web-интерфейс;
- GPU;
- многопоточность;
- полное заранее построенное дерево стратегии;
- поддержка `n > 6` в optimized memo;
- сжатие `.dsmem` через zip/zstd;
- переносимость `.dsmem` между big-endian/little-endian архитектурами;
- хранение best_move/refuting_move внутри основной memo-таблицы.

Главный фокус этапа:

```text
компактная memo-таблица + сохранение/загрузка базы + интерактивный lazy oracle для анализа стратегий
```

