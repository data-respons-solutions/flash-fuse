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
    Disabled JTAG byt setting BT_DIR_DIS in cfg5
    '''
    lockmask = 0x00100000
    fuselock = "0x00100008\n"
    
    def is_disabled(self):
        with open(os.path.join(fuse_path, 'HW_OCOTP_CFG5'), 'r') as f:
            cfg5 = f.read()
        return (int(cfg5, base=16) & self.lockmask)
        
    def disable(self):
        if self.is_disabled(): 
            raise RuntimeError('JTAG already disabled')
         
        with open(os.path.join(fuse_path, 'HW_OCOTP_CFG5'), 'w') as f:
            f.write(fuselock)

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
    args = parser.parse_args()
    if args.mac:
        if not verify_mac(args.mac):
            print(f'Ethernet MACs are always 12 hex characters, you entered {args.mac}', file=sys.stderr)
            sys.exit(1)
            
        mac = EthernetMac()
        if mac.is_fused():
            print(f'Ethernet MAC already fused: {mac.get()}', file=sys.stderr)
            sys.exit(1)
        print(f'Ethernet MAC: fusing: {args.mac}')
        mac.set(args.mac)
    
    if args.disable_jtag:
        jtag = Jtag()
        if jtag.is_disabled():
            print('JTAG is already disabled', file=sys.stderr)
            sys.exit(1)
        jtag.disable()

    sys.exit(0)
