#ifndef PLUGINS_H
#define PLUGINS_H 1

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef __linux__
void plugins_cli_init(const char *path);
#else
#define plugins_mgmt_init(path)
#endif

#ifdef  __cplusplus
}
#endif

#endif /* cli_plugins.h */
