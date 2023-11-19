#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <map>
#include <chrono>
#include <ctime>
#include <mysql/mysql.h>

#include "user.h"

using namespace std;

#define MAX_LEN_LOGIN 50
#define MESSAGE_LENGHT 1024
#define PORT 7777

char login[MAX_LEN_LOGIN];
char buffer[MESSAGE_LENGHT]; 
char message[MESSAGE_LENGHT];
int socket_file_descriptor, message_size, connection_status, connection;
socklen_t length;
struct sockaddr_in serveraddress, client;

string current_login;
bool go_exit = false;

MYSQL mysql;
MYSQL_RES* res;
MYSQL_ROW row;


enum string_code
{
	test, reg, auth, all, privat, show, users, clean,  endd, null
};

string_code command_code(string const& str)
{
	if (str == "@test") return test;
	if (str == "@reg") return reg;
	if (str == "@auth") return auth;
	if (str == "@all") return all;
	if (str == "@private") return privat;
	if (str == "@show") return show;
	if (str == "@users") return users;
	if (str == "@clean") return clean;
	if (str == "@end") return endd;

return null;
}

void print_data()
{
	auto time_tmp = std::chrono::system_clock::now();
	std::time_t time = std::chrono::system_clock::to_time_t(time_tmp);
	cout << endl << std::ctime(&time);
}

void parse_str(string &str, int n, string &mess) // достаем n-ое слово из строки (разделитель пробел)
{
	int count = 0;
	for (int i = 0; i < mess.size(); ++i)
	{
		if (count == n && mess[i] != ' ')
		{
			str += mess[i];
			continue;
		}

		if (mess[i] == ' ')
			count++;

		if (count > n) break;
		if (mess[i] == '\0') break;
	}
}

string give_message(string login)
{
	string to_database = "SELECT Sender,Message FROM Messages WHERE Recver = 'ALL' OR Recver = '" + login + "'";
	mysql_query(&mysql, to_database.c_str());

	string tmp = "";
	if (res = mysql_store_result(&mysql))
	{
		while (row = mysql_fetch_row(res))
		{
			tmp = "$" + string(row[0]) + string(row[1]) + tmp;
		}
	}

	return tmp;
}

void show_databases()
{
	mysql_query(&mysql, "SELECT * FROM Users");

	cout << "\nUser database (name, login, password): \n=================\n";

	if (res = mysql_store_result(&mysql))
	{
		while (row = mysql_fetch_row(res))
		{
			for (int i = 0; i < mysql_num_fields(res); ++i)
			{
				cout << row[i] << " ";
			}
			cout << endl;
		}
	}

	cout << "=================\n\nMessages database (№, Sender, Recver, Messsage):\n=================\n";
	
	mysql_query(&mysql, "SELECT * FROM Messages");

	if (res = mysql_store_result(&mysql))
	{
		while (row = mysql_fetch_row(res))
		{
			for (int i = 0; i < mysql_num_fields(res); ++i)
			{
				cout << row[i] << " ";
			}
			cout << endl;
		}
	}

	cout << "=================\n" << endl;
}

