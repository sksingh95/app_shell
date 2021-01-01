/*
 * main.c
 *
 *  Created on: 28-Jul-2019
 *      Author: Sonu
 */

#include <stdio.h>
#include <stdlib.h>
#include "app_shell.h"

static cmd_ret_t main_test_cmd(char ** args);

/* Main module command for testing. */
static cmd_t cmd_tbl[] = {
        {"mainTest", main_test_cmd, "main module."},
};

int main(){
    /* Initialize shell related data structure. 
     * It should be called before calling any other module related things. */
    shell_setup();
    register_module_cmd_tbl("Main", cmd_tbl, ARRAY_SIZE(cmd_tbl));
    
    /* Start the shell. It should be called very last, it runs in while(1) loop. */
    shell_run();
}

static cmd_ret_t main_test_cmd(char ** args){
    print_shell_resp("Hello from main module!!!! \n");
    return CMD_RUN_SUCCESS_E;
}
