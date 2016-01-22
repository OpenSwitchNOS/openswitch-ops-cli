extern void dyncb_helpstr_speeds(struct cmd_token *token, struct vty *vty, \
                                 char * const dyn_helpstr_ptr, int max_strlen);
extern void dyncb_helpstr_mtu(struct cmd_token *token, struct vty *vty, \
                              char * const helpstr, int max_strlen);

struct dyn_cb_func
{
  char * funcname;
  void (*funcptr)(struct cmd_token *token, struct vty *vty, \
                  char * const dyn_helpstr_ptr, int max_strlen);
};
/* callback func lookup table for dynamic helpstr */
struct dyn_cb_func dyn_cb_lookup[] =
{
  {"dyncb_helpstr_1G", dyncb_helpstr_speeds},
  {"dyncb_helpstr_10G", dyncb_helpstr_speeds},
  {"dyncb_helpstr_40G", dyncb_helpstr_speeds},
  {"dyncb_helpstr_mtu", dyncb_helpstr_mtu},
};
