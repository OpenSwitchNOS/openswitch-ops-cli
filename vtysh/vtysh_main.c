/* Virtual terminal interface shell.
 * Copyright (C) 2000 Kunihiro Ishiguro
 * Copyright (C) 2015-2016 Hewlett Packard Enterprise Development LP
 *
 * This file is part of GNU Zebra.
 *
 * GNU Zebra is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * GNU Zebra is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Zebra; see the file COPYING.  If not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef ENABLE_OVSDB
#include <zebra.h>
#endif

#include <sys/un.h>
#include <setjmp.h>
#include <sys/wait.h>
#include <pwd.h>
#include <termios.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <lib/version.h>
#include <pthread.h>
#include <semaphore.h>
#include "getopt.h"
#include "command.h"
#include "memory.h"
#include "timeval.h"
#include <ltdl.h>
#include <libaudit.h>
#include "vtysh/vtysh.h"
#include "vtysh/vtysh_user.h"
#ifdef ENABLE_OVSDB
#include "openvswitch/vlog.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh_ovsdb_config.h"
#include "lib/lib_vtysh_ovsdb_if.h"
#include "openvswitch/vlog.h"
#include "vtysh/banner_vty.h"
#include "lib/cli_plugins.h"
#include "vtysh/utils/passwd_srv_utils.h"
#include "rbac.h"
#include "vtysh/utils/audit_log_utils.h"

#define FEATURES_CLI_PATH     "/usr/lib/cli/plugins"
VLOG_DEFINE_THIS_MODULE(vtysh_main);
#endif

#define AAA_UTILS_SO_PATH "/usr/lib/security/libpam_tacplus.so"

extern int64_t timeout_start;
extern struct termios tp;

/* tacacs command authorization function pointer */
int (*tac_cmd_author_ptr)(const char *, const char *,
                            const char *, char * , char *,
                            char *, char *, char  *,
                            int , bool , const char *,
                            const char *, const char * ) = NULL;

/* Return value of audit_open call. Use for subsequent audit call.*/
int audit_fd = 0;

/* VTY shell program name. */
char *progname;

/* Configuration file name and directory. */
char config_default[] = SYSCONFDIR VTYSH_DEFAULT_CONFIG;
char history_file[MAXPATHLEN];

/* Flag for indicate executing child command. */
int execute_flag = 0;

/* For sigsetjmp() & siglongjmp(). */
static sigjmp_buf jmpbuf;

/* Flag for avoid recursive siglongjmp() call. */
static int jmpflag = 0;

/* A static variable for holding the line. */
static char *line_read;

/* Master of threads. */
struct thread_master *master;

/* Command logging */
FILE *logfile;

#ifdef ENABLE_OVSDB
extern void reset_page_break_on_interrupt();
extern pthread_mutex_t vtysh_ovsdb_mutex;
#endif //ENABLE_OVSDB

/* SIGTSTP handler.  This function care user's ^Z input. */
void
sigtstp (int sig)
{

#ifdef ENABLE_OVSDB
  /* Tell any executing command to stop */
  vty_interrupted_flag_set(1);
  /* Reset the page_break settings to default */
  reset_page_break_on_interrupt();

  /* Release the lock, if command execution thread has taken it */
  pthread_mutex_unlock(&vtysh_ovsdb_mutex);
#endif //ENABLE_OVSDB

  /* Execute "end" command. */
  vtysh_execute ("end");

  /* Initialize readline. */
  rl_initialize ();
  printf ("\n");

  /* Check jmpflag for duplicate siglongjmp(). */
  if (! jmpflag)
    return;

  jmpflag = 0;

  /* Back to main command loop. */
  siglongjmp (jmpbuf, 1);
}

/* SIGINT handler.  This function care user's ^Z input.  */
void
sigint (int sig)
{

#ifdef ENABLE_OVSDB
  /* Tell any executing command to stop */
  vty_interrupted_flag_set(1);
  /* Reset the page_break settings to default */
  reset_page_break_on_interrupt();

  /* Release the lock, if command execution thread has taken it */
  pthread_mutex_unlock(&vtysh_ovsdb_mutex);
#endif //ENABLE_OVSDB

  /* Check this process is not child process. */
  if (! execute_flag)
    {
      rl_initialize ();
      printf ("\n");
      rl_forced_update_display ();
    }
}

/* Signale wrapper for vtysh. We don't use sigevent because
 * vtysh doesn't use threads. TODO */
