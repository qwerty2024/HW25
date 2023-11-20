# HW25

Клиент-серверный чат + база данных mysql.

Перед запуском необходимо создать базу данных с названием chatdb.
Таблицы создаются автоматически.

Перед компиляцией нужно заменить в вызове mysql_real_connect() логин и пароль для подключения к базе (вызов в файле server.cpp в функции main()).

Для сборки клиента: make client 

Для сборки сервера: make server

Демо ролик с тестом. https://youtu.be/K6ZJtQkCrsc

Возможные проблемы:
1. При компиляции не находит файл mysql.h.
   Решение: установить - sudo apt-get install libmysqlclient-dev
2. Ошибка подключения к базе данных.Решение: Настроить текущего пользователя для подключения:
   
      sudo mysql -u root # I had to use "sudo" since it was a new installation
      
      mysql> USE mysql;
   
      mysql> CREATE USER 'YOUR_SYSTEM_USER'@'localhost' IDENTIFIED BY 'YOUR_PASSWD';
   
      mysql> GRANT ALL PRIVILEGES ON *.* TO 'YOUR_SYSTEM_USER'@'localhost';
   
      mysql> UPDATE user SET plugin='auth_socket' WHERE User='YOUR_SYSTEM_USER';
   
      mysql> FLUSH PRIVILEGES;
   
      mysql> exit;

      sudo service mysql restart
