#include <ncurses.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include "php_fileselect.h"
#define MAX_ENTRIES 100
#define LINES_PER_PAGE 25
#define MAX_NAME_LEN 64
struct Entry {
	char name[256];
	int is_dir;
};

struct Entry entries[MAX_ENTRIES];
int num_entries = 0;
int current_selection = 0;
int current_page = 0;
int no_files = 0;
char current_path[1024];
char previous_path[1024];
char search_term[128];
int is_search = 0;
char ct_comfirm[256];

char *root_paths[] = {
	"/sdcard",
	"/data/data/com.termux/files/home/",
	"./",
	"/"
};
const char *root_names[] = {
	"Phone Storage",
	"Termux Storage",
	"Current directory",
	"Custom Path"
};
int num_roots = sizeof(root_paths) / sizeof(root_paths[0]);
int current_root = 0;
int is_dot_or_dotdot(const char *name) {
	return (strcmp(name, ".") == 0 || strcmp(name, "..") == 0);
}
void updateRealPaths() {
	char resolved_path[PATH_MAX];
	if (realpath(current_path, resolved_path) != NULL) {
		strcpy(current_path, resolved_path);
	}
	if (realpath(previous_path, resolved_path) != NULL) {
		strcpy(previous_path, resolved_path);
	}
}
char *real_paths(char path[]) {
	char resolved_path[PATH_MAX];
	if (realpath(path, resolved_path) != NULL) {
		return strdup(resolved_path);
	} else {
		return "#quit";
	}
}
void list_directory(const char *path) {
	num_entries = 0;
	struct dirent *entry;
	DIR *dir = opendir(path);
	if (!dir) {
		return;
	}
	snprintf(entries[num_entries].name, sizeof(entries[num_entries].name), "..");
	entries[num_entries].is_dir = 1;
	num_entries++;
	while ((entry = readdir(dir))) {
		if (is_dot_or_dotdot(entry->d_name)) {
			continue; // Skip "." & ".."
		}
		struct stat statbuf;
		snprintf(entries[num_entries].name, sizeof(entries[num_entries].name), "%s", entry->d_name);
		char full_path[1024];
		snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

		if (stat(full_path, &statbuf) == 0) {
			entries[num_entries].is_dir = S_ISDIR(statbuf.st_mode);
		} else {
			entries[num_entries].is_dir = 0;
		}

		num_entries++;
	}

	closedir(dir);
}

void search() {
	// Delete the current list
	num_entries = 0;

	// Browse the folder again to filter
	struct dirent *entry;
	DIR *dir = opendir(current_path);
	if (!dir) {
		return;
	}
	while ((entry = readdir(dir))) {
		// Skip . and ..
		if (is_dot_or_dotdot(entry->d_name)) continue;

		// Check if it contains keywords or not
		if (strstr(entry->d_name, search_term)) {
			// Add to list
			struct stat statbuf;
			snprintf(entries[num_entries].name, sizeof(entries[num_entries].name), "%s", entry->d_name);
			char full_path[1024];
			snprintf(full_path, sizeof(full_path), "%s/%s", current_path, entry->d_name);

			if (stat(full_path, &statbuf) == 0) {
				entries[num_entries].is_dir = S_ISDIR(statbuf.st_mode);
			} else {
				entries[num_entries].is_dir = 0;
			}
			num_entries++;
		}
	}

	closedir(dir);
}

