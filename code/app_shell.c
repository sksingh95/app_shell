/*
 * app_shell.c
 *
 *  Created on: 28-Jul-2019
 *      Author: Sonu
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "app_shell.h"


#define LIST_ITERATOR(sll_head_ptr, tmp) 	\
		tmp = sll_head_ptr->next;			\
		for(; tmp!=NULL; tmp=tmp->next)

/* Global variables declaration*/
struct {
    char cmd_buffer[CMD_LENGTH_IN_BYTE];          /* Command string from user. */
    char *cmd_arg_ptr[CMD_ARG_SIZE];              /* Pointer for command name and arguments. */
    cmd_cb_fptr fptrCmdCb;
}CurrentCmd;

/* This table must be in same order as enum cmd_ret_t. */
char * cmd_ret_str[] = {
	"CMD_PARSE_SUCCESS",
	"CMD_FOUND",
	"CMD_RUN_SUCCESS",
	"CMD_PARSE_FAIL",
	"CMD_RUN_FAIL",
	"CMD_PARAM_ERR",
	"CMD_NOT_FOUND",
	"CMD_MAX_NUM_ARG_ERR"
};

static cmd_list_node_t *ptrCmdTableListHead;
static char in_prompt[10];
static char out_prompt[10];
static char shell_resp_buf[SHELL_RESP_BUFF_IN_BYTE];     /* Shell response. */

/* Global function declaration */
static cmd_list_node_t * cmd_list_node_sll_init();
static char *read_cmd_from_stdin(void);
static cmd_ret_t parse_cmd(char *line_ptr);
static cmd_ret_t execute_cmd();
static cmd_ret_t search_cmd_in_list(char *cmd, cmd_cb_fptr *cmd_cb);

/* Shell related command function */
static cmd_ret_t help_cmd(char **cmd_arg_ptr);
static cmd_ret_t version_cmd(char **cmd_arg_ptr);
static cmd_ret_t exit_cmd(char **cmd_arg_ptr);
static cmd_ret_t mod_list_cmd(char **cmd_arg_ptr);

cmd_t cmd_table[] = {
		{"help",              help_cmd,       "Show list of available commands."},
		{"version",           version_cmd,    "Version of shell."},
		{"exit",              exit_cmd,       "Terminate sh."},
		{"modList",           mod_list_cmd,   "List of module registered with shell."}
};

void shell_setup(){
	/*
	 * Initialize the shell
	 */
	ptrCmdTableListHead = cmd_list_node_sll_init();  /* Create dummy head node */
	strcpy(in_prompt, "IP"SHELL_USR_MODE_STR);
	strcpy(out_prompt, "OP"SHELL_USR_MODE_STR);

	register_module_cmd_tbl("App Shell", cmd_table, ARRAY_SIZE(cmd_table));
}

void shell_run(){
	char *line_ptr = NULL;
	cmd_ret_t cmd_parse_ret = CMD_PARSE_FAIL_E;
	int ret = 0;

	/*
	 * Interpret the shell (steps)
	 * 	- Read    : read the command from stdin
	 * 	- Parse   : separate command & args. Search for command.
	 * 	- Execute : run the command
	 */
	while(1) {
		printf("%s",in_prompt);
		line_ptr = read_cmd_from_stdin();
		if(line_ptr == NULL) {
			printf("Unable to read from stdin !!!\n");
			exit(1);
		}
		else if(line_ptr[0] == '\0') {
			continue;
		}
		printf("%s \n",out_prompt);

		cmd_parse_ret = processCommand(line_ptr);  /* Will parse and execute command. */
	}
}

static cmd_list_node_t * cmd_list_node_sll_init() {
	cmd_list_node_t *temp;
	temp                = (cmd_list_node_t*) malloc(sizeof(cmd_list_node_t));
	temp->next          = NULL;
	temp->cmd_list_size = 0;
	temp->mod_name      = NULL;
	temp->cmd_list      = NULL;
	return temp;
}

char *read_cmd_from_stdin(void) {
	int index         = 0;
	int ch            = 0;
	char * cmd_buffer = CurrentCmd.cmd_buffer;

	memset(cmd_buffer, 0, sizeof(CurrentCmd.cmd_buffer));

	while(1) {
		ch = getchar();
		switch(ch) {
		case EOF:
			printf("EOF : Ctrl-D Pressed. Exiting ...\n");
			exit(1);
		case '\n':
			cmd_buffer[index] = '\0';
			return cmd_buffer;
		default:
			if(index < CMD_LENGTH_IN_BYTE) {
				cmd_buffer[index++] = ch;
			}
			else {
				cmd_buffer[0] = '\0';
				printf("Buffer full. command too long.\n");
				while ((ch = getchar()) != '\n' && ch != EOF); /* flush remaining char in buffer */
				return cmd_buffer;
			}
			break;
		}
	}
}

cmd_ret_t processCommand(char *pCmdLine) {
	cmd_ret_t ret = CMD_PARSE_FAIL_E;
	
	ret = parse_cmd(pCmdLine);
	if(CMD_PARSE_SUCCESS_E == ret) {
		ret = execute_cmd();
	}
	
	print_shell_resp("\n");
	print_shell_resp("%s %s\n", CurrentCmd.cmd_arg_ptr[0], cmd_ret_str[ret]);
	return ret;
}

