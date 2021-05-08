# Gor-routines

Однажды с помощью stackful корутин мы реализовали кооперативную многозадачность в виде файберов (легковесных потоков).

Теперь мы сделаем то же самое для stackless корутин из С++20. То, что у нас получится, мы (не всерьез) назовем _gor-routines_ в честь одного из авторов Coroutines TS в С++ – [Гора Нишанова](https://www.youtube.com/watch?v=_fu0gx-xseY).

## Gor-routines in Action

```cpp
gorr::StaticThreadPool scheduler{/*threads=*/4};
gorr::Mutex mutex;

gorr::JoinHandle GorRoutine() {
  // Перепланируемся в пул потоков
  co_await scheduler.Schedule();

  for (size_t i = 0; i < 100; ++i) {
    if (i % 7 == 0) {
      // Уступаем поток пула другой горрутине
      co_await gorr::Yield();
    }

    {
      auto guard = co_await mutex.Lock();
      // Мы в критической секции!
    }
  }
  
  co_return;
}

int main() {
  for (size_t i = 0; i < 100; ++i) {
    // Запускаем горрутину
    GorRoutine();
  }
  
  // Дожидаемся завершения всех горрутин
  scheduler.Join();
  
  return 0;
}

```

## Планировщик

В отличие от обычного пула потоков, `gorr::StaticThreadPool` планирует не задачи, а корутины: 

```cpp
// Телепортируемся в пул потоков
co_await pool.Schedule();
```

К пулу (планировщику) прилагается операция `gorr::Yield`:

```cpp
// Перепланировать текущую горрутину
// в конец очереди планировщика
co_await gorr::Yield();
```

### Реализация

Пул не должен выполнять динамических аллокаций при планировании! 

За основу возьмите уже написанный ранее пул потоков. 

Используйте интрузивную очередь, узлы для нее аллоцируйте на фреймах корутин (через awaiter, который пул возвращает в вызове `Schedule`).

## `Mutex`

Операция `Lock` у `gorr::Mutex` – асинхронная:

```cpp
{
  auto guard = co_await mutex.Lock();
  // Критическая секция
}
```

### Реализация

Реализация мьютекса не должна выполнять динамические аллокации.

В реализации мьютекса вы больше не сможете написать цикл `while`.

Вам потребуется вариант `await_suspend`, который возвращает `bool`, а не `void`:
- `false` означает, что корутина передумала останавливаться, она сразу выполнит `await_resume` и продолжит исполнение
- `true` – что awaiter запланировал возобновление корутины, и она должна остановиться

В методе `await_ready` для awaiter-а мьютекса поместите быстрый захват мьютекса.

В качестве задачи под звездочкой подумайте над реализацией, которая не будет использовать взаимное исключение на уровне _потоков_.

## `JoinHandle`

Горрутина при запуске дает пользователю `gorr::JoinHandle`, с помощью которого тот может дождаться завершения горрутины:   

```cpp

gorr::StaticThreadPool scheduler{4};

gorr::JoinHandle GorRoutine() {
  co_await scheduler.Schedule();
  
  // Тут горутина делает что-то полезное
  co_return;
}

int main() {
  // Стартуем горрутину
  auto h = GorRoutine();

  // Синхронно дожидаемся завершения горрутины в пуле
  gorr::Join(h);

  // ...
}
```

### Реализация 

В задаче дана заглушка, [опционально] можно заменить ее на решение задачи `Task`.

## References

- [Asymmetric Transfer](https://lewissbaker.github.io/)
- [cppreference / Coroutines](https://en.cppreference.com/w/cpp/language/coroutines)
- [dcl.fct.def.coroutine](https://eel.is/c++draft/dcl.fct.def.coroutine), [expr.await](https://eel.is/c++draft/expr.await#:co_await)