void display_page() {
	int start = current_page * LINES_PER_PAGE;
	int end = start + LINES_PER_PAGE;

	if (end > num_entries) {
		end = num_entries;
	}
	char shortName[MAX_NAME_LEN+1];
	for (int i = start; i < end; i++) {
		if (i == current_selection) {
			attron(A_REVERSE);
		}

		if (entries[i].is_dir) {
			attron(COLOR_PAIR(2));
			if (strlen(entries[i].name) <= MAX_NAME_LEN) {
				mvprintw(i - start + 11, 0, "[dir] %s", entries[i].name);
			} else {
				memcpy(shortName, entries[i].name, MAX_NAME_LEN);
				shortName[MAX_NAME_LEN] = '\0';
				mvprintw(i - start + 11, 0, "[dir] %s...", shortName);
			}
			attroff(COLOR_PAIR(2));
		} else {
			attron(COLOR_PAIR(3));
			if (strlen(entries[i].name) <= MAX_NAME_LEN) {
				mvprintw(i - start + 11, 0, "[file] %s", entries[i].name);
			} else {
				memcpy(shortName, entries[i].name, MAX_NAME_LEN);
				shortName[MAX_NAME_LEN] = '\0';
				mvprintw(i - start + 11, 0, "[file] %s...", shortName);
			}
			attroff(COLOR_PAIR(3));
		}

		if (i == current_selection) {
			attroff(A_REVERSE);
		}
	}
}

