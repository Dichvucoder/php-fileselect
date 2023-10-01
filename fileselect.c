#include <ncurses.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "php_fileselect.h"
#define MAX_ENTRIES 100
#define LINES_PER_PAGE 25

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
	return (strcmp(name, ".") == 0);
}
void list_directory(const char *path) {
	num_entries = 0;
	struct dirent *entry;
	DIR *dir = opendir(path);
	if (!dir) {
		perror("opendir");
		return;
	}

	while ((entry = readdir(dir))) {
		if (is_dot_or_dotdot(entry->d_name)) {
			continue; // Skip "."
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

void display_page() {
	int start = current_page * LINES_PER_PAGE;
	int end = start + LINES_PER_PAGE;

	if (end > num_entries) {
		end = num_entries;
	}

	for (int i = start; i < end; i++) {
		if (i == current_selection) {
			attron(A_REVERSE);
		}

		mvprintw(i - start + 10, 0, "%s", entries[i].name);

		if (i == current_selection) {
			attroff(A_REVERSE);
		}
	}
}

char *fileselect() {
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	strcpy(current_path, root_paths[current_root]);
	previous_path[0] = '\0';
	current_selection = 0;
	current_page = 0;
	char cwd[512];
	if (getcwd(cwd, sizeof(cwd)) != NULL) {
        root_paths[2] = (char *)cwd;
  }
	while (1) {
		clear();
		mvprintw(0, 0, "Welcome to File Select");
		mvprintw(1, 0, "Current Path: %s", current_path);
		mvprintw(2, 0, "Root directory: %s", root_names[current_root]);
		mvprintw(3, 0, "Enter : Select File/Open Folder");
		mvprintw(4, 0, "n : Create a new file and select it");
		mvprintw(5, 0, "b : Go back to the parent directory");
		mvprintw(6, 0, "Navigation keys : Move the cursor");
		mvprintw(7, 0, "f : Use full path instead of File Select");
		mvprintw(8, 0, "g : Go to the specified path");
		list_directory(current_path);

		if (num_entries > 0) {
			no_files = 0;
			display_page();
		} else {
			no_files = 1;
			mvprintw(10, 0, "No files or directories found.");
		}

		int ch = getch();

		switch (ch) {
			case 'q':
			endwin();
			return "quit";
			case 'b': // 'back' key
			if (strcmp(current_path, root_paths[current_root]) != 0) {
				strcpy(previous_path, current_path);
				char *last_slash = strrchr(current_path, '/');
				if (last_slash) {
					*last_slash = '\0'; // Remove the last part of the path
				}
				current_selection = 0;
				current_page = 0;
			}
			break;
			case 'n': // 'new' key
			clear();
			mvprintw(0, 0, "Current Path: %s", current_path);
			mvprintw(1, 0, "Enter new file name: ");
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
					mvprintw(2, 0, "Selected: %s ", full_path);
					endwin();
					return full_path;
				} else {
					mvprintw(2, 0, "Failed to create new file.");
				}
				refresh();
				getch();
			}
			break;
			case 'f': // 'full' key
			clear();
			mvprintw(0, 0, "Current Path: %s", current_path);
			mvprintw(1, 0, "Enter full path : ");
			echo();
			char f_path_f[256];
			getstr(f_path_f);
			noecho();
			if (f_path_f[0] != '\0') {
				mvprintw(2, 0, "Selected: %s ", f_path_f);
				endwin();
				return f_path_f;
			}
			break;
			case 'g': // 'goto' key
			clear();
			mvprintw(0, 0, "Current Path: %s", current_path);
			mvprintw(1, 0, "Enter the path to go to : ");
			echo();
			char goto_path[256];
			getstr(goto_path);
			noecho();
			if (goto_path[0] != '\0') {
				mvprintw(2, 0, "Selected : %s ", goto_path);
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
			}else if(!entries[current_selection].is_dir) {
				clear();
				mvprintw(0, 0, "Current Path: %s", current_path);
				char full_path[1024];
				snprintf(full_path, sizeof(full_path), "%s/%s", current_path, entries[current_selection].name);
				mvprintw(2, 0, "Selected: %s ", full_path);
				endwin();
				return full_path;
			}
			break;
			case KEY_UP:
			case 'w':
			if (current_selection > 0) {
				current_selection--;
				if (current_selection < current_page * LINES_PER_PAGE) {
					current_page--;
				}
			}
			break;
			case KEY_DOWN:
			case 's':
			if (current_selection < num_entries - 1) {
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
	return "quit";
}

ZEND_FUNCTION(file_select) {
	ZEND_PARSE_PARAMETERS_START(0, 0)
	ZEND_PARSE_PARAMETERS_END();

	char *result = fileselect();
	RETURN_STRING(result);
}

ZEND_BEGIN_ARG_INFO(arginfo_file_select, 0)
ZEND_ARG_INFO(0, "The tool returns the realpath of a file")
ZEND_END_ARG_INFO()

zend_function_entry fileselect_functions[] = {
	PHP_FE(file_select, arginfo_file_select)
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