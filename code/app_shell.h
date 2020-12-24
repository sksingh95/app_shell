/*
 * app_shell.h
 *
 *  Created on: 28-Jul-2019
 *      Author: Sonu
 */

#ifndef _APP_SHELL_H_
#define _APP_SHELL_H_

#include <string.h>

#define ARRAY_SIZE(Arr)		    (sizeof(Arr)/sizeof(Arr[0]))

#define SHELL_USR_MODE_STR		"<usr>$ "   /* Promt to show on console */
#define CMD_LENGTH_IN_BYTE		100			/* Total char in command including args */
#define CMD_ARG_SIZE			20          /* Max number of argument in a command. */
#define SHELL_RESP_BUFF_IN_BYTE 100


typedef enum {
	CMD_PARSE_SUCCESS_E,
	CMD_FOUND_E,
	CMD_RUN_SUCCESS_E,
	CMD_PARSE_FAIL_E,
	CMD_RUN_FAIL_E,
	CMD_PARAM_ERR_E,
	CMD_NOT_FOUND_E,
	CMD_MAX_NUM_ARG_ERR_E,
}cmd_ret_t;

/* This table must be in same order as above table in c file . */
extern char * cmd_ret_str[];

typedef cmd_ret_t (*cmd_cb_fptr)(char **);		  /* command function pointer */

/* struct to map command name to its function */
typedef struct {
	char         cmd_name[CMD_LENGTH_IN_BYTE];    /* Name of command */
	cmd_cb_fptr  cmd_cb;                          /* Function that run when command is triggered. */
	char *       short_desc;                      /* Short description of command. */
	char *       syntax;                          /* Syntax for the command. */
}cmd_t;

typedef struct cmd_list_node {
	struct cmd_list_node * next;
	char *                 mod_name;              /* Module name */
	cmd_t *                cmd_list;              /* List of command supported by module. */
	int                    cmd_list_size;         /* Numbers of command in module.*/
} cmd_list_node_t;

/* Public variable declaration

/* Public function declaration */
int register_module_cmd_tbl(char * module_name, cmd_t cmd_list[], int cmd_list_size); /* for registering new command list with shell. */
cmd_ret_t processCommand(char *cmdLine);
int print_shell_resp(char *format, ...);
void shell_setup(void);
void shell_run(void);

#endif /* _APP_SHELL_H_ */
