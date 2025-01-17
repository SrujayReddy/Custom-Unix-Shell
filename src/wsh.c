// Include standard libraries for input/output, standard functions, string manipulation,
// UNIX-specific functions like fork() and exec(), and boolean type definitions.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdbool.h>

// Define constants for maximum input line length, maximum number of arguments in a command,
// maximum number of commands to keep in history, a placeholder for the 'history set' command,
// and the maximum number of local variables supported.
#define MAX_LINE_LENGTH 1024
#define MAX_ARGS 128
#define MAX_HISTORY 5
#define HISTORY_SET "history set"
#define MAX_LOCAL_VARS 128

// Global variables for managing command history and local variables.
char **history;                     // Dynamically allocated array of strings to store command history.
int current_history_count = 0;      // Current number of commands in the history.
int history_capacity = MAX_HISTORY; // Maximum number of commands history can hold.
bool add_to_history_enabled = true; // Flag to enable/disable adding commands to history.

// Structure to represent a local variable with a name and a value.
typedef struct
{
    char name[MAX_LINE_LENGTH];
    char value[MAX_LINE_LENGTH];
} LocalVar;

LocalVar localVars[MAX_LOCAL_VARS]; // Array to store local variables.
int localVarCount = 0;              // Current number of local variables stored.

// Function prototypes for processing and executing commands, managing history and local variables.
void process_input(char *input, char *argv[], int *background);             // Parses input into command arguments.
void execute_command(char *argv[], int background);                         // Executes a given command.
int built_in_command(char *argv[]);                                         // Checks and executes built-in commands.
void print_history();                                                       // Displays the command history.
void add_to_history(const char *cmd);                                       // Adds a command to the history.
void set_history_size(int size);                                            // Adjusts the size of the command history.
void execute_history_command(int command_number);                           // Executes a command from the history.
void execute_piped_commands(char **commands, int num_cmds, int background); // Executes piped commands.
void parse_and_execute(char *input);                                        // Parses and executes an input command.
void set_local_var(char *name, char *value);                                // Sets a local variable.
void substitute_variable(char **arg);                                       // Substitutes variables in a command argument.
bool isValidCommand(char *argv[]);                                          // Checks if a command is valid.
void substitute_variables_in_command(char *argv[]);                         // Substitutes variables in all command arguments.
bool isBuiltInCommand(char *command);                                       // Checks if a command is a built-in command.

// Handlers for built-in commands.
void cmd_cd(char *path);                  // Changes the current directory.
void cmd_exit(char *argv[]);              // Exits the shell.
void cmd_history_control(char *argv[]);   // Manages history commands.
void cmd_export(char *name, char *value); // Sets an environment variable.
void cmd_local(char *name, char *value);  // Sets a local variable.
void cmd_vars();                          // Displays all local variables.

/**
 * Entry point of the shell program.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 * @return Returns 0 on successful execution, and 1 or -1 on errors.
 */