char *fileselect() {
	initscr();
	cbreak();
	noecho();
	start_color();
	keypad(stdscr, TRUE);
	strcpy(current_path, root_paths[current_root]);
	previous_path[0] = '\0';
	current_selection = 0;
	current_page = 0;
	init_pair(1, COLOR_RED, COLOR_BLACK);
	init_pair(2, COLOR_BLUE, COLOR_BLACK);
	init_pair(3, COLOR_GREEN, COLOR_BLACK);
	init_pair(4, COLOR_CYAN, COLOR_BLACK);
	init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
	init_pair(6, COLOR_YELLOW, COLOR_BLACK);

	while (1) {
		updateRealPaths();
		clear();
		if (!is_search) {
			list_directory(current_path);
		}else {
			search();
		}
		check:
		attron(COLOR_PAIR(2));
		mvprintw(0, 0, "Welcome to File Selector");
		attroff(COLOR_PAIR(2));
		attron(COLOR_PAIR(1));
		mvprintw(1, 0, "Root directory: %s", root_names[current_root]);
		attroff(COLOR_PAIR(1));
		attron(COLOR_PAIR(5));
		mvprintw(2, 0, "Enter : Select File/Open Folder");
		attroff(COLOR_PAIR(5));
		attron(COLOR_PAIR(6));
		mvprintw(3, 0, "h : Show help");
		attroff(COLOR_PAIR(6));
		attron(COLOR_PAIR(3));
		mvprintw(4, 0, "Navigation keys : Move the cursor");
		attroff(COLOR_PAIR(3));
		attron(COLOR_PAIR(5));
		mvprintw(5, 0, "q : Close File Selector");
		attroff(COLOR_PAIR(5));
		attron(COLOR_PAIR(3));
		mvprintw(6, 0, "Current Path: %s", current_path);
		attroff(COLOR_PAIR(3));
		if (num_entries > 0) {
			no_files = 0;
			display_page();
		} else {
			no_files = 1;
			attron(COLOR_PAIR(1));
			mvprintw(11, 0, "No files or directories found.");
			attroff(COLOR_PAIR(1));
		}

		int ch = getch();
		switch (ch) {
			case 'q':
			endwin();
			return "#quit";
			case 'b': // 'back' key
			updateRealPaths();
			is_search = 0;
			if (strcmp(current_path, root_paths[current_root]) != 0) {
				strcpy(previous_path, current_path);
				char *last_slash = strrchr(current_path, '/');
				if (last_slash) {
					*last_slash = '\0'; // Remove the last part of the path
				}
				current_selection = 0;
				current_page = 0;
			}
			updateRealPaths();
			break;
			case 'n': // 'new' key
			updateRealPaths();
			is_search = 0;
			clear();
			attron(COLOR_PAIR(2));
			mvprintw(0, 0, "Current Path: %s", current_path);
			attroff(COLOR_PAIR(2));
			attron(COLOR_PAIR(3));
			mvprintw(3, 0, "Enter new file name: ");
			attroff(COLOR_PAIR(3));
			echo();
			char new_file_name[256];
			getstr(new_file_name);
			noecho();
			if (new_file_name[0] != '\0') {
				char full_path[1024];
				snprintf(full_path, sizeof(full_path), "%s/%s", current_path, new_file_name);
				FILE *new_file = fopen(full_path, "w");
				if (new_file) {
					fclose(new_file);
					attron(COLOR_PAIR(6));
					mvprintw(4, 0, "Selected: %s ", full_path);
					attroff(COLOR_PAIR(6));
					endwin();
					return real_paths(full_path);
				} else {
					attron(COLOR_PAIR(1));
					mvprintw(4, 0, "Failed to create new file.");
					attroff(COLOR_PAIR(1));
				}
				refresh();
				getch();
			}
			break;
			case 'f':
			if(!is_search) {
				attron(COLOR_PAIR(6));
				mvprintw(9, 0, "Enter search keywords: ");
				attroff(COLOR_PAIR(6));
				echo();
				getstr(search_term);
				if(num_entries <= 0) {
					attron(COLOR_PAIR(1));
					mvprintw(9, 0, "There are no files or folders to search.");
					attron(COLOR_PAIR(6));
					mvprintw(10, 0, "Press Enter to continue");
					attroff(COLOR_PAIR(6));
					echo();
					getstr(ct_comfirm);
					noecho();
					attroff(COLOR_PAIR(1));
					break;
				}
				is_search = 1;
				current_page = current_selection = 0;
				noecho();
			}else {
				is_search = 0;
			}
			break;
			case 'i': // 'input full path' key
			updateRealPaths();
			clear();
			attron(COLOR_PAIR(2));
			mvprintw(0, 0, "Current Path: %s", current_path);
			attroff(COLOR_PAIR(2));
			attron(COLOR_PAIR(3));
			mvprintw(3, 0, "Enter full path : ");
			attroff(COLOR_PAIR(3));
			echo();
			char f_path_f[256];
			getstr(f_path_f);
			noecho();
			if (f_path_f[0] != '\0') {
				attron(COLOR_PAIR(6));
				mvprintw(4, 0, "Selected: %s ", f_path_f);
				attroff(COLOR_PAIR(6));
				endwin();
				return strdup(f_path_f);
			}
			break;
			case 'g': // 'goto' key
			updateRealPaths();
			clear();
			attron(COLOR_PAIR(2));
			mvprintw(0, 0, "Current Path: %s", current_path);
			attroff(COLOR_PAIR(2));
			attron(COLOR_PAIR(3));
			mvprintw(3, 0, "Enter the path to go to : ");
			attroff(COLOR_PAIR(3));
			echo();
			char goto_path[256];
			getstr(goto_path);
			noecho();
			if (goto_path[0] != '\0') {
				attron(COLOR_PAIR(6));
				mvprintw(4, 0, "Selected : %s ", goto_path);
				attroff(COLOR_PAIR(6));
				strcpy(current_path, goto_path);
				root_paths[3] = (char *)goto_path;
				current_root = 3;
				current_selection = 0;
				current_page = 0;
			}
			break;
			case '\n':
			if (entries[current_selection].is_dir && !no_files) {
				strcpy(previous_path, current_path);
				strcat(current_path, "/");
				strcat(current_path, entries[current_selection].name);
				current_selection = 0;
				current_page = 0;
				is_search = 0;
			}else if(!entries[current_selection].is_dir) {
				clear();
				attron(COLOR_PAIR(2));
				mvprintw(0, 0, "Current Path: %s", current_path);
				attroff(COLOR_PAIR(2));
				char full_path[1024];
				snprintf(full_path, sizeof(full_path), "%s/%s", current_path, entries[current_selection].name);
				attron(COLOR_PAIR(6));
				mvprintw(4, 0, "Selected: %s ", full_path);
				attroff(COLOR_PAIR(6));
				endwin();
				return real_paths(full_path);
			}
			break;
			case KEY_UP:
			case 'w':
			if (current_selection == 0) {
				current_selection = num_entries - 1;
				current_page = (int)((num_entries - 1) / LINES_PER_PAGE);
			} else if (current_selection > 0) {
				current_selection--;
				if (current_selection < current_page * LINES_PER_PAGE) {
					current_page--;
				}
			}
			break;
			case 'h':
			clear();
			attron(COLOR_PAIR(2));
			mvprintw(0, 0, "Welcome to File Selector");
			attroff(COLOR_PAIR(2));
			attron(COLOR_PAIR(1));
			mvprintw(1, 0, "Help");
			attroff(COLOR_PAIR(1));
			attron(COLOR_PAIR(5));
			mvprintw(2, 0, "Enter : Select File/Open Folder");
			attroff(COLOR_PAIR(5));
			attron(COLOR_PAIR(6));
			mvprintw(3, 0, "n : Create a new file and select it");
			attroff(COLOR_PAIR(6));
			attron(COLOR_PAIR(2));
			mvprintw(4, 0, "b : Go back to the parent directory");
			attroff(COLOR_PAIR(2));
			attron(COLOR_PAIR(3));
			mvprintw(5, 0, "Navigation keys : Move the cursor");
			attroff(COLOR_PAIR(3));
			attron(COLOR_PAIR(4));
			mvprintw(6, 0, "i : Enter the file path and return");
			attroff(COLOR_PAIR(4));
			attron(COLOR_PAIR(6));
			mvprintw(7, 0, "g : Go to the specified path");
			attroff(COLOR_PAIR(6));
			attron(COLOR_PAIR(5));
			mvprintw(8, 0, "q : Close File Selector");
			attroff(COLOR_PAIR(5));
			attron(COLOR_PAIR(2));
			mvprintw(9, 0, "f : Search/cancel search");
			attroff(COLOR_PAIR(2));
			attron(COLOR_PAIR(1));
			mvprintw(11, 0, "Press Enter to continue");
			attroff(COLOR_PAIR(1));
			echo();
			getstr(ct_comfirm);
			noecho();
			break;
			case KEY_DOWN:
			case 's':
			if (current_selection == num_entries - 1) {
				current_selection = 0;
				current_page = 0;
			} else if (current_selection < num_entries - 1) {
				current_selection++;
				if (current_selection >= (current_page + 1) * LINES_PER_PAGE) {
					current_page++;
				}
			}
			break;
			case KEY_RIGHT:
			case 'd':
			current_root = (current_root + 1) % num_roots;
			strcpy(current_path, root_paths[current_root]);
			previous_path[0] = '\0';
			current_selection = 0;
			current_page = 0;
			break;
			case KEY_LEFT:
			case 'a':
			current_root = (current_root - 1);
			if(current_root == -1)current_root = 3;
			strcpy(current_path, root_paths[current_root]);
			previous_path[0] = '\0';
			current_selection = 0;
			current_page = 0;
			break;
		}

		refresh();
	}

	endwin();
	return "#quit";
}

ZEND_FUNCTION(file_selector) {
	ZEND_PARSE_PARAMETERS_START(0, 0)
	ZEND_PARSE_PARAMETERS_END();

	char *result = fileselect();
	RETURN_STRING(result);
}

ZEND_BEGIN_ARG_INFO(arginfo_file_selector, 0)
ZEND_ARG_INFO(0, "The tool returns the realpath of a file")
ZEND_END_ARG_INFO()

zend_function_entry fileselect_functions[] = {
	PHP_FE(file_selector, arginfo_file_selector)
	PHP_FE_END
};

zend_module_entry fileselect_module_entry = {
	STANDARD_MODULE_HEADER,
	"Dgbaodev Libs : File selector",
	// Your extension's name
	fileselect_functions,
	// An array of your extension's functions
	NULL,
	// PHP_MINIT callback
	NULL,
	// PHP_MSHUTDOWN callback
	NULL,
	// PHP_RINIT callback
	NULL,
	// PHP_RSHUTDOWN callback
	NULL,
	// PHP_MINFO callback
	"1.0",
	// Your extension's version
	STANDARD_MODULE_PROPERTIES
};


#ifdef COMPILE_DL_FILESELECT
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(fileselect)
#endif