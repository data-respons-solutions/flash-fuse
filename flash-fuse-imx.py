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
    LOCK_MAC_ADDR_mask = 0x0300
    
    def is_fused(self):
        return (int(self.get().replace(':', ''), base=16))
    
    def get(self):    
        mac0 = f'{read_fuse("HW_OCOTP_MAC0"):0{8}x}'
        mac1 = f'{read_fuse("HW_OCOTP_MAC1"):0{8}x}'
        
        mac_list = [mac1[4:6], mac1[6:8], mac0[0:2], mac0[2:4], mac0[4:6], mac0[6:8]]
        return ':'.join(mac_list)
        return mac
    
    def set(self, mac):
        if self.is_fused():
            raise RuntimeError('MAC already fused -- aborting')
        
        if not verify_mac(mac):
            raise RuntimeError(f'MAC invalid: {mac} -- aborting')
        
        if (read_fuse('HW_OCOTP_LOCK') & self.LOCK_MAC_ADDR_mask):
            raise RuntimeError('MAC empty but fuse locked (HW_OCOTP_LOCK - 0x300')
        
        mac_list = mac.split(':')
        write_fuse('HW_OCOTP_MAC0', int('0x' +''.join(mac_list[2:6]), base=16))
        write_fuse('HW_OCOTP_MAC1', int('0x' + ''.join(mac_list[0:2]), base=16))
        write_fuse('HW_OCOTP_LOCK', self.LOCK_MAC_ADDR_mask) 

class Jtag(object):
    '''
    Disabled JTAG byt setting SJC_DISABLE in CFG5
    '''
    CFG5_SJC_DISABLE_mask = 0x00100000
    
    def is_disabled(self):
        return (read_fuse('HW_OCOTP_CFG5') & self.CFG5_SJC_DISABLE_mask)
    
    def disable(self):
        write_fuse('HW_OCOTP_CFG5', self.CFG5_SJC_DISABLE_mask)

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
    parser.add_argument('--fuse-path', help='Path to fsl_otp')
    parser.add_argument('--mac', help='Flash MAC, format xx:xx:xx:xx:xx:xx')
    parser.add_argument('--disable-jtag', action='store_true', help='Disable JTAG')
    parser.add_argument('--mandatory', action='store_true', help='Mandatory fuses')
    parser.add_argument('--commit', action='store_true', help='Allow burning fuses')
    args = parser.parse_args()
    
    if args.fuse_path:
        fuse_path = args.fuse_path
    
    if args.mac:
        if not verify_mac(args.mac):
            print(f'Ethernet MACs are always 12 hex characters, you entered {args.mac}', file=sys.stderr)
            sys.exit(1)
            
        mac = EthernetMac()    
        if mac.is_fused():
            fused_mac = mac.get()
            if fused_mac == args.mac:
                print('Ethernet MAC: already fused')
            else:
                print(f'ERROR: Ethernet MAC alreadY fused: {fused_mac} != {args.mac}', file=sys.stderr)
                sys.exit(1) 
        else:
            if args.commit:
                print(f'Ethernet MAC: fusing: {args.mac}')
                mac.set(args.mac)
            else:
                sys.exit(1)
            
    
    if args.disable_jtag:
        jtag = Jtag()
        if jtag.is_disabled():
            print('JTAG: already disabled')
        else:
            if args.commit:
                print('JTAG: disabling')
                jtag.disable()
            else:
                print('ERROR: JTAG: not disabled', file=sys.stderr)
                sys.exit(1)
        
    if args.mandatory:
        cfg5_dir_bt_dis = CFG5_DIR_BT_DIS()  
        if cfg5_dir_bt_dis.is_fused():
            print('CFG5: DIR_BT_DIS: already fused')
        else:
            if args.commit:
                print('CFG5: DIR_BT_DIS: fusing')
                cfg5_dir_bt_dis.fuse()
            else:
                print('ERROR: CFG5: DIR_BT_DIS: not fused')
                sys.exit(1)

    sys.exit(0)
