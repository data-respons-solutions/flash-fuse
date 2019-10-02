#!/usr/bin/python3

import subprocess
import os
import sys
import argparse
from argparse import RawDescriptionHelpFormatter

fuse_path = '/sys/fsl_otp'

def read_fuse(name):
    with open(os.path.join(fuse_path, name), 'r') as f:
        return int(f.read(), base=16)
    
def write_fuse(name, mask):
    with open(os.path.join(fuse_path, name), 'w') as f:
        f.write(f'{hex(mask)}\n')

class CFG5_DIR_BT_DIS(object):
    CFG5_DIR_BT_DIS_mask = 0x00000008
    def is_fused(self):
        return (read_fuse('HW_OCOTP_CFG5') & self.CFG5_DIR_BT_DIS_mask)
    def blow(self, unused):
        write_fuse('HW_OCOTP_CFG5', self.CFG5_DIR_BT_DIS_mask)
    def get(self):
        return None
    
class EthernetMac(object):
    LOCK_MAC_ADDR_mask = 0x0300
    def is_fused(self):
        return (int(self.get().replace(':', ''), base=16))
    def get(self):    
        mac0 = f'{read_fuse("HW_OCOTP_MAC0"):0{8}x}'
        mac1 = f'{read_fuse("HW_OCOTP_MAC1"):0{8}x}'
        
        mac_list = [mac1[4:6], mac1[6:8], mac0[0:2], mac0[2:4], mac0[4:6], mac0[6:8]]
        return ':'.join(mac_list)
        return mac
    def blow(self, mac):
        if not len(mac.replace(':', '')) == 12:
            raise RuntimeError(f'MAC invalid: {mac} -- aborting')
        if (read_fuse('HW_OCOTP_LOCK') & self.LOCK_MAC_ADDR_mask):
            raise RuntimeError('MAC empty but fuse locked (HW_OCOTP_LOCK - 0x300')
        
        mac_list = mac.split(':')
        write_fuse('HW_OCOTP_MAC0', int('0x' +''.join(mac_list[2:6]), base=16))
        write_fuse('HW_OCOTP_MAC1', int('0x' + ''.join(mac_list[0:2]), base=16))
        write_fuse('HW_OCOTP_LOCK', self.LOCK_MAC_ADDR_mask) 
        
class JtagDisable(object):
    CFG5_SJC_DISABLE_mask = 0x00100000
    def is_fused(self):
        return (read_fuse('HW_OCOTP_CFG5') & self.CFG5_SJC_DISABLE_mask)
    def blow(self, unused):
        write_fuse('HW_OCOTP_CFG5', self.CFG5_SJC_DISABLE_mask)
    def get(self):
        None

fuse_obj_map = {
    'jtag-disable' : {
        'obj' : JtagDisable,
        'arg' : False,
    },
    'mac' : {
        'obj' : EthernetMac,
        'arg' : True,
    },
    'cfg5_dir_bit_ds' : {
        'obj' : CFG5_DIR_BT_DIS,
        'arg' : False,
    },
}

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='''Write IMX fuses.
!WARNING!

Changes are permanent and irreversible

!WARNING!
''',
                                     epilog='''Return value:
0 for success, 1 for failure                                
''',
                                     formatter_class=RawDescriptionHelpFormatter)
    parser.add_argument('--fuse', help='Target fuse')
    parser.add_argument('fuse_argument', nargs='?', default=None, help='Optional argument to target fuse')
    parser.add_argument('--fuse-path', help='Path to fsl_otp')
    parser.add_argument('--mac', help='Flash MAC, format xx:xx:xx:xx:xx:xx')
    parser.add_argument('--disable-jtag', action='store_true', help='Disable JTAG')
    parser.add_argument('--mandatory', action='store_true', help='Mandatory fuses')
    parser.add_argument('--commit', action='store_true', help='Allow burning fuses')
    parser.add_argument('--verify', action='store_true', help='Only verify, overrides --commit')
    args = parser.parse_args()
    
    if args.fuse_path:
        fuse_path = args.fuse_path
    
    if not args.fuse:
        print('--fuse not defined -- aborting', file=sys.stderr)
        sys.exit(1)
    
    if not fuse_obj_map[args.fuse]['arg']:
        args.fuse_argument = None
        
    fuse = fuse_obj_map[args.fuse]['obj']()
    fuse_arg_str = lambda x: f': {x}' if x else ''

    if args.commit and not args.verify:
        if fuse.is_fused():
            val = fuse.get()
            if val != args.fuse_argument:
                print(f'ERROR: {args.fuse}: already fused differently: {val} != {args.fuse_argument}', file=sys.stderr)
                sys.exit(1)
        else:
            print(f'{args.fuse}: fusing{fuse_arg_str(args.fuse_argument)}')
            fuse.blow(args.fuse_argument)
    else:
        if fuse.is_fused():
            print(f'{args.fuse}: already fused')
        else:
            print(f'{args.fuse}: not fused{fuse_arg_str(args.fuse_argument)}')
            sys.exit(1)
    
    sys.exit(0)
