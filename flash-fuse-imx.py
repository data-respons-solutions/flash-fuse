#!/usr/bin/python3

import subprocess
import os
import sys
import argparse
from argparse import RawDescriptionHelpFormatter

fuse_path = '/sys/fsl_otp'

def verify_mac(mac):
    s = mac.replace(':','')
    return len(s) == 12   

def read_fuse(name):
    with open(os.path.join(fuse_path, name), 'r') as f:
        return int(f.read(), base=16)
    
def write_fuse(name, mask):
    with open(os.path.join(fuse_path, name), 'w') as f:
        f.write(f'{hex(mask)}\n')

class CFG5_DIR_BT_DIS(object):
    '''
    DIR_BT_DIS in CFG5 must be blown to 1 for normal operation.
    ''' 
    
    CFG5_DIR_BT_DIS_mask = 0x00000008

    def is_fused(self):
        return (read_fuse('HW_OCOTP_CFG5') & self.CFG5_DIR_BT_DIS_mask)
    
    def fuse(self):
        write_fuse('HW_OCOTP_CFG5', self.CFG5_DIR_BT_DIS_mask)
        
class EthernetMac(object):
    fuselock = "0x0300\n"
    
    def is_fused(self):
        return (int(self.get().replace(':', ''), base=16))
    
    def get(self):
        with open(os.path.join(fuse_path, 'HW_OCOTP_MAC0'), 'r') as f:
            mac0 = f.read().strip()[2:].zfill(8)
    
        with open(os.path.join(fuse_path, 'HW_OCOTP_MAC1'), 'r') as f:
            mac1 = f.read().strip()[2:].zfill(4)
        
        mac_list = [mac1[0:2], mac1[2:4], mac0[0:2], mac0[2:4], mac0[4:6], mac0[6:8]]
        return ':'.join(mac_list)
        return mac
    
    def set(self, mac):
        if self.is_fused():
            raise RuntimeError('MAC already fused -- aborting')
        
        if not verify_mac(mac):
            raise RuntimeError(f'MAC invalid: {mac} -- aborting')
        
        mac_list = mac.split(':')
        mac0 = '0x' + ''.join(mac_list[2:6]) + '\n'
        mac1 = '0x' + ''.join(mac_list[0:2]) + '\n'
        
        with open(os.path.join(fuse_path, 'HW_OCOTP_MAC0'), 'w') as f:
            f.write(mac0)
        with open(os.path.join(fuse_path, 'HW_OCOTP_MAC1'), 'w') as f:
            f.write(mac1)
        with open(os.path.join(fuse_path, 'HW_OCOTP_LOCK'), 'w') as f:
            f.write(self.fuselock)

class Jtag(object):
    '''
    Disabled JTAG byt setting SJC_DISABLE in CFG5
    '''
    CFG5_SJC_DISABLE = 0x00100000
    
    def is_disabled(self):
        return (read_fuse('HW_OCOTP_CFG5') & self.CFG5_SJC_DISABLE)
    
    def disable(self):
        write_fuse('HW_OCOTP_CFG5', self.CFG5_SJC_DISABLE)

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
    parser.add_argument('--mac', help='Flash MAC, format xx:xx:xx:xx:xx:xx')
    parser.add_argument('--disable-jtag', action='store_true', help='Disable JTAG')
    parser.add_argument('--mandatory', action='store_true', help='Mandatory fuses')
    args = parser.parse_args()
    if args.mac:
        if not verify_mac(args.mac):
            print(f'Ethernet MACs are always 12 hex characters, you entered {args.mac}', file=sys.stderr)
            sys.exit(1)
            
        mac = EthernetMac()
        if mac.is_fused():
            print(f'Ethernet MAC already fused: {mac.get()}', file=sys.stderr)
        else:
            print(f'Ethernet MAC: fusing: {args.mac}')
            mac.set(args.mac)
    
    if args.disable_jtag:
        jtag = Jtag()
        if jtag.is_disabled():
            print('JTAG: already disabled')
        else:
            print('JTAG: disabling')
            jtag.disable()
        
    if args.mandatory:
        cfg5_dir_bt_dis = CFG5_DIR_BT_DIS()  
        if cfg5_dir_bt_dis.is_fused():
            print('CFG5: DIR_BT_DIS: already fused')
        else:
            print('CFG5: DIR_BT_DIS: fusing')
            cfg5_dir_bt_dis.fuse()

    sys.exit(0)
