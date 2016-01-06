#ifndef CLI_PLUGINS_H
#define CLI_PLUGINS_H 1

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef __linux__
void plugins_cli_init(const char *path);
#endif

#ifdef  __cplusplus
}
#endif

#endif /* cli_plugins.h */
