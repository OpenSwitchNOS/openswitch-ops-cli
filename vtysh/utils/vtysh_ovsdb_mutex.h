/* To serialize updates to OVSDB.
 * interface threads calls to update OVSDB states. */
extern pthread_mutex_t vtysh_ovsdb_mutex;

/* Macros to lock and unlock mutexes in a verbose manner. */
#define VTYSH_OVSDB_LOCK { \
                VLOG_DBG("%s(%d): VTYSH_OVSDB_LOCK: taking lock...", __FUNCTION__, __LINE__); \
                pthread_mutex_lock(&vtysh_ovsdb_mutex); \
}

#define VTYSH_OVSDB_UNLOCK { \
                VLOG_DBG("%s(%d): VTYSH_OVSDB_UNLOCK: releasing lock...", __FUNCTION__, __LINE__); \
                pthread_mutex_unlock(&vtysh_ovsdb_mutex); \
}