RETSIGTYPE *
vtysh_signal_set (int signo, void (*func)(int))
{
  int ret;
  struct sigaction sig;
  struct sigaction osig;

  sig.sa_handler = func;
  sigemptyset (&sig.sa_mask);
  sig.sa_flags = 0;
#ifdef SA_RESTART
  sig.sa_flags |= SA_RESTART;
#endif /* SA_RESTART */

  ret = sigaction (signo, &sig, &osig);

  if (ret < 0)
    return (SIG_ERR);
  else
    return (osig.sa_handler);
}

/* Initialization of signal handles. */
void
vtysh_signal_init ()
{
  vtysh_signal_set (SIGINT, sigint);
  vtysh_signal_set (SIGTSTP, sigtstp);
  vtysh_signal_set (SIGPIPE, SIG_IGN);
  vtysh_signal_set (SIGQUIT, SIG_IGN);
}

/* Help information display. */
static void
usage (int status)
{
  if (status != 0)
    fprintf (stderr, "Try `%s --help' for more information.\n", progname);
  else
    printf ("Usage : %s [OPTION...]\n\n" \
	    "Integrated shell for Quagga routing software suite. \n\n" \
	    "-b, --boot               Execute boot startup configuration\n" \
	    "-c, --command            Execute argument as command\n" \
	    "-d, --daemon             Connect only to the specified daemon\n" \
	    "-E, --echo               Echo prompt and command in -c mode\n" \
	    "-C, --dryrun             Check configuration for validity and exit\n" \
	    "-h, --help               Display this help and exit\n\n" \
	    "Note that multiple commands may be executed from the command\n" \
	    "line by passing multiple -c args, or by embedding linefeed\n" \
	    "characters in one or more of the commands.\n\n", progname);

  exit (status);
}

/* VTY shell options, we use GNU getopt library. */
struct option longopts[] =
{
  { "boot",                 no_argument,             NULL, 'b'},
  /* For compatibility with older zebra/quagga versions */
  { "eval",                 required_argument,       NULL, 'e'},
  { "command",              required_argument,       NULL, 'c'},
#ifndef ENABLE_OVSDB
  { "daemon",               required_argument,       NULL, 'd'},
#endif
  { "echo",                 no_argument,             NULL, 'E'},
  { "dryrun",		    no_argument,	     NULL, 'C'},
  { "help",                 no_argument,             NULL, 'h'},
  { "noerror",		    no_argument,	     NULL, 'n'},
#ifdef ENABLE_OVSDB
  { "mininet-test",         no_argument,             NULL, 't'},
  { "verbose",              required_argument,       NULL, 'v'},
  { "temporary-db",         required_argument,       NULL, 'D'},
#endif
  { 0 }
};

/* Read a string, and return a pointer to it.  Returns NULL on EOF. */
char *
vtysh_rl_gets ()
{
  HIST_ENTRY *last;
  /* If the buffer has already been allocated, return the memory
   * to the free pool. */
  if (line_read)
    {
      free (line_read);
      line_read = NULL;
    }

  /* Timer start for idle session timeout. */
  timeout_start = time_now();
  /* Copying terminal settings to global variable. */
  tcgetattr(STDIN_FILENO, &tp);

  /* Get a line from the user.  Change prompt according to node.  XXX. */
  line_read = readline (vtysh_prompt ());

  /* If the line has any text in it, save it on the history. But only if
   * last command in history isn't the same one. */
  if (line_read && *line_read)
    {
      using_history();
      last = previous_history();
      if (!last || strcmp (last->line, line_read) != 0) {
	add_history (line_read);
	append_history(1,history_file);
      }
    }

  return (line_read);
}

static void log_it(const char *line)
{
  time_t t = time(NULL);
  struct tm *tmp = localtime(&t);
  char *user = getenv("USER") ? : "boot";
  char tod[64];

  strftime(tod, sizeof tod, "%Y%m%d-%H:%M.%S", tmp);

  fprintf(logfile, "%s:%s %s\n", tod, user, line);
}

static void tacacs_author_func_ptr_init()
{
  lt_dlhandle dhhandle = 0;
  lt_dlinit();
  lt_dlerror();

  dhhandle = lt_dlopen (AAA_UTILS_SO_PATH);
  if (lt_dlerror())
  {
    VLOG_ERR ("Failed to load the rbac library for sending command for authorization");
  }

  tac_cmd_author_ptr = lt_dlsym (dhhandle,"tac_cmd_author");
  if (tac_cmd_author_ptr == NULL)
  {
    VLOG_ERR ("Could not get the lock for %s", AAA_UTILS_SO_PATH);
  }
}