void req()
{
	bzero(message, MESSAGE_LENGHT);
	//memset(message, '\0', MESSAGE_LENGHT);
	//read(connection, message, sizeof(message));
	recv(connection, message, MESSAGE_LENGHT, 0);
	print_data();
	if (strcmp(message, "") != 0) cout << "Data: " << message << endl;

	string recv_message = message;
	string temp = "";
	parse_str(temp, 0, recv_message);

	bzero(message, MESSAGE_LENGHT); // обнуляем сообщение для возврата клиенту
	string tmp_message = "";

	switch(command_code(temp))
	{
	case test:
		{
			tmp_message = "OK!"; // клиенту вернем ОК, так как получили от него сообщение
			break;
		}

	case reg:
		{
			string name; 
			parse_str(name, 1, recv_message);
			string login;
			parse_str(login, 2, recv_message);
			string pass;
			parse_str(pass, 3, recv_message);

			string to_database = "SELECT login FROM Users WHERE login = '" + login + "';";
			mysql_query(&mysql, to_database.c_str());
			if (res = mysql_store_result(&mysql))
			{
				while (row = mysql_fetch_row(res))
				{
					for (int i = 0; i < mysql_num_fields(res); ++i)
					{
						cout << row[i] << " ";
					}
					cout << endl;
				}
			}
		
			unsigned long long count = mysql_num_rows(res);
			if (count == 1)
			{
				cout << "ERROR. Login is busy!" << endl;
				tmp_message = "@error ";
			}else
			{
				cout << "SAVE. Good login!" << endl;
				tmp_message = "@success ";
				to_database = "INSERT INTO Users(login,name,pass) VALUES('" + login + "','" + name + "','" + pass + "')";
				mysql_query(&mysql, to_database.c_str());
			}

			break;
		}

	case auth:
		{
			User temp_user;

			string login;
			parse_str(login, 1, recv_message);
			string pass;
			parse_str(pass, 2, recv_message);

			string to_database = "SELECT pass,name FROM Users WHERE login = '" + login + "';";
			mysql_query(&mysql, to_database.c_str());
			
			string tmp_pass = "";
			string tmp_name = "";
			if (res = mysql_store_result(&mysql))
			{
				while (row = mysql_fetch_row(res))
				{
					for (int i = 0; i < mysql_num_fields(res); ++i)
					{
						tmp_pass = row[0];
						tmp_name = row[1];
					}
				}
			}

			unsigned long long count = mysql_num_rows(res);
			if (count == 1)
			{
				cout << "Login exists!" << endl;
				if (tmp_pass == pass)
				{
					tmp_message = tmp_name;
					current_login = login;
				}else
				{
					tmp_message = "@error: Incorrect password";
				}
			}else
			{
				cout << "ERROR. No this user!" << endl;
				tmp_message = "@error ";
			}
			break;
		}

	case all:
		{
			string msg;
			parse_str(msg, 1, recv_message);
			tmp_message = "OK!";

			string to_database = "INSERT INTO Messages(Sender,Recver,Message) VALUES('" + current_login + "','ALL','" + msg + "')";
			mysql_query(&mysql, to_database.c_str());

			break;
		}

	case privat:
		{
			string login;
			parse_str(login, 1, recv_message);
			string msg;
			parse_str(msg, 2, recv_message);

			tmp_message = "OK!";

			string to_database = "INSERT INTO Messages(Sender,Recver,Message) VALUES('" + current_login + "','" + login  + "','" + msg  + "')";
			mysql_query(&mysql, to_database.c_str());

			break;
		}

	case users:
		{
			cout << "Send users!" << endl;
			mysql_query(&mysql, "SELECT login FROM Users");

			if (res = mysql_store_result(&mysql))
			{
				int count = 0;
				while (row = mysql_fetch_row(res))
				{
					for (int i = 0; i < mysql_num_fields(res); ++i)
					{
						if (count != 0) // убираем системного юзера ALL
							tmp_message = tmp_message + row[i] + " ";
					}
					count++;
				}
			}

			break;
		}

	case clean:
		{
			cout << "Clean databases!" << endl;
			mysql_query(&mysql, "DELETE FROM Users WHERE login != 'ALL';");
			mysql_query(&mysql, "DELETE FROM Messages;");
			tmp_message = "OK!";
			break;
		}


	case endd:
		{
			tmp_message = "OK!";
			go_exit = true;
			break;
		}

	case show:
		{
			string login = current_login;
			string msg = give_message(login);

			if (msg == "")
			{
				tmp_message = "@nomsg"; // временно
			}else
			{
				tmp_message = msg;
			}
			break;
		}

	case null:
		{
			break;
		}
	}


	strcpy(message, tmp_message.c_str());
//	ssize_t bytes = write(connection, message, sizeof(message));
	ssize_t bytes = send(connection, message, strlen(message), 0);
}


int main(int argc, char** argv)
{
	system("clear");

	mysql_init(&mysql);
	if (&mysql == nullptr)
	{
		cout << "Error: can't create MySQL-descriptor" << endl;
	}

	if (!mysql_real_connect(&mysql, "localhost", "user", "qwerty123", "chatdb", 3306, NULL, 0))
	{
		cout << "Error: can't connect to database -> " << mysql_error(&mysql) << endl;
	} else cout << "Success connectet database!" << endl;

	// таблица с сообщениями
	mysql_query(&mysql, "CREATE TABLE Messages(id INT AUTO_INCREMENT PRIMARY KEY, Sender VARCHAR(255) NOT NULL, Recver VARCHAR(255) NOT NULL, Message TEXT)");
	// таблица с пользователями
	mysql_query(&mysql, "CREATE TABLE Users(login VARCHAR(255) NOT NULL PRIMARY KEY, name VARCHAR(255) NOT NULL, pass VARCHAR(255) NOT NULL)");
	// дефолтный пользователь для отправки сообщений всем
	mysql_query(&mysql, "INSERT INTO Users(login,name,pass) VALUES('ALL','ALL','ALL')");

	if (argc == 2)
		if (strcmp(argv[1], "show_databases") == 0)
			show_databases();

	cout << "SERVER IS LISTENING THROUGH THE PORT: " << PORT << endl;

	socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
	serveraddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddress.sin_port = htons(PORT);
	serveraddress.sin_family = AF_INET;
	bind(socket_file_descriptor, (struct sockaddr*)&serveraddress, sizeof(serveraddress));

	connection_status = listen(socket_file_descriptor, 1);
	length = sizeof(client);
	connection = accept(socket_file_descriptor, (struct sockaddr*)&client, &length);


	while(!go_exit)
		req();

	mysql_close(&mysql);
	close(socket_file_descriptor);
	return 0;
}