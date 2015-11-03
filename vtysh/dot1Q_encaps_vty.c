#include <zebra.h>
#include "vty.h"
#include <vector.h>
#include "vswitch-idl.h"
#include "ovsdb-idl.h"
#include "vtysh_ovsdb_if.h"
#include "vtysh_ovsdb_config.h"
#include "vlan_vty.h"
#include "vrf_vty.h"
#include "dot1Q_encaps_vty.h"

VLOG_DEFINE_THIS_MODULE(vtysh_dot1Q_cli);
extern struct ovsdb_idl *idl;

/*vlan envapsulation configuration*/
DEFUN  (cli_encapsulation_dot1Q_vlan,
        cli_encapsulation_dot1Q_vlan_cmd,
        "encapsulation dot1Q <1-4094>",
        "Set encapsulation type for an interface\n"
	"IEEE 802.1Q Virtual LAN\n"
        "VLAN identifier\n")
{
    const struct ovsrec_port *port_row = NULL;
    const struct ovsrec_port *vlan_port_row = NULL;
    const struct ovsrec_interface *intf_row = NULL;
    const struct ovsrec_vlan *vlan_row = NULL;
    struct ovsdb_idl_txn *status_txn = cli_do_config_start();
    enum ovsdb_idl_txn_status status;
    int vlan_id = atoi((char *) argv[0]);
    int i = 0, found_vlan = 0;

    if (NULL == status_txn)
    {
        VLOG_ERR("Failed to create transaction. Function:%s, Line:%d", __func__, __LINE__);
        cli_do_config_abort(status_txn);
        vty_out(vty, OVSDB_INTF_VLAN_TRUNK_ALLOWED_ERROR, vlan_id, VTY_NEWLINE);
        return CMD_SUCCESS;
    }

    char *ifname = (char *) vty->index;
    OVSREC_INTERFACE_FOR_EACH(intf_row, idl)
    {
        if (strcmp(intf_row->name, ifname) == 0)
        {
            break;
        }
    }

    port_row = ovsrec_port_first(idl);
    if (NULL == port_row)
    {
        vlan_port_row = port_check_and_add(ifname, true, true, status_txn);
    }
    else
    {
        OVSREC_PORT_FOR_EACH(port_row, idl)
        {
            for (i = 0; i < port_row->n_interfaces; i++)
            {
                if (port_row->interfaces[i] == intf_row)
                {
                    if (strcmp(port_row->name, ifname) != 0)
                    {
                        vty_out(vty, "Can't configure VLAN, %s.%s", port_row->name, VTY_NEWLINE);
                        cli_do_config_abort(status_txn);
                        return CMD_SUCCESS;
                    }
                    else
                    {
                        vlan_port_row = port_row;
                        break;
                    }
                }
            }
        }
    }

    if (NULL == vlan_port_row )
    {
        vlan_port_row = port_check_and_add(ifname, true, true, status_txn);
    }

    if (NULL == vlan_port_row->vlan_mode)
    {
        ovsrec_port_set_vlan_mode(vlan_port_row, OVSREC_PORT_VLAN_MODE_TRUNK);
    }
    else if (strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_ACCESS) == 0)
    {
        vty_out(vty, "The interface is in access mode.%s", VTY_NEWLINE);
        cli_do_config_abort(status_txn);
        return CMD_SUCCESS;
    }
    else if (strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_NATIVE_TAGGED) != 0 &&
        strcmp(vlan_port_row->vlan_mode, OVSREC_PORT_VLAN_MODE_NATIVE_UNTAGGED) != 0)
    {
        ovsrec_port_set_vlan_mode(vlan_port_row, OVSREC_PORT_VLAN_MODE_TRUNK);
    }

    int64_t* trunks = NULL;
    trunks = xmalloc(sizeof *vlan_port_row->trunks * (vlan_port_row->n_trunks + 1));
    for (i = 0; i < vlan_port_row->n_trunks; i++)
    {
        trunks[i] = vlan_port_row->trunks[i];
    }
    trunks[vlan_port_row->n_trunks] = vlan_id;
    int trunk_count = vlan_port_row->n_trunks + 1;
    ovsrec_port_set_trunks(vlan_port_row, trunks, trunk_count);

    status = cli_do_config_finish(status_txn);
    free(trunks);

    if (status == TXN_SUCCESS || status == TXN_UNCHANGED)
    {
        return CMD_SUCCESS;
    }
    else
    {
        vty_out(vty, OVSDB_INTF_VLAN_TRUNK_ALLOWED_ERROR, vlan_id, VTY_NEWLINE);
        return CMD_SUCCESS;
    }
}

void encapsulation_vty_init(void) {
    install_element (SUB_INTERFACE_NODE,&cli_encapsulation_dot1Q_vlan_cmd);
}
