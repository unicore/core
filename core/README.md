# CORE

UNICORE - универсальный протокол создания цифровых экономических систем.

Протокол UNICORE предлагает инструменты создания цифровых экономических систем и инновационный механизм перераспределения игровых жетонов в них на основе математического алгоритма точного финансового планирования "Двойная Спираль".
 
  - Исполняется в любой сетевой операционной системе типа EOS.
  - Генерирует теоретически неограниченную прибыль при фиксированных рисках и абсолютном финансовом балансе в любой момент.
  - Предоставляет инновационную бизнес-модель, основанную на энергии внимания и добровольных пожертвованиях людей.

### Назначение
Протокол предназначен для запуска и обслуживания цифровых экономических систем "Двойная Спираль" во множестве различных конфигураций с целью увеличения качества жизни участников и реализации новых глобальных проектов.

### Компиляция
Версия eosio_cdt: 1.7.0

Заменить ABSOLUTE_PATH_TO_CONTRACT на абсолютный путь к директории контракта core. 

```
docker run --rm --name eosio.cdt_v1.7.0 --volume /ABSOLUTE_PATH_TO_CONTRACT:/project -w /project eostudio/eosio.cdt:v1.7.0 /bin/bash -c "eosio-cpp -abigen -I include -R include -contract unicore -o core.wasm main.cpp" &
```

### Установка
```
cleos set contract CONTRACT_NAME /ABSOLUTE_PATH_TO_CONTRACT/core -p CONTRACT_NAME
```
<!-- 
### Инициализация
Для инициализации необходимо совершить транзакцию с системным процентом, изымаемым из оборота всех Двойных Спиралей. (1000000 = 100%). Максимальный процент: 30%.

```
cleos push action CONTRACT_NAME init [0] -p CONTRACT_NAME
```

Изъятые из оборота средства распределяются среди делегатов сети. 
 -->

### Порядок создания и запуска хоста
1. Произвести апгрейд аккаунта методом upgrade.
2. Установить параметры хоста методом setparams.
3. Произвести оплату стоимости запуска хоста методом transfer с кодом в мемо: "110-имя_аккаунта_хоста" (операция перевода принимается к оплате только от имени хоста).
4. Вызвать метод start.

После запуска, хост готов оперировать токенами согласно настройки своей цифровой экономики. 

### Лицензия
MIT

UNICORE is released under the open source MIT license and is offered “AS IS” without warranty of any kind, express or implied. Any security provided by the UNICORE software depends in part on how it is used, configured, and deployed. UNICORE is built upon many third-party libraries such as EOSIO (MIT Licence), WABT (Apache License) and WAVM (BSD 3-clause) which are also provided “AS IS” without warranty of any kind. Without limiting the generality of the foregoing, UNICODE makes no representation or guarantee that UNICORE or any third-party libraries will perform as intended or will be free of errors, bugs or faulty code. Both may fail in large or small ways that could completely or partially limit functionality or compromise computer systems. If you use or implement UNICORE, you do so at your own risk. In no event will UNICODE be liable to any party for any damages whatsoever, even if it had been advised of the possibility of damage.
