#!/usr/bin/python3

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
    MASK = 0x00000008
    OCOTP = 'HW_OCOTP_CFG5'
    def is_fused(self):
        return (read_fuse(self.OCOTP) & self.MASK == self.MASK)
    def blow(self, unused):
        write_fuse(self.OCOTP, self.MASK)
    def get(self):
        return None
    
class CFG5_BT_FUSE_SEL(CFG5_DIR_BT_DIS):
    MASK = 0x00000010
    OCOTP = 'HW_OCOTP_CFG5'
    
class CFG5_SJC_DISABLE(CFG5_DIR_BT_DIS):
    MASK = 0x00100000
    OCOTP = 'HW_OCOTP_CFG5'
    
class CFG5_SEC_CONFIG(CFG5_DIR_BT_DIS):
    MASK = 0x00000002
    OCOTP = 'HW_OCOTP_CFG5'
    
class CFG4_EMMC(CFG5_DIR_BT_DIS):
    MASK = 0x00000060
    OCOTP = 'HW_OCOTP_CFG4'
    
class CFG4_SERIALROM(CFG5_DIR_BT_DIS):
    MASK = 0x00000030
    OCOTP = 'HW_OCOTP_CFG4'
    
class CFG4_EMMC_SDHC3(CFG5_DIR_BT_DIS):
    MASK = 0x00001000
    OCOTP = 'HW_OCOTP_CFG4'
    
class CFG4_EMMC_8BIT(CFG5_DIR_BT_DIS):
    MASK = 0x00004000
    OCOTP = 'HW_OCOTP_CFG4'

class CFG4_SERIALROM_SPI2(CFG5_DIR_BT_DIS):
    MASK = 0x01000000
    OCOTP = 'HW_OCOTP_CFG4'
    
class Srk(object):
    LOCKMASK = 0x00004000 
    def is_fused(self):
        return (read_fuse('HW_OCOTP_LOCK') & self.LOCKMASK == self.LOCKMASK)
    def get(self):
        srk = []
        for i in range(8):
            s = read_fuse(f'HW_OCOTP_SRK{i}')
            srk.append(f'{s:#0{10}x}')
        return ','.join(srk)
    def blow(self, srk_str):
        srk = [int(k, base=16) for k in srk_str.strip('\n').split(',')]
        if len(srk) != 8:
            raise RuntimeError(f'SRK invalid: {srk_str} -- aborting')
        for i, val in enumerate(srk):
            write_fuse(f'HW_OCOTP_SRK{i}', val)
        write_fuse('HW_OCOTP_LOCK', self.LOCKMASK)

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

fuse_obj_map = {
    'CFG5_SJC_DISABLE' : {
        'obj' : CFG5_SJC_DISABLE,
        'arg' : False,
    },
    'MAC' : {
        'obj' : EthernetMac,
        'arg' : True,
    },
    'SRK' : {
        'obj' : Srk,
        'arg' : True,
    },
    'CFG5_DIR_BT_DIS' : {
        'obj' : CFG5_DIR_BT_DIS,
        'arg' : False,
    },
    'CFG5_BT_FUSE_SEL' : {
        'obj' : CFG5_BT_FUSE_SEL,
        'arg' : False,
    },
    'CFG5_SEC_CONFIG' : {
        'obj' : CFG5_SEC_CONFIG,
        'arg' : False,
    },
    'CFG4_EMMC' : {
        'obj' : CFG4_EMMC,
        'arg' : False,
    },
    'CFG4_SERIALROM' : {
        'obj' : CFG4_SERIALROM,
        'arg' : False,
    },
    'CFG4_EMMC_SDHC3' : {
        'obj' : CFG4_EMMC_SDHC3,
        'arg' : False,
    },
    'CFG4_EMMC_8BIT' : {
        'obj' : CFG4_EMMC_8BIT,
        'arg' : False,
    },
    'CFG4_SERIALROM_SPI2' : {
        'obj' : CFG4_SERIALROM_SPI2,
        'arg' : False,
    },
}

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='''Write IMX fuses.
    
!WARNING!

Changes are permanent and irreversible

Available --fuses, optional argument in parenthesis:
    # Boot mode
    CFG5_BT_FUSE_SEL # Force boot from fuses
    CFG4_EMMC
    CFG4_SERIALROM
    
    # CFG4_EMMC options
    CFG4_EMMC_SDHC3
    CFG4_EMMC_8BIT
    
    # CFG4_SERIALROM options
    CFG4_SERIALROM_SPI2
    
    # General
    MAC (lower case, i.e. xx:xx:xx:xx:xx:xx)
    SRK (lower case, comma separated, 8 fields of 8 bytes, i.e. 0x12345678,....)
    CFG5_DIR_BT_DIS
    
    # Security
    CFG5_SEC_CONFIG
    CFG5_SJC_DISABLE

!WARNING!

''',
                                     epilog='''Return value:
0 for success, 1 for failure                                
''',
                                     formatter_class=RawDescriptionHelpFormatter)
    parser.add_argument('--fuse', help='Target fuse')
    parser.add_argument('fuse_argument', nargs='?', default=None, help='Optional argument to target fuse')
    parser.add_argument('--fuse-path', help='Path to fsl_otp')
    parser.add_argument('--commit', action='store_true', help='Allow burning fuses')
    parser.add_argument('--verify', action='store_true', help='Only verify, overrides --commit')
    args = parser.parse_args()
    
    if args.fuse_path:
        fuse_path = args.fuse_path
    
    if not args.fuse:
        print('--fuse not defined -- aborting', file=sys.stderr)
        sys.exit(1)
    
    # Ignore fuse_argument if fuse type doesn't require it
    if not fuse_obj_map[args.fuse]['arg']:
        args.fuse_argument = None
        
    fuse = fuse_obj_map[args.fuse]['obj']()
    fuse_arg_str = lambda x: f': {x}' if x else ''

    if fuse.is_fused():
        val = fuse.get()
        if val:
            if val != args.fuse_argument:
                print(f'ERROR: {args.fuse}: already fused differently: fuse:"{val}" != arg:"{args.fuse_argument}"', file=sys.stderr)
                sys.exit(1)
        
        print(f'{args.fuse}: already fused')
        sys.exit(0)
        
    if args.commit and not args.verify:
        print(f'{args.fuse}: fusing{fuse_arg_str(args.fuse_argument)}')
        fuse.blow(args.fuse_argument)
        sys.exit(0)

    print(f'{args.fuse}: not fused{fuse_arg_str(args.fuse_argument)}')
    sys.exit(1)
