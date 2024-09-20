# GDB script to get lvgl global pointer in NuttX.

import argparse
import sys
from os import path

import gdb
from nuttxgdb import utils

# Add current script folder so we can import module lvgl


if __name__ == "__main__":
    here = path.dirname(path.abspath(__file__))
    if here not in sys.path:
        sys.path.insert(0, here)

from lvgl import set_lvgl_instance


class Lvglobal(gdb.Command):
    """Set which lvgl instance to inspect by finding lv_global pointer in task TLS data."""

    def __init__(self):
        super(Lvglobal, self).__init__("lvglobal", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        parser = argparse.ArgumentParser(description=self.__doc__)
        parser.add_argument("-p", "--pid", type=int, help="Optional process ID")
        try:
            args = parser.parse_args(gdb.string_to_argv(arg))
        except SystemExit:
            return

        lv_global = utils.gdb_eval_or_none("lv_global")
        if lv_global:
            gdb.write(f"Found single instance lv_global@{hex(lv_global.address)}\n")
            set_lvgl_instance(lv_global)
            return

        # find the lvgl global pointer in tls
        if not args.pid:
            gdb.write("Lvgl is in multi-process mode, need pid argument.\n")
            return
        lv_key = utils.gdb_eval_or_none("lv_nuttx_tlskey")
        if lv_key is None:
            gdb.write("Can't find lvgl tls key in multi-process mode.\n")
            return
        lv_global = utils.get_task_tls(args.pid, lv_key)
        if lv_global:
            gdb.write(f"Found lv_global@{hex(lv_global)}\n")
            set_lvgl_instance(lv_global)
        else:
            gdb.write("\nCan't find lv_global in tls.\n")
        gdb.write("\n")


Lvglobal()
