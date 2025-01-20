// INCLUDES
//{{{
#include <ncurses.h>
#include <vector>
#include <string>
#include <fstream>
#include <stdlib.h>
#include <unistd.h>
#include <functional>
#include <map>
#include <sstream>
//}}}

// CONST VARIABLES
//{{{
const unsigned short TAB_SIZE = 4;
unsigned short h, w;
//}}}

// FUNCTIONS
//{{{
inline void insert();
inline void handle_command(std::string command);
inline void command();
inline void moveLeft();
inline void moveDown();
inline void moveUp();
inline void moveRight();
inline void clearline();
inline void scrrend();
inline void saveF(std::string file);
inline void loadF(std::string file);
inline void insertBefore();
inline void insertAfter();
//}}}

// STRUCTS
//{{{

//buffer
//{{{
class buffer {
public:
	std::string name;
	std::vector <std::string> content;
	std::string path;
	unsigned x = 0, y = 0, toprend = 0;
	buffer() {
		this -> name = "";
		this -> content = {""};
	}
	buffer(std::string title, std::vector <std::string> cont) {
		this -> name = title;
		for (auto& line : cont) {
			this -> content.push_back(line);
		}
	}
};
//}}}

// register
//{{{
struct reg {
	std::vector<std::string> content;
	void copy(buffer& copyfrom, unsigned startL, unsigned endL) {
		content.clear();
		for (; startL <= endL; ++startL) {
			content.push_back(copyfrom.content[startL]); // Access content of buffer
		}
	}
	void paste(buffer& pasteTo) {
		for (const auto& line : content) {
			pasteTo.content.insert(pasteTo.content.begin() + pasteTo.y, line);
			pasteTo.y++;
		}
	}
};
//}}}

// keybind
//{{{
class keybind {
public:
	using CommandFunction = std::function<void()>;

	void bind(const std::string& keySequence, CommandFunction command) {
		bindings[keySequence] = command;
	}

	bool execute(const std::string& keySequence) {
		auto it = bindings.find(keySequence);
		if (it != bindings.end()) {
			it -> second();
			return true;
		}
		return false;
	}

	void loadBindingsFromFile(const std::string& filename) {
		std::ifstream file(filename);
		if (!file.is_open()) {
			move(h - 1, 0);
			printw("Bind file not found");
			return;
		}

		std::string line;
		while (std::getline(file, line)) {
			parseLine(line);
		}
		file.close();
	}

	void bindDefaultActions() {
		bind("h", moveLeft);
		bind("j", moveDown);
		bind("k", moveUp);
		bind("l", moveRight);
		bind(":", command);
		bind("i", insertBefore);
		bind("a", insertAfter);
	}

private:
	std::map<std::string, CommandFunction> bindings;

	void parseLine(const std::string& line) {
		std::istringstream iss(line);
		std::string command, keySequence;

		if (iss >> command && command == "bind") {
			std::getline(iss, keySequence, '"');
			std::getline(iss, keySequence, '"');
			std::string funcCall;
			std::getline(iss, funcCall);

			funcCall.erase(0, funcCall.find_first_not_of(" \t"));

			if (funcCall.back() == ')') {
				bind(keySequence, [funcCall]() {
				});
			}
			else {
				bind(keySequence, [funcCall]() {
				});
			}
		}
	}
};
//}}}

//}}}

// GLOBAL VARIABLES
//{{{
std::vector<buffer> buffers;
reg registers[26]; // from a to z
reg clipboard;
int cb = 0; // current buffer index
chtype c;
std::string keyseq;
//}}}

// MAIN
//{{{
int main() {
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	getmaxyx(stdscr, h, w);
	
	buffers.push_back(buffer("0", {""}));
	keybind kb;
	kb.bindDefaultActions();
	kb.loadBindingsFromFile("bind");
	while (1) {
		scrrend();
		c = getch();
		keyseq.push_back(c);
		if (kb.execute(keyseq) || 
			keyseq.size() > 1 && kb.execute(keyseq.substr(1)) || 
			keyseq.size() > 2 && kb.execute(keyseq.substr(2)) || 
			keyseq.size() == 3) {
			keyseq.clear();
		}
	}
	scrrend();
	endwin();
	return 1;
}
//}}}

// Clear the current line
//{{{
inline void clearline() {
	for (unsigned short i = 0; i < w; ++i) {
		addch(' ');
	}
}
//}}}

