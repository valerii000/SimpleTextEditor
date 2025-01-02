#include <ncurses.h>
#include <vector>
#include <string>
#include <fstream>

const unsigned short TAB_SIZE = 4;

unsigned x = 0, y = 0, toprend = 0;
std::vector<std::string> text;
unsigned short h, w;
chtype c;

void clearline();
void scrrend();
void saveF(std::string file);
void loadF(std::string file);

int main() {
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE); // Enable special keys (like arrow keys)
	getmaxyx(stdscr, h, w);
	
	// Initialize the first line of text
	text.push_back("");

	while (1) {
		scrrend(); // Render the screen before getting input

		c = getch();
		
		if (c == 27) { // Command mode (ESC key)
			move(h - 1, 0);
			clearline();
			move(h - 1, 0);
			addch(':');
			std::string command = "";
			unsigned short cx = 1;
			while (1) {
				c = getch();
				if (c == '\n') {
					//move(h - 1, 0);
					//clearline();
					move(y - toprend, x);
					break;
				}
				else if (c == 27) { // Exit command mode
					move(h - 1, 0);
					clearline();
					move(y - toprend, x);
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
			// Command handling
			if (command == "q") {
				endwin();
				return 0;
			}
			else if (command[0] == 'w') {
				if (command.size() > 2) {
					saveF(command.substr(2));
				}
			}
			else if (command[0] == 'e') {
				if (command.size() > 2) {
					loadF(command.substr(2));
				}
			}
		}
		else if (c == '\n') { // New line
			if (x == text[y].size()) {
				text.insert(text.begin() + y + 1, ""); // Insert a new line
			}
			else {
				std::string s = text[y].substr(x);
				text[y].erase(x); // Remove the rest of the line
				text.insert(text.begin() + y + 1, s); // Insert the rest into a new line
			}
			y++; // Move to the next line
			x = 0; // Reset x to the beginning of the new line
		}
		else if (c == KEY_BACKSPACE) { // Backspace
			if (x > 0) {
				x--; // Move cursor left first
				text[y].erase(x, 1); // Remove the character at the new cursor position
			}
			else if (y > 0) {
				x = text[y - 1].size(); // Set cursor to the end of the previous line
				// Combine current line with the previous line
				text[y - 1] += text[y]; // Append current line to the previous line
				text.erase(text.begin() + y); // Remove the current line
				y--; // Move to the previous line
			}
		}
		else if (31 < c && c < 127) { // Printable characters
			if (x < text[y].size()) {
				text[y].insert(x, 1, c); // Insert character at position x
			}
			else {
				text[y].push_back(c); // Append character at the end
			}
			x++; // Move cursor right
		}
		else if (c == '\t') { // Handle tab key
			// Insert spaces for the tab
			for (int i = 0; i < TAB_SIZE; ++i) {
				if (x < text[y].size()) {
					text[y].insert(x, 1, ' '); // Insert space at position x
				} else {
					text[y].push_back(' '); // Append space at the end
				}
				x++; // Move cursor right
			}
		}
		else if (c == KEY_LEFT) { // Move cursor left
			if (x > 0) {
				x--;
			}
			else if (y > 0) { // Move to the end of the previous line
				y--;
				x = text[y].size();
			}
		}
		else if (c == KEY_RIGHT) { // Move cursor right
			if (x < text[y].size()) {
				x++;
			}
			else if (y < text.size() - 1) { // Move to the beginning of the next line
				y++;
				x = 0;
			}
		}
		else if (c == KEY_UP) { // Move cursor up
			if (y > 0) {
				y--;
				if (x > text[y].size()) {
					x = text[y].size(); // Adjust x if it exceeds the new line length
				}
			}
		}
		else if (c == KEY_DOWN) { // Move cursor down
			if (y < text.size() - 1) {
				y++;
				if (x > text[y].size()) {
					x = text[y].size(); // Adjust x if it exceeds the new line length
				}
			}
		}
	}
	scrrend();
	endwin(); // End ncurses mode
	return 1;
}

// Clear the current line
void clearline() {
	for (unsigned short i = 0; i < w; ++i) {
		addch(' ');
	}
}

// Render the screen
void scrrend() {
	if (toprend > y) {
		toprend = y; // Adjust toprend if the cursor is above the visible area
	}
	else if (toprend + h - 2 < y) {
		toprend = y - (h - 2); // Adjust toprend if the cursor is below the visible area
	}

	for (unsigned short i = 0; i < h - 1; ++i) {
		move(i, 0);
		clearline();
		move(i, 0);
		if (i + toprend >= text.size()) {
			addch('~'); // Indicate empty lines
		}
		else {
			printw(text[i + toprend].c_str());
		}
	}

	// Move the cursor to the correct position
	move(y - toprend, x);
}

// Save the current text to a file
void saveF(std::string file) {
	std::ofstream ofs(file);
	if (!ofs) {
		// Handle error
		move(h - 1, 0);
		printw("Error saving file: %s", file.c_str());
		refresh();
		return;
	}
	for (const auto& line : text) {
		ofs << line << "\n";
	}
	ofs.close();
}

// Load text from a file
void loadF(std::string file) {
	std::ifstream ifs(file);
	if (!ifs) {
		// Handle error
		move(h - 1, 0);
		printw("Error loading file: %s", file.c_str());
		refresh();
		return;
	}
	text.clear(); // Clear current text
	std::string line;
	while (std::getline(ifs, line)) {
		text.push_back(line);
	}
	ifs.close();
	y = 0; // Reset cursor position
	x = 0;
}