cmd_ret_t parse_cmd(char *line_ptr) {
	char *token = NULL;
	int index = 0;
	cmd_ret_t ret_value = CMD_PARSE_SUCCESS_E;
	char ** cmd_arg_ptr = CurrentCmd.cmd_arg_ptr;
	cmd_cb_fptr cmd_cb ;

	token = strtok(line_ptr, " ");
	while(token != NULL) {
		if(index < CMD_ARG_SIZE) {
			cmd_arg_ptr[index++] = token;
		}
		else {
			return CMD_MAX_NUM_ARG_ERR_E;
		}
		token = strtok(NULL, " ");
	}
	cmd_arg_ptr[index] = NULL;

	for(int i=0; i<strlen(line_ptr); i++) {
		if(line_ptr[i] == ' '){
			line_ptr[i] = '\0';
		}
	}
	
	ret_value = search_cmd_in_list(*cmd_arg_ptr, &cmd_cb);
	if(ret_value == CMD_FOUND_E) {
	    CurrentCmd.fptrCmdCb = cmd_cb;
	}
	else {
		return CMD_NOT_FOUND_E;
	}
	
	return CMD_PARSE_SUCCESS_E;
}

cmd_ret_t execute_cmd() {
	int i=0;
	char cmd_name[CMD_LENGTH_IN_BYTE] = {'\0'};
	
	char ** cmd_arg_ptr = CurrentCmd.cmd_arg_ptr;
	assert(CurrentCmd.fptrCmdCb);
	
	strcat(cmd_name, "Running... ");
	for(i=0; *(cmd_arg_ptr+i) != NULL; i++) {
		strcat(cmd_name, " ");
		strcat(cmd_name, *(cmd_arg_ptr+i));
	}
	strcat(cmd_name, "\n");
	print_shell_resp(cmd_name);

	return CurrentCmd.fptrCmdCb(cmd_arg_ptr); // Execute command here.
}

cmd_ret_t search_cmd_in_list(char *cmd, cmd_cb_fptr *cmd_cb) {
	int i = 0;
	cmd_list_node_t *temp;
	LIST_ITERATOR(ptrCmdTableListHead, temp) {
		for(i=0; i < (temp->cmd_list_size); i++) {
			if(strcasecmp(cmd, temp->cmd_list[i].cmd_name) == 0) {	
				*cmd_cb = temp->cmd_list[i].cmd_cb;				    /* return reference to callback if found */
				return CMD_FOUND_E;
			}
		}
	}
	return CMD_NOT_FOUND_E;
}

int register_module_cmd_tbl(char * module_name, cmd_t cmd_list[], int cmd_list_size) {
	
	cmd_list_node_t* ptr_temp = NULL;
	
	cmd_list_node_t* cmd_list_node = (cmd_list_node_t*) malloc(sizeof(cmd_list_node_t));
	assert(cmd_list_node != NULL);                        /* Assert when memory is not allocated. */
	
	cmd_list_node->cmd_list      = cmd_list;
	cmd_list_node->cmd_list_size = cmd_list_size;
	cmd_list_node->mod_name      = module_name;
	cmd_list_node->next          = NULL;
	
	/* Add at the end of list */
	ptr_temp = ptrCmdTableListHead;
	while(ptr_temp->next != NULL) ptr_temp = ptr_temp->next;
	ptr_temp->next = cmd_list_node;

	return 0;
}

/* Separate print function to print all shell related msg in same format. */
int print_shell_resp(char *format, ...) {
    va_list ap;
	char *c;

    va_start(ap, format);
    memset(shell_resp_buf, '\0', sizeof(shell_resp_buf));
    sprintf(shell_resp_buf, "         %s",format);
	vprintf(shell_resp_buf, ap);
    return 0;
}

static cmd_ret_t mod_list_cmd(char **cmd_arg_ptr){
	cmd_list_node_t *temp;

	LIST_ITERATOR(ptrCmdTableListHead, temp){
		print_shell_resp("%s\n", temp->mod_name);
	}
	
	return CMD_RUN_SUCCESS_E;
}

static cmd_ret_t help_cmd(char **cmd_arg_ptr){
	cmd_list_node_t *temp;
	int i=0;
	LIST_ITERATOR(ptrCmdTableListHead, temp){
		print_shell_resp("---%s---\n", temp->mod_name);
			for(i=0;  i < (temp->cmd_list_size); i++){
				print_shell_resp("%s\n", temp->cmd_list[i].cmd_name);
			}
			print_shell_resp("\n");
	}
	return CMD_RUN_SUCCESS_E;
}

static cmd_ret_t version_cmd(char **cmd_arg_ptr) {
	print_shell_resp("Version 1.0.0 \n");
	print_shell_resp("Author : Sonu\n");
	return CMD_RUN_SUCCESS_E;
}

static cmd_ret_t exit_cmd(char **cmd_arg_ptr) {
	print_shell_resp("Exiting...\n");
	exit(0);
}