// Render the screen
//{{{
inline void scrrend() {
	if (buffers[cb].toprend > buffers[cb].y) {
		buffers[cb].toprend = buffers[cb].y;
	}
	else if (buffers[cb].toprend + h - 2 < buffers[cb].y) {
		buffers[cb].toprend = buffers[cb].y - (h - 2); // Adjust toprend if the cursor is below the visible area
	}

	for (unsigned short i = 0; i < h - 1; ++i) {
		move(i, 0);
		clearline();
		move(i, 0);
		if (i + buffers[cb].toprend >= buffers[cb].content.size()) {
			addch('~'); // Indicate empty lines
		}
		else {
			printw(buffers[cb].content[i + buffers[cb].toprend].c_str());
		}
	}

	// Move the cursor to the correct position
	move(buffers[cb].y - buffers[cb].toprend, buffers[cb].x);
}
//}}}

// Save the current buffer to a file
//{{{
inline void saveF(std::string file) {
	std::ofstream ofs(buffers[cb].path);
	for (const auto& line : buffers[cb].content) {
		ofs << line << '\n';
	}
	ofs.close();
}
//}}}

// Load text from a file to the current buffer
//{{{
inline void loadF(std::string file) {
	std::ifstream ifs(file);
	std::string wd = std::string(getcwd(nullptr, 0));
	buffers[cb].path = wd + "/" + file;
	if (!ifs) {
		buffers.push_back({file, {""}});
		cb = buffers.size() - 1;
		move(h - 1, 0);
		clearline();
		move(h - 1, 0);
		printw(file.c_str());
		printw(" [new]");
		return;
	}
	
	buffers[cb].content.clear();
	std::string line;
	
	while (std::getline(ifs, line)) {
		std::string newLine;
		for (char ch : line) {
			if (ch == '\t') {
				newLine.append(TAB_SIZE, ' ');
			}
			else {
				newLine.push_back(ch);
			}
		}
		buffers[cb].content.push_back(newLine);
	}
	
	ifs.close();
	buffers[cb].y = 0;
	buffers[cb].x = 0;
	buffers[cb].toprend = 0;
	move(h - 1, 0);
	clearline();
	move(h - 1, 0);
	printw(file.c_str());
}
//}}}

// INSERT
//{{{
inline void insert() {
	move(h - 1, 0);
	clearline();
	move(h - 1, 0);
	printw("-- INSERT --");
	scrrend();
	while (1) {
		c = getch();
		// New line
		if (c == '\n') {
			if (buffers[cb].x == buffers[cb].content[buffers[cb].y].size()) {
				buffers[cb].content.insert(buffers[cb].content.begin() + buffers[cb].y + 1, "");
			}
			else {
				std::string s = buffers[cb].content[buffers[cb].y].substr(buffers[cb].x);
				buffers[cb].content[buffers[cb].y].erase(buffers[cb].x);
				buffers[cb].content.insert(buffers[cb].content.begin() + buffers[cb].y + 1, s);
			}
			buffers[cb].y++;
			buffers[cb].x = 0; // Reset x to the beginning of the new line
		}
		// Backspace
		else if (c == KEY_BACKSPACE) {
			if (buffers[cb].x > 0) {
				buffers[cb].x--;
				buffers[cb].content[buffers[cb].y].erase(buffers[cb].x, 1);
			}
		}
		// Printable characters
		else if (31 < c && c < 127) {
			if (buffers[cb].x < buffers[cb].content[buffers[cb].y].size()) {
				buffers[cb].content[buffers[cb].y].insert(buffers[cb].x, 1, c);
			}
			else {
				buffers[cb].content[buffers[cb].y].push_back(c);
			}
			buffers[cb].x++;
		}
		// Handle tab key
		else if (c == '\t') {
			for (int i = 0; i < TAB_SIZE; ++i) {
				if (buffers[cb].x < buffers[cb].content[buffers[cb].y].size()) {
					buffers[cb].content[buffers[cb].y].insert(buffers[cb].x, 1, ' ');
				}
				else {
					buffers[cb].content[buffers[cb].y].push_back(' ');
				}
				buffers[cb].x++;
			}
		}
		else if (c == 27 || c == 0) {
			break;
		}
		scrrend();
	}
	move(h - 1, 0);
	clearline();
}
//}}}