int main(int argc, char *argv[])
{
    char input[MAX_LINE_LENGTH]; // Buffer to hold user input.
    // Allocate memory for storing command history.
    history = malloc(history_capacity * sizeof(char *));
    // Check for memory allocation failure.
    if (!history)
    {
        perror("Failed to allocate memory for history");
        return 1;
    }
    // Initialize history array with NULL.
    for (int i = 0; i < history_capacity; i++)
    {
        history[i] = NULL;
    }

    // Check for batch file mode.
    if (argc == 2)
    {
        FILE *batchFile = fopen(argv[1], "r"); // Try to open the batch file.
        if (!batchFile)
        {
            perror("Error opening batch file");
            exit(-1);
        }

        // Read and execute commands from the batch file.
        while (fgets(input, MAX_LINE_LENGTH, batchFile) != NULL)
        {
            if (input[strlen(input) - 1] == '\n')
            {
                input[strlen(input) - 1] = '\0'; // Remove newline character.
            }
            parse_and_execute(input);
        }

        fclose(batchFile);
        exit(EXIT_SUCCESS);
    }
    else if (argc > 2) // Incorrect usage of the program.
    {
        fprintf(stderr, "Usage: %s [batch file]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Interactive mode: read and execute commands from stdin.
    while (1)
    {
        printf("wsh> ");
        fflush(stdout);
        if (!fgets(input, MAX_LINE_LENGTH, stdin))
        {
            if (feof(stdin)) // Check for end of input.
            {
                exit(0);
            }
            else
            {
                perror("fgets error");
                continue;
            }
        }

        if (strcmp(input, "\n") == 0) // Ignore empty lines.
            continue;

        parse_and_execute(input); // Parse and execute the input command.
    }

    // Cleanup: free allocated memory for command history.
    for (int i = 0; i < current_history_count; i++)
    {
        free(history[i]);
    }
    free(history);
    return 0;
}

/**
 * Parses the input string into command arguments.
 *
 * @param input The input string to be parsed.
 * @param argv Array to store the parsed arguments.
 * @param background Pointer to an integer to indicate background execution.
 */
void process_input(char *input, char *argv[], int *background)
{
    char *token = strtok(input, " \n"); // Tokenize the input.
    int index = 0;                      // Index for storing arguments.

    // Loop through all tokens and add them to argv.
    while (token != NULL && index < MAX_ARGS - 1)
    {
        argv[index++] = token;
        token = strtok(NULL, " \n");
    }
    argv[index] = NULL; // Null-terminate the argument list.

    // Check for background execution flag '&'.
    if (index > 0 && strcmp(argv[index - 1], "&") == 0)
    {
        *background = 1;      // Set background execution flag.
        argv[--index] = NULL; // Remove '&' from arguments.
    }
}

/**
 * Executes a command.
 *
 * @param argv Array containing command and arguments.
 * @param background Integer indicating if the command should run in the background.
 */
void execute_command(char *argv[], int background)
{
    char *filtered_argv[MAX_ARGS]; // Array for filtered arguments after substitution.
    int filtered_index = 0;        // Index for filtered arguments.

    // Perform variable substitution for each argument.
    for (int i = 0; argv[i] != NULL; i++)
    {
        substitute_variable(&argv[i]);
    }

    // Copy non-empty strings to filtered_argv.
    for (int i = 0; argv[i] != NULL; i++)
    {
        if (strcmp(argv[i], "") != 0)
        {
            filtered_argv[filtered_index++] = argv[i];
        }
    }
    filtered_argv[filtered_index] = NULL; // Terminate the filtered argument list.

    // Validate the command after substitution.
    if (!isValidCommand(filtered_argv))
    {
        printf("Error: Command validation failed.\n");
        return; // Exit if the command is invalid.
    }

    pid_t pid = fork(); // Create a new process.
    if (pid == 0)       // Child process.
    {
        // Execute the command with execvp.
        if (execvp(filtered_argv[0], filtered_argv) == -1)
        {
            perror("execvp");
            exit(-1);
        }
    }
    else if (pid > 0) // Parent process.
    {
        if (!background) // Wait for the child process if not running in background.
        {
            int status;
            waitpid(pid, &status, 0);
        }
        else
        {
            printf("[PID %d running in background]\n", pid);
        }
    }
    else // Fork failed.
    {
        perror("fork failed");
        exit(-1);
    }
}

/**
 * Checks and executes built-in commands such as exit, cd, history, export, local, and vars.
 *
 * @param argv Array of command and arguments.
 * @return Returns 1 if a built-in command is executed, 0 otherwise.
 */
int built_in_command(char *argv[])
{
    // Handling 'exit' command.
    if (strcmp(argv[0], "exit") == 0)
    {
        cmd_exit(argv); // Call the exit command handler.
        return 1;
    }
    // Handling 'cd' command for changing directory.
    else if (strcmp(argv[0], "cd") == 0)
    {
        cmd_cd(argv[1]); // Call the cd command handler with the directory path.
        return 1;
    }
    // Handling 'history' command to display or set history size.
    else if (strcmp(argv[0], "history") == 0)
    {
        cmd_history_control(argv); // Call the history command handler.
        return 1;
    }
    // Handling 'export' command for setting environment variables.
    else if (strcmp(argv[0], "export") == 0 && argv[1] != NULL)
    {
        char *name = strtok(argv[1], "=");
        char *value = strtok(NULL, "");
        cmd_export(name, value); // Call the export command handler with name and value.
        return 1;
    }
    // Handling 'local' command for setting local (shell) variables.
    else if (strcmp(argv[0], "local") == 0 && argv[1] != NULL)
    {
        char *name = strtok(argv[1], "=");
        char *value = strtok(NULL, "");
        cmd_local(name, value); // Call the local command handler with name and value.
        return 1;
    }
    // Handling 'vars' command to list all local variables.
    else if (strcmp(argv[0], "vars") == 0)
    {
        cmd_vars(); // Call the vars command handler.
        return 1;
    }
    return 0; // Indicate that the command is not a built-in command.
}

/**
 * Adds a command to the history, avoiding duplicates and managing history size.
 *
 * @param cmd Command string to be added to the history.
 */
void add_to_history(const char *cmd)
{
    // Ignore if history is disabled, or cmd is NULL, empty, or all whitespace.
    if (!add_to_history_enabled || cmd == NULL || *cmd == '\0' || strspn(cmd, " \t\n\v\f\r") == strlen(cmd))
    {
        return;
    }

    // Avoid adding if the last command in history is the same.
    if (current_history_count > 0 && strcmp(history[current_history_count - 1], cmd) == 0)
    {
        return;
    }

    // Remove the oldest command if history is at capacity.
    if (current_history_count == history_capacity)
    {
        free(history[0]);
        memmove(history, history + 1, sizeof(char *) * (current_history_count - 1));
        current_history_count--;
    }

    // Add the new command to history.
    history[current_history_count++] = strdup(cmd);
}

/**
 * Sets the size of the command history buffer. This function allows dynamically adjusting
 * the number of commands that can be stored in the shell's history. If the new size is smaller
 * than the current number of commands in the history, the oldest commands are discarded. If the
 * new size is larger, the history capacity is increased.
 *
 * @param newSize The new size for the command history buffer.
 */
void set_history_size(int newSize)
{
    // Validate the new size to ensure it's not negative.
    if (newSize < 0)
    {
        fprintf(stderr, "Invalid history size.\n");
        return; // Exit if the new size is invalid.
    }

    // Enable or disable adding commands to history based on the new size.
    if (newSize == 0)
    {
        add_to_history_enabled = false; // Disable history if size is set to 0.
    }
    else
    {
        add_to_history_enabled = true; // Enable history for any size greater than 0.
    }

    // If the current history count exceeds the new size, remove the oldest entries.
    while (current_history_count > newSize)
    {
        free(history[0]); // Free the memory of the oldest command.
        // Shift the remaining commands towards the beginning of the array.
        for (int i = 1; i < current_history_count; i++)
        {
            history[i - 1] = history[i];
        }
        current_history_count--; // Decrement the count to reflect the removed command.
    }

    // Attempt to reallocate the history array to match the new size.
    char **temp = realloc(history, newSize * sizeof(char *));
    if (!temp && newSize > 0) // Check for reallocation failure.
    {
        perror("Unable to resize history");
        return;
    }
    history = temp;             // Update the history pointer to the newly allocated memory.
    history_capacity = newSize; // Update the capacity to reflect the new size.

    // Ensure the current history count does not exceed the new size.
    current_history_count = current_history_count > newSize ? newSize : current_history_count;

    // Initialize any new entries to NULL if the history size has been increased.
    for (int i = current_history_count; i < newSize; i++)
    {
        history[i] = NULL;
    }
}

/**
 * Executes a specific command from history based on its number.
 *
 * @param command_number The number of the command in history to execute.
 */
void execute_history_command(int command_number)
{
    // Validate the command number.
    if (command_number < 1 || command_number > current_history_count)
    {
        printf("Invalid history command number.\n");
        return;
    }

    // Duplicate the command to ensure it can be modified if needed.
    char *cmd = strdup(history[current_history_count - command_number]);

    // Temporarily disable adding commands to history to prevent recursion.
    bool oldHistoryState = add_to_history_enabled;
    add_to_history_enabled = false;
    parse_and_execute(cmd);                   // Parse and execute the duplicated command.
    add_to_history_enabled = oldHistoryState; // Restore the history state.

    free(cmd); // Clean up the duplicated command string.
}

/**
 * Validates if the provided command and arguments are syntactically correct.
 *
 * @param argv Array of command arguments to validate.
 * @return True if the command is valid, False otherwise.
 */
bool isValidCommand(char *argv[])
{
    // Command cannot be empty.
    if (argv[0] == NULL || strcmp(argv[0], "") == 0)
    {
        printf("Error: Command is empty.\n");
        return false;
    }

    // Special validation for 'cd' command: must have exactly one argument.
    if (strcmp(argv[0], "cd") == 0)
    {
        if (argv[1] == NULL || argv[2] != NULL)
        {
            printf("cd: wrong number of arguments. Usage: cd <directory>\n");
            return false;
        }
    }
    // 'exit' command should not have any arguments.
    else if (strcmp(argv[0], "exit") == 0 && argv[1] != NULL)
    {
        printf("exit: does not take any arguments.\n");
        return false;
    }
    // 'export' and 'local' commands require a VAR=value format.
    else if ((strcmp(argv[0], "export") == 0 || strcmp(argv[0], "local") == 0) &&
             (argv[1] == NULL || argv[2] != NULL || strchr(argv[1], '=') == NULL))
    {
        printf("%s: incorrect usage. Expected format: %s VAR=value\n", argv[0], argv[0]);
        return false;
    }
    // 'history' command checks for correct usage with optional 'set' argument.
    else if (strcmp(argv[0], "history") == 0 && argv[1] != NULL)
    {
        if (strcmp(argv[1], "set") == 0)
        {
            if (argv[2] == NULL || atoi(argv[2]) <= 0 || argv[3] != NULL)
            {
                printf("history set: incorrect usage.\n");
                return false;
            }
        }
        else
        {
            printf("history: incorrect usage. Usage: history [set <size>]\n");
            return false;
        }
    }

    return true; // If all checks pass, the command is considered valid.
}

/**
 * Parses the input command and executes it. This function handles both built-in and external commands,
 * including support for piping and command history.
 *
 * @param input The command line input to parse and execute.
 */
void parse_and_execute(char *input)
{
    // Remove trailing newline, if present, to clean the input for processing.
    if (input[strlen(input) - 1] == '\n')
    {
        input[strlen(input) - 1] = '\0';
    }

    // Duplicate the input for safe modifications during parsing.
    char *dupInp = strdup(input);

    // Prepare to check if the command is a built-in command.
    char tempInput[MAX_LINE_LENGTH];
    strncpy(tempInput, input, MAX_LINE_LENGTH - 1);
    tempInput[MAX_LINE_LENGTH - 1] = '\0';

    // Extract the first word to identify built-in commands.
    char *firstToken = strtok(tempInput, " \t\n");
    bool isBuiltIn = false; // Flag to track if the command is built-in.

    // Directly execute built-in commands, bypassing history addition for certain commands.
    if (firstToken != NULL && isBuiltInCommand(firstToken))
    {
        isBuiltIn = true; // Mark as a built-in command.
        char *argv[MAX_ARGS];
        int background = 0;
        process_input(input, argv, &background); // Process the command.

        if (strcmp(firstToken, "history") != 0 || (argv[1] != NULL))
        {
            built_in_command(argv); // Execute if not a pure history view request.
        }
        else if (strcmp(firstToken, "history") == 0)
        {
            print_history(); // Handle history printing separately.
        }
    }
    else
    {
        // Handle non-built-in commands, including piped commands.
        char *commands[MAX_ARGS];
        int num_cmds = 0, background = 0;
        char *token = strtok(input, "|");

        // Split the command by pipes, if any.
        while (token != NULL && num_cmds < MAX_ARGS - 1)
        {
            commands[num_cmds++] = token;
            token = strtok(NULL, "|");
        }

        if (num_cmds == 1)
        {
            // Execute a single command without piping.
            char *argv[MAX_ARGS];
            process_input(commands[0], argv, &background);
            execute_command(argv, background);
        }
        else
        {
            // Execute piped commands.
            execute_piped_commands(commands, num_cmds, background);
        }
    }

    // Add the command to history if appropriate.
    if (!isBuiltIn && strcmp(firstToken, "history") != 0)
    {
        add_to_history(dupInp);
    }

    free(dupInp); // Free the duplicated input string.
}

/**
 * Checks if the given command is a built-in command of the shell.
 *
 * @param command The command to check.
 * @return True if the command is built-in, False otherwise.
 */
bool isBuiltInCommand(char *command)
{
    // Define an array of all built-in commands for comparison.
    const char *builtInCommands[] = {"cd", "exit", "history", "export", "local", "vars"};
    int builtInCommandsSize = sizeof(builtInCommands) / sizeof(builtInCommands[0]);

    // Loop through the built-in commands to check for a match.
    for (int i = 0; i < builtInCommandsSize; i++)
    {
        if (strcmp(command, builtInCommands[i]) == 0)
        {
            return true; // Command is built-in.
        }
    }

    return false; // Command is not built-in.
}

/**
 * Executes a series of commands connected by pipes, allowing the output of one command to serve as input to the next.
 * It sets up pipes between commands and forks processes to execute each command in the pipeline.
 *
 * @param commands Array of command strings that need to be executed in a pipeline.
 * @param num_cmds Number of commands in the commands array.
 * @param background Flag indicating whether the command should be executed in the background.
 */
void execute_piped_commands(char **commands, int num_cmds, int background)
{
    int pipefds[2 * (num_cmds - 1)]; // Array to store pipe file descriptors.
    pid_t pid;
    int fd_in = 0; // File descriptor for input redirection.

    // Setup pipes and fork processes for each command in the pipeline.
    for (int i = 0; i < num_cmds; ++i)
    {
        // Create pipes for all but the last command.
        if (i < num_cmds - 1)
        {
            if (pipe(pipefds + i * 2) < 0)
            {
                perror("Couldn't Pipe");
                exit(EXIT_FAILURE);
            }
        }

        pid = fork();
        if (pid == 0)
        { // Child process
            // Redirect input if necessary.
            if (fd_in != 0)
            {
                dup2(fd_in, STDIN_FILENO);
                close(fd_in);
            }
            // Redirect output for all but the last command.
            if (i < num_cmds - 1)
            {
                dup2(pipefds[i * 2 + 1], STDOUT_FILENO);
                close(pipefds[i * 2 + 1]);
            }

            // Close all other pipe ends in the child process.
            for (int j = 0; j < 2 * num_cmds - 2; j++)
            {
                close(pipefds[j]);
            }

            // Process the command and execute.
            char *argv[MAX_ARGS];
            process_input(commands[i], argv, &background);
            if (execvp(argv[0], argv) == -1)
            {
                perror("execvp");
                exit(EXIT_FAILURE);
            }
        }
        else if (pid < 0)
        { // Fork failed
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else
        { // Parent process
            // Close the input end of the previous pipe.
            if (fd_in != 0)
            {
                close(fd_in);
            }
            // Save the read end of the current pipe, if not the last command.
            if (i < num_cmds - 1)
            {
                close(pipefds[i * 2 + 1]);
                fd_in = pipefds[i * 2];
            }
        }
    }

    // Parent closes all pipes.
    for (int i = 0; i < 2 * num_cmds - 2; i++)
    {
        close(pipefds[i]);
    }

    // Wait for all child processes to finish if not running in the background.
    if (!background)
    {
        for (int i = 0; i < num_cmds; i++)
        {
            wait(NULL);
        }
    }
    else
    {
        // Print the PID of the last command executed in the background.
        printf("[PID %d running in background]\n", pid);
    }
}

/**
 * Prints the history of commands executed in the shell session up to the current moment.
 * This function iterates backward through the history array, displaying each command
 * along with its order number, starting from the most recent.
 */
void print_history()
{
    if (current_history_count == 0)
    {
        return; // Exit early if there are no commands in history.
    }
    for (int i = current_history_count - 1; i >= 0; i--)
    {
        printf("%d) %s\n", current_history_count - i, history[i]); // Print each command in reverse order.
    }
}

/**
 * Sets or updates a local variable within the shell's environment.
 * If a variable with the given name already exists, its value is updated.
 * Otherwise, a new variable is created.
 *
 * @param name The name of the variable to set or update.
 * @param value The value to assign to the variable.
 */
void set_local_var(char *name, char *value)
{
    // Search for existing variable with the given name.
    for (int i = 0; i < localVarCount; ++i)
    {
        if (strcmp(localVars[i].name, name) == 0)
        {
            strncpy(localVars[i].value, value, MAX_LINE_LENGTH); // Update existing variable.
            return;
        }
    }

    // Add new variable if not found.
    if (localVarCount < MAX_LOCAL_VARS)
    {
        strncpy(localVars[localVarCount].name, name, MAX_LINE_LENGTH);
        strncpy(localVars[localVarCount].value, value, MAX_LINE_LENGTH);
        localVarCount++; // Increment the count of local variables.
    }
}

/**
 * Substitutes the value of a variable into the given argument if the argument starts with '$'.
 * The function first looks for the variable in the environment variables; if not found, it checks the shell's local variables.
 * If the variable is not found or its value is empty, the argument is replaced with an empty string.
 *
 * @param arg Pointer to the string argument that potentially contains a variable to be substituted.
 */
void substitute_variable(char **arg)
{
    if ((*arg)[0] == '$')
    {
        char *varName = *arg + 1;         // Exclude '$' from variable name.
        char *varValue = getenv(varName); // Attempt to get value from environment variables.

        // Try local variables if environment variable not found.
        if (varValue == NULL)
        {
            for (int i = 0; i < localVarCount; i++)
            {
                if (strcmp(localVars[i].name, varName) == 0)
                {
                    varValue = localVars[i].value;
                    break; // Found the local variable.
                }
            }
        }

        // Substitute with empty string if variable not found or value is empty.
        if (varValue == NULL || strcmp(varValue, "") == 0)
        {
            *arg = strdup(""); // Replace with empty string.
        }
        else
        {
            *arg = strdup(varValue); // Substitute with variable's value.
        }
    }
}

/**
 * Iterates over all arguments in a command and substitutes any variables found.
 * This function uses substitute_variable() to process each argument individually.
 *
 * @param argv Array of string arguments representing the command and its parameters.
 */
void substitute_variables_in_command(char *argv[])
{
    for (int i = 0; argv[i] != NULL; i++)
    {
        substitute_variable(&argv[i]); // Apply variable substitution to each argument.
    }
}

/**
 * Changes the current working directory of the shell to the specified path.
 *
 * @param path The path to change the directory to. If the path is invalid, an error is reported.
 */
void cmd_cd(char *path)
{
    // Attempt to change the directory. On failure, report the error.
    if (chdir(path) != 0)
    {
        perror("cd failed");
        // Do not exit the program on failure, simply report the error.
    }
}

/**
 * Exits the shell program. If additional arguments are provided, an error is reported and the exit is aborted.
 *
 * @param argv Array of arguments passed to the exit command. Only the first argument (the command itself) is expected.
 */
void cmd_exit(char *argv[])
{
    // Check for additional arguments and report an error if found.
    if (argv[1] != NULL)
    {
        fprintf(stderr, "exit: does not take any arguments\n");
        return; // Return to continue the shell loop, do not exit.
    }
    else
    {
        exit(0); // Exit the shell with a normal exit status.
    }
}

/**
 * Controls the shell's command history. Can display the history, set its size, or execute a command from it.
 *
 * @param argv Array of arguments passed to the history command. Supports viewing, setting size, or executing commands.
 */
void cmd_history_control(char *argv[])
{
    // Display the command history if no additional arguments are provided.
    if (argv[1] == NULL)
    {
        print_history();
    }
    // Set the history size if 'set' command is provided with a valid size.
    else if (strcmp(argv[1], "set") == 0 && argv[2] != NULL)
    {
        int newSize = atoi(argv[2]); // Convert the size argument to an integer.
        set_history_size(newSize);   // Set the new history size.
    }
    // Execute a specific command from the history if a command number is provided.
    else
    {
        int command_number = atoi(argv[1]);      // Convert the command number argument to an integer.
        execute_history_command(command_number); // Execute the specified command.
    }
}

/**
 * Sets or unsets an environment variable.
 *
 * @param name The name of the environment variable to set or unset.
 * @param value The value to set the environment variable to, or NULL/empty string to unset.
 */
void cmd_export(char *name, char *value)
{
    // Validate the command usage.
    if (name == NULL)
    {
        fprintf(stderr, "Usage: export VAR=value\n");
        return;
    }

    // Unset the environment variable if the value is NULL or an empty string.
    if (value == NULL || strcmp(value, "") == 0)
    {
        if (unsetenv(name) != 0)
        {
            perror("unsetenv failed");
        }
    }
    else
    {
        // Set or update the environment variable with the provided value.
        if (setenv(name, value, 1) != 0)
        {
            perror("setenv failed");
        }
    }
}

/**
 * Sets or unsets a local (shell-specific) variable.
 *
 * @param name The name of the local variable to set or unset.
 * @param value The value to set the local variable to, or NULL/empty string to unset.
 */
void cmd_local(char *name, char *value)
{
    // Unset the local variable if the value is NULL or an empty string.
    if (value == NULL || strcmp(value, "") == 0)
    {
        // Find and remove the variable if it exists.
        int found = -1;
        for (int i = 0; i < localVarCount; i++)
        {
            if (strcmp(localVars[i].name, name) == 0)
            {
                found = i;
                break;
            }
        }
        if (found != -1)
        {
            // Remove the found variable by shifting subsequent variables up.
            for (int i = found; i < localVarCount - 1; i++)
            {
                localVars[i] = localVars[i + 1];
            }
            localVarCount--; // Decrement the count of local variables.
        }
    }
    else
    {
        // Set or update the local variable.
        int found = -1;
        for (int i = 0; i < localVarCount; i++)
        {
            if (strcmp(localVars[i].name, name) == 0)
            {
                found = i;
                break;
            }
        }
        if (found != -1)
        {
            // Update the value of an existing variable.
            strncpy(localVars[found].value, value, MAX_LINE_LENGTH - 1);
        }
        else
        {
            // Add a new local variable.
            if (localVarCount < MAX_LOCAL_VARS)
            {
                strncpy(localVars[localVarCount].name, name, MAX_LINE_LENGTH - 1);
                strncpy(localVars[localVarCount].value, value, MAX_LINE_LENGTH - 1);
                localVarCount++; // Increment the count of local variables.
            }
        }
    }
}

/**
 * Displays all local variables set within the shell.
 */
void cmd_vars()
{
    // Iterate through the list of local variables and print them.
    for (int i = 0; i < localVarCount; i++)
    {
        printf("%s=%s\n", localVars[i].name, localVars[i].value); // Print variable name and value.
    }
}
