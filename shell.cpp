#include <iostream>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

using namespace std;

#define sz(X) ((int)((X).size()))
#define pb push_back

struct command
{
	vector<string> parameters;
	string in;
	string out;

	command(){reset();}

	void reset()
	{
		parameters.clear();
		in = "";
		out = "";
	}
};

struct Shell
{
	vector<string> parse;
	vector<command> Commands;
	vector<int> Wait;
	bool background;
	char *argv[525];
	char argv_tmp[525][35];
	char path[525];

	Shell(){}

	void print(string s)
	{
		printf("%s", s.c_str());
	}

	bool get_command()
	{
		char tmp[525];
		if (!gets(tmp)) return false;
		int len = strlen(tmp);
		tmp[len] = ' ';
		++len;
		parse.clear();
		string tmp2 = "";
		for (int i = 0; i < len; ++i)
		{
			if (tmp[i] == ' ' && tmp2 != "")
			{
				parse.pb(tmp2);
				tmp2 = "";
			}
			if (tmp[i] != ' ')
				tmp2 += tmp[i];
		}
		return true;
	}

	bool cd(vector<string> s)
	{
		getcwd(path, 525);
		string target;
		if (sz(s) == 1)
			target = getenv("HOME");
		else if (sz(s) == 2)
			target = s[1];
		else return false;
		return chdir(target.c_str()) != -1;
	}

	bool analysis()
	{
		background = false;
		if (sz(parse) && parse.back() == "&")
		{
			background = true;
			parse.pop_back();
		}

		if (sz(parse) && parse.back() == "|")
		{
			print("command error: no correct token after \'|\'\n");
			return false;
		}
		parse.pb("|");
		Commands.clear();
		if (sz(parse) == 1) return true;
		command tmp;
		for (int i = 0; i < sz(parse); ++i)
		{
			if (parse[i] == "|")
			{
				Commands.pb(tmp);
				tmp.reset();
				continue;
			}
			if (parse[i] == "<" || parse[i] == ">")
			{
				if (i + 1 < sz(parse) && parse[i + 1] != "|" && parse[i + 1] != "<" && parse[i + 1] != ">")
				{
					if (parse[i] == "<")
						tmp.in = parse[i + 1];
					else
						tmp.out = parse[i + 1];
					++i;
				}
				else
				{
					print("command error: no correct token after \'" + parse[i] + "\'\n");
					return false;
				}
				continue;
			}
			tmp.parameters.pb(parse[i]);
		}

		return true;
	}

	int work()
	{
		if (sz(Commands) == 1 && Commands[0].parameters[0] == "cd")
		{
			if (!cd(Commands[0].parameters)){print("Command error: cd\n");return -1;}
			return 1;
		}
		pid_t tmp = fork();
		if (tmp < 0){print("command error: fork\n"); return -1;}
		if (tmp == 0)
		{
			if (Commands[0].in != "")
			{
				int id = open(Commands[0].in.c_str(), (O_RDWR | O_CREAT), 0644);
				if (id == -1){print("command error: file\n");return tmp;}
				dup2(id, 0);
			}
			for (int i = 0; i < sz(Commands); ++i)
			{
				int Pipe[2];
				if (pipe(Pipe) != 0){print("command error: pipe\n"); return tmp;}
				pid_t pid = fork();
				if (pid < 0){print("command error: fork\n");return tmp;}
				if (pid == 0)
				{
					if (i + 1 < sz(Commands) || Commands[i].out != "")
						close(1);
					if (i + 1 < sz(Commands))
						dup2(Pipe[1], 1);
					if (Commands[i].out != "")
					{
						int id = open(Commands[i].out.c_str(), (O_RDWR | O_CREAT), 0644);
						if (id == -1){print("command error: file\n");return tmp;}
						dup2(id, 1);
					}
					close(Pipe[0]);
					if (Commands[i].parameters[0] == "cd")
					{
						if (!cd(Commands[i].parameters)){print("Command error: cd\n");return tmp;}
						return tmp;
					}
					for (int j = 0; j < sz(Commands[i].parameters); ++j)
					{
						for (int k = 0; k < sz(Commands[i].parameters[j]); ++k)
							argv_tmp[j][k] = Commands[i].parameters[j][k];
						argv_tmp[j][sz(Commands[i].parameters[j])] = '\0';
						argv[j] = argv_tmp[j];
					}
					argv[sz(Commands[i].parameters)] = NULL;
					if (execvp(argv[0], argv) == -1){print("Command error: command\n");return tmp;}
					return tmp;
				}
				else
				{
					if (wait(NULL) == -1){print("command error: wait\n");return tmp;}
					close(0);
					dup2(Pipe[0], 0);
					close(Pipe[1]);
				}
			}
		}
		else
		{
			sleep(1);
			if (background)
				Wait.pb(tmp);
			else if (wait(NULL) == -1){print("command error: wait\n");return tmp;}
		}
		return tmp;
	}

	void kill_background()
	{
		for (int i = 0; i < sz(Wait); ++i)
			kill(Wait[i], 0);
	}
};

int main()
{
	Shell *shell = new Shell();
	while (1)
	{
		shell->print("shell: ");
		if (!shell->get_command())
		{
			shell->kill_background();
			break;
		}
		if (!shell->analysis()) continue;
		if (shell->work() == 0) return 0;
	}
	delete shell;
	return 0;
}