// HANDLE_COMMAND
//{{{
inline void handle_command(std::string command) {
	if (command == "q") {
		endwin();
		exit(0);
	}
	else if (command == "w") {
		saveF(buffers[cb].name);
	}
	else if (command[0] == 'c' && command[1] == 'd' && command[2] == ' ') {
		chdir(command.substr(3).c_str());
	}
	else if (command == "pwd") {
		move(h - 1, 0);
		clearline();
		move(h - 1, 0);
		char* pwd = new char[256];
		getcwd(pwd, 256);
		printw(pwd);
	}
	else if (command == "buffers") {
		move(h - 1, 0);
		clearline();
		move(h - 1, 0);
		for (auto& buf : buffers) {
			std::string title = buf.name + ' ';
			printw(title.c_str());
		}
	}
	else if (command[0] == 'e' && command[1] == ' ') {
		if (command.size() > 2) {
			loadF(command.substr(2));
		}
	}
	else if (command[0] == 'b' && command[1] == ' ') {
		std::string newbuf = command.substr(2);
		for (const auto& buf : buffers) {
			if (buf.name == newbuf) {
				cb = &buf - &buffers[0];
				return;
			}
		}
		buffers.push_back(buffer(newbuf, {""}));
		cb = buffers.size() - 1;
	}
	else if (command == "cb") {
		move(h - 1, 0);
		clearline();
		move(h - 1, 0);
		printw("%s %u", buffers[cb].name.c_str(), cb);
	}
	else if (command[0] == 'b' && command[1] == 'd' && command[2] == ' ') {
		std::string delbuf = command.substr(3);
	}
	else if (!command.empty()) {
		move(h - 1, 0);
		clearline();
		move(h - 1, 0);
		printw("Command not found");
	}
}
//}}}

// COMMAND
//{{{
inline void command() {
	std::string command = "";
	move(h - 1, 0);
	clearline();
	move(h - 1, 0);
	addch(':');
	unsigned short cx = 1;
	while (1) {
		c = getch();
		if (c == '\n') {
			move(buffers[cb].y - buffers[cb].toprend, buffers[cb].x);
			break;
		}
		else if (c == 27) { // Exit command mode
			move(h - 1, 0);
			clearline();
			move(buffers[cb].y - buffers[cb].toprend, buffers[cb].x);
			command.clear();
			break;
		}
		else if (c == KEY_BACKSPACE) { // Backspace
			if (cx > 1) {
				move(h - 1, --cx);
				addch(' ');
				move(h - 1, cx);
				command.pop_back();
			}
			else {
				move(h - 1, 0);
				clearline();
				break;
			}
		}
		else if (31 < c && c < 127) { // Printable characters
			command.push_back(c);
			if (cx < w) {
				addch(c);
			}
			cx++;
		}
	}
	handle_command(command);
}
//}}}

// CURSOR MOVEMENT
//{{{
// left
inline void moveLeft() {
	if (buffers[cb].x > 0) {
		buffers[cb].x--;
	}
}

// down
inline void moveDown() {
	if (buffers[cb].y < buffers[cb].content.size() - 1) {
		buffers[cb].y++;
		if (buffers[cb].x > buffers[cb].content[buffers[cb].y].size()) {
			buffers[cb].x = buffers[cb].content[buffers[cb].y].size(); // Adjust x if it exceeds the new line length
		}
	}
}

// up
inline void moveUp() {
	if (buffers[cb].y > 0) {
		buffers[cb].y--;
		if (buffers[cb].x > buffers[cb].content[buffers[cb].y].size()) {
			buffers[cb].x = buffers[cb].content[buffers[cb].y].size(); // Adjust x if it exceeds the new line length
		}
	}
}

// right
inline void moveRight() {
	if (buffers[cb].x < buffers[cb].content[buffers[cb].y].size()) {
		buffers[cb].x++;
	}
}

//}}}

// INSERT
//{{{
inline void insertBefore() {
	if (buffers[cb].x > 0) {
		buffers[cb].x--;
		insert();
	}
	else {
		insert();
	}
}

inline void insertAfter() {
	if (buffers[cb].x < buffers[cb].content[buffers[cb].y].size() - 2 && buffers[cb].content[buffers[cb].y].size() != 0) {
		buffers[cb].x++;
		insert();
		buffers[cb].x--;
	}
	else {
		insert();
	}
}
//}}}