/* VTY shell main routine. */
int
main (int argc, char **argv, char **env)
{
  char *p;
  int opt;
  char *verbosity_arg = NULL;
  int dryrun = 0;
  int boot_flag = 0;
#ifndef ENABLE_OVSDB
  const char *daemon_name = NULL;
#endif
  struct cmd_rec {
    const char *line;
    struct cmd_rec *next;
  } *cmd = NULL;
  struct cmd_rec *tail = NULL;
  int echo_command = 0;
  int no_error = 0;
  int ret = 0;
  int counter=0;
  char *temp_db = NULL;
  pthread_t vtysh_ovsdb_if_thread;
  struct passwd *pw = NULL;

  /* set CONSOLE as OFF and SYSLOG as DBG for ops-cli VLOG moduler list.*/
  vlog_set_verbosity("CONSOLE:OFF");
  vlog_set_verbosity("SYSLOG:INFO");

  /* Initiate a connection to the audit framework for subsequent audit calls.*/
  audit_fd = audit_open();

  /* Preserve name of myself. */
  progname = ((p = strrchr (argv[0], '/')) ? ++p : argv[0]);

  /* if logging open now */
  if ((p = getenv("VTYSH_LOG")) != NULL)
      logfile = fopen(p, "a");

  /* Option handling. */
  while (1)
    {
#ifdef ENABLE_OVSDB
      opt = getopt_long (argc, argv, "be:c:d:nEhCtv:D:", longopts, 0);
#else
      opt = getopt_long (argc, argv, "be:c:nEhC", longopts, 0);
#endif

      if (opt == EOF)
	break;

      switch (opt)
	{
	case 0:
	  break;
	case 'b':
	  boot_flag = 1;
	  break;
	case 'e':
	case 'c':
	  {
	    struct cmd_rec *cr;
	    cr = XMALLOC(0, sizeof(*cr));
	    cr->line = optarg;
	    cr->next = NULL;
	    if (tail)
	      tail->next = cr;
	    else
	      cmd = cr;
	    tail = cr;
	  }
	  break;
#ifndef ENABLE_OVSDB
	case 'd':
	  daemon_name = optarg;
	  break;
#endif
	case 'n':
	  no_error = 1;
	  break;
	case 'E':
	  echo_command = 1;
	  break;
	case 'C':
	  dryrun = 1;
	  break;
	case 'h':
	  usage (0);
	  break;
#ifdef ENABLE_OVSDB
        case 't':
          enable_mininet_test_prompt = 1;
          break;
        case 'v':
          verbosity_arg = strdup(optarg);
          break;
        case 'D':
          temp_db = optarg;
          vtysh_show_startup = 1;
          break;
#endif
	default:
	  usage (1);
	  break;
	}
    }
  pw = getpwuid( getuid());
  if (pw == NULL)
  {
      fprintf(stderr,"Unknown User.\n");
      exit(1);
  }

  if (!rbac_is_user_permitted(pw->pw_name, VTY_SH))
  {
      const char *remote_user = getenv(REMOTE_USER_ENV);
      if (remote_user != NULL) {
          fprintf (stderr,
              "%s does not have the required permissions to access Vtysh.\n",
              remote_user);
      } else {
          fprintf (stderr,
              "%s does not have the required permissions to access Vtysh.\n",
              pw->pw_name);
      }
      exit(1);
  }
#ifdef ENABLE_OVSDB
  vtysh_ovsdb_init_clients();
  vtysh_ovsdb_init(argc, argv, temp_db);
  /* Make vty structure. */
  vty = vty_new ();
  vty->type = VTY_SHELL;
  vty->node = VIEW_NODE;
  cmd_init(0);
  plugins_cli_init(FEATURES_CLI_PATH);
  ret = pthread_create(&vtysh_ovsdb_if_thread,
                       (pthread_attr_t *)NULL,
                       vtysh_ovsdb_main_thread,
                       NULL);

  if (ret)
  {
      VLOG_ERR("Failed to create the poll thread %d",ret);
      exit(-ret);
  }
#endif

  /* Initialize user input buffer. */
  line_read = NULL;
  setlinebuf(stdout);

  /* Signal and others. */
  vtysh_signal_init ();

  /* Make vty structure and register commands. */
  vtysh_init_vty (pw);

  /* set CONSOLE as OFF */
  vlog_set_verbosity("CONSOLE:OFF");
  if (verbosity_arg) {
      vlog_set_verbosity(verbosity_arg);
      free(verbosity_arg);
  }
  else
      vlog_set_verbosity("SYSLOG:INFO");

  vtysh_user_init ();
  vtysh_config_init ();

  /* parse yaml file to be used by 'password' command */
  passwd_srv_path_manager_init();

  vty_init_vtysh ();

  /* Read vtysh configuration file before connecting to daemons. */
  vtysh_read_config (config_default);

  /* Start execution only if not in dry-run mode */
  if(dryrun)
    return(0);

  /* Ignore error messages */
  if (no_error)
    freopen("/dev/null", "w", stdout);

  /* Make sure we pass authentication before proceeding. */
  vtysh_auth ();

#ifndef ENABLE_OVSDB
  /* Do not connect until we have passed authentication. */
  if (vtysh_connect_all (daemon_name) <= 0)
    {
      fprintf(stderr, "Exiting: failed to connect to any daemons.\n");
      exit(1);
    }
#endif

  /* If eval mode. */
  if (cmd)
    {
      /* Wait for idl sequence number */
      do
      {
	 if(vtysh_ovsdb_is_loaded())
         {
            break;
         }
         sleep(1);
         counter++;
      }while(counter < MAX_TIMEOUT_FOR_IDL_CHANGE);

      /* Enter into enable node. */
      vtysh_execute ("enable");

      while (cmd != NULL)
        {
	  int ret;
	  char *eol;

	  while ((eol = strchr(cmd->line, '\n')) != NULL)
	    {
	      *eol = '\0';

	      if (echo_command)
		printf("%s%s\n", vtysh_prompt(), cmd->line);

	      if (logfile)
		log_it(cmd->line);

	      ret = vtysh_execute_no_pager(cmd->line);
	      if (!no_error &&
		  ! (ret == CMD_SUCCESS ||
		     ret == CMD_SUCCESS_DAEMON ||
		     ret == CMD_WARNING))
		exit(1);

	      cmd->line = eol+1;
	    }

	  if (echo_command)
	    printf("%s%s\n", vtysh_prompt(), cmd->line);

	  if (logfile)
	    log_it(cmd->line);

	  ret = vtysh_execute_no_pager(cmd->line);
	  if (!no_error &&
	      ! (ret == CMD_SUCCESS ||
		 ret == CMD_SUCCESS_DAEMON ||
		 ret == CMD_WARNING))
	    exit(1);

	  {
	    struct cmd_rec *cr;
	    cr = cmd;
	    cmd = cmd->next;
	    XFREE(0, cr);
	  }
        }
      exit (0);
    }

  /* Boot startup configuration file. */
  if (boot_flag)
    {
      if (vtysh_read_config (integrate_default))
	{
	  fprintf (stderr, "Can't open configuration file [%s]\n",
		   integrate_default);
	  exit (1);
	}
      else
	exit (0);
    }

  vtysh_pager_init ();

  vtysh_readline_init ();
/* Welcome Banner of vtysh */
#ifndef ENABLE_OVSDB
  vty_hello (vty);
#endif
  /* Enter into enable node. */
  vtysh_execute ("enable");

  /* Preparation for longjmp() in sigtstp(). */
  sigsetjmp (jmpbuf, 1);
  jmpflag = 1;

  snprintf(history_file, sizeof(history_file), "%s/.history_quagga", getenv("HOME"));
  read_history(history_file);

#ifdef ENABLE_OVSDB
  /*
   * Wait for  ovsdb to be loaded. If ovsdb is not ready and user tries to configure,
   * commands will fail to execute. So, wait for idl sequence number to change which
   * indicates OVSDB is ready for transactions.
   */
  counter = 0;
  do
  {
    if (vtysh_ovsdb_is_loaded())
    {
        break;
    }
    usleep(500000); //sleep for 500 msec
    counter++;
  } while (counter < MAX_TIMEOUT_FOR_IDL_CHANGE);
#endif

  ospf_area_vlink_init();

  tacacs_author_func_ptr_init();

  /* Main command loop. */
  while (vtysh_rl_gets ())
    vtysh_execute (line_read);

  history_truncate_file(history_file,1000);
  printf ("\n");
#ifdef ENABLE_OVSDB
  vtysh_ovsdb_exit();
#endif

  /* Rest in peace. */
  exit (0);
}
