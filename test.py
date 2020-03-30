#!/usr/bin/python3

import unittest
import tempfile
import os
import subprocess

def create_otp(dir, name, value):
    with open(os.path.join(dir, name), 'w') as f:
        f.write(f'{value}\n')
    
def get_otp(dir, name):
    with open(os.path.join(dir, name), 'r') as f:
        return '0x' + f.read().strip('\n')[2:].zfill(8)
        
def flash_fuse(dir, arglist):
    args = ['./flash-fuse-imx.py', '--fuse-path' , dir]
    args.extend(arglist)
    return subprocess.run(args).returncode

class test_CFG5_SJC_DISABLE(unittest.TestCase):
    def setUp(self):
        self.tmpdir = tempfile.TemporaryDirectory()
        self.dir = self.tmpdir.name
    def tearDown(self):
        self.tmpdir.cleanup()
    @classmethod
    def setUpClass(cls):
        cls.ocotp = 'HW_OCOTP_CFG5'
        cls.unfused = '0x00000000'
        cls.fused = '0x00100000'
        cls.arg = ['--fuse', 'CFG5_SJC_DISABLE']
        
    def test_burn(self):
        create_otp(self.dir, self.ocotp, self.unfused)
        self.assertEqual(0, flash_fuse(self.dir, self.arg + ['--commit']))
        self.assertEqual(get_otp(self.dir, self.ocotp), self.fused)
        
    def test_burn_already_fused(self):
        create_otp(self.dir, self.ocotp, self.fused)
        self.assertEqual(0, flash_fuse(self.dir, self.arg + ['--commit']))
        self.assertEqual(get_otp(self.dir, self.ocotp), self.fused)
                
    def test_verify_not_fused(self):
        create_otp(self.dir, self.ocotp, self.unfused)
        self.assertEqual(1, flash_fuse(self.dir, self.arg))
        self.assertEqual(get_otp(self.dir, self.ocotp), self.unfused)
            
    def test_verify_fused(self):
        create_otp(self.dir, self.ocotp, self.fused)
        self.assertEqual(0, flash_fuse(self.dir, self.arg))
        self.assertEqual(get_otp(self.dir, self.ocotp), self.fused)
            
class test_CFG5_DIR_BT_DIS(test_CFG5_SJC_DISABLE):
    @classmethod
    def setUpClass(cls):
        cls.ocotp = 'HW_OCOTP_CFG5'
        cls.unfused = '0x00000000'
        cls.fused = '0x00000008'
        cls.arg = ['--fuse', 'CFG5_DIR_BT_DIS']
        
class test_CFG5_BT_FUSE_SEL(test_CFG5_SJC_DISABLE):
    @classmethod
    def setUpClass(cls):
        cls.ocotp = 'HW_OCOTP_CFG5'
        cls.unfused = '0x00000000'
        cls.fused = '0x00000010'
        cls.arg = ['--fuse', 'CFG5_BT_FUSE_SEL']
        
class test_CFG5_SEC_CONFIG(test_CFG5_SJC_DISABLE):
    @classmethod
    def setUpClass(cls):
        cls.ocotp = 'HW_OCOTP_CFG5'
        cls.unfused = '0x00000000'
        cls.fused = '0x00000002'
        cls.arg = ['--fuse', 'CFG5_SEC_CONFIG']
        
class test_CFG4_BOOT_CFG1_EMMC(test_CFG5_SJC_DISABLE):
    @classmethod
    def setUpClass(cls):
        cls.ocotp = 'HW_OCOTP_CFG4'
        cls.unfused = '0x00000000'
        cls.fused = '0x00000060'
        cls.arg = ['--fuse', 'CFG4_BOOT_CFG1_EMMC']
        
class test_CFG4_BOOT_CFG1_SPI(test_CFG5_SJC_DISABLE):
    @classmethod
    def setUpClass(cls):
        cls.ocotp = 'HW_OCOTP_CFG4'
        cls.unfused = '0x00000000'
        cls.fused = '0x00000030'
        cls.arg = ['--fuse', 'CFG4_BOOT_CFG1_SPI']
        
class test_CFG4_BOOT_CFG2_EMMC_SDHC3(test_CFG5_SJC_DISABLE):
    @classmethod
    def setUpClass(cls):
        cls.ocotp = 'HW_OCOTP_CFG4'
        cls.unfused = '0x00000000'
        cls.fused = '0x00001000'
        cls.arg = ['--fuse', 'CFG4_BOOT_CFG2_EMMC_SDHC3']
        
class test_CFG4_BOOT_CFG2_EMMC_8BIT(test_CFG5_SJC_DISABLE):
    @classmethod
    def setUpClass(cls):
        cls.ocotp = 'HW_OCOTP_CFG4'
        cls.unfused = '0x00000000'
        cls.fused = '0x00004000'
        cls.arg = ['--fuse', 'CFG4_BOOT_CFG2_EMMC_8BIT']
        
class test_CFG4_BOOT_CFG4_SPI_SPI2(test_CFG5_SJC_DISABLE):
    @classmethod
    def setUpClass(cls):
        cls.ocotp = 'HW_OCOTP_CFG4'
        cls.unfused = '0x00000000'
        cls.fused = '0x01000000'
        cls.arg = ['--fuse', 'CFG4_BOOT_CFG4_SPI_SPI2']

class test_mac(unittest.TestCase):
    def setUp(self):
        self.tmpdir = tempfile.TemporaryDirectory()
        self.dir = self.tmpdir.name
    def tearDown(self):
        self.tmpdir.cleanup()

    def test_burn(self):
        create_otp(self.dir, 'HW_OCOTP_MAC0', '0x00000000')
        create_otp(self.dir, 'HW_OCOTP_MAC1', '0x00000000')
        create_otp(self.dir, 'HW_OCOTP_LOCK', '0x00000000')
        self.assertEqual(0, flash_fuse(self.dir, ['--fuse', 'MAC', '00:11:22:33:44:55', '--commit']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC0'), '0x22334455')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC1'), '0x00000011')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_LOCK'), '0x00000300')
              
    def test_burn_already_fused(self):
        create_otp(self.dir, 'HW_OCOTP_MAC0', '0x22334455')
        create_otp(self.dir, 'HW_OCOTP_MAC1', '0x0011')
        create_otp(self.dir, 'HW_OCOTP_LOCK', '0x00000300')
        self.assertEqual(0, flash_fuse(self.dir, ['--fuse', 'MAC', '00:11:22:33:44:55', '--commit']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC0'), '0x22334455')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC1'), '0x00000011')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_LOCK'), '0x00000300')
                
    def test_burn_already_fused_wrong_mac(self):
        create_otp(self.dir, 'HW_OCOTP_MAC0', '0xff334455')
        create_otp(self.dir, 'HW_OCOTP_MAC1', '0x0011')
        create_otp(self.dir, 'HW_OCOTP_LOCK', '0x00000300')
        self.assertEqual(1, flash_fuse(self.dir, ['--fuse', 'MAC', '00:11:22:33:44:55', '--commit']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC0'), '0xff334455')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC1'), '0x00000011')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_LOCK'), '0x00000300')

    def test_verify_not_fused(self):
        create_otp(self.dir, 'HW_OCOTP_MAC0', '0x00000000')
        create_otp(self.dir, 'HW_OCOTP_MAC1', '0x00000000')
        create_otp(self.dir, 'HW_OCOTP_LOCK', '0x00000000')
        self.assertEqual(1, flash_fuse(self.dir, ['--fuse', 'MAC', '00:11:22:33:44:55']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC0'), '0x00000000')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC1'), '0x00000000')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_LOCK'), '0x00000000')
            
    def test_verify_fused(self):
        create_otp(self.dir, 'HW_OCOTP_MAC0', '0x22334455')
        create_otp(self.dir, 'HW_OCOTP_MAC1', '0x0011')
        create_otp(self.dir, 'HW_OCOTP_LOCK', '0x00000300')
        self.assertEqual(0, flash_fuse(self.dir, ['--fuse', 'MAC', '00:11:22:33:44:55']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC0'), '0x22334455')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC1'), '0x00000011')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_LOCK'), '0x00000300')

class test_srk(unittest.TestCase):
    def setUp(self):
        self.tmpdir = tempfile.TemporaryDirectory()
        self.dir = self.tmpdir.name
        
        self.srk_key = [
            "0xd3845694",
            "0xdb234b98",
            "0xf3c22abd",
            "0x6b063ffa",
            "0x30a506ca",
            "0x9205c3af",
            "0x8a7ff149",
            "0x052cad15",
            ]

    def tearDown(self):
        self.tmpdir.cleanup()

    def test_burn(self):
        for i, val in enumerate(self.srk_key):
            create_otp(self.dir, f'HW_OCOTP_SRK{i}', '0x00000000')
        create_otp(self.dir, 'HW_OCOTP_LOCK', '0x00000000')        
        self.assertEqual(0, flash_fuse(self.dir, ['--fuse', 'SRK', ','.join(self.srk_key), '--commit']))
        for i, val in enumerate(self.srk_key):
            self.assertEqual(get_otp(self.dir, f'HW_OCOTP_SRK{i}'), val)
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_LOCK'), '0x00004000')
              
    def test_burn_already_fused(self):
        for i, val in enumerate(self.srk_key):
            create_otp(self.dir, f'HW_OCOTP_SRK{i}', val)
        create_otp(self.dir, 'HW_OCOTP_LOCK', '0x00004000')        
        self.assertEqual(0, flash_fuse(self.dir, ['--fuse', 'SRK', ','.join(self.srk_key), '--commit']))
        for i, val in enumerate(self.srk_key):
            self.assertEqual(get_otp(self.dir, f'HW_OCOTP_SRK{i}'), val)
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_LOCK'), '0x00004000')
                
    def test_burn_already_fused_wrong_value(self):
        for i, val in enumerate(self.srk_key):
            create_otp(self.dir, f'HW_OCOTP_SRK{i}', '0x11223344')
        create_otp(self.dir, 'HW_OCOTP_LOCK', '0x00004000')        
        self.assertEqual(1, flash_fuse(self.dir, ['--fuse', 'SRK', ','.join(self.srk_key), '--commit']))
        for i, val in enumerate(self.srk_key):
            self.assertEqual(get_otp(self.dir, f'HW_OCOTP_SRK{i}'), '0x11223344')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_LOCK'), '0x00004000')

    def test_verify_not_fused(self):
        for i, val in enumerate(self.srk_key):
            create_otp(self.dir, f'HW_OCOTP_SRK{i}', '0x00000000')
        create_otp(self.dir, 'HW_OCOTP_LOCK', '0x00000000')        
        self.assertEqual(1, flash_fuse(self.dir, ['--fuse', 'SRK', ','.join(self.srk_key)]))
        for i, val in enumerate(self.srk_key):
            self.assertEqual(get_otp(self.dir, f'HW_OCOTP_SRK{i}'), '0x00000000')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_LOCK'), '0x00000000')
            
    def test_verify_fused(self):
        for i, val in enumerate(self.srk_key):
            create_otp(self.dir, f'HW_OCOTP_SRK{i}', val)
        create_otp(self.dir, 'HW_OCOTP_LOCK', '0x00004000')   
        self.assertEqual(0, flash_fuse(self.dir, ['--fuse', 'SRK', ','.join(self.srk_key)]))
        for i, val in enumerate(self.srk_key):
            self.assertEqual(get_otp(self.dir, f'HW_OCOTP_SRK{i}'), val)
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_LOCK'), '0x00004000')
        
    def test_verify_fused_wrong_value(self):
        for i, val in enumerate(self.srk_key):
            create_otp(self.dir, f'HW_OCOTP_SRK{i}', '0x11223344')
        create_otp(self.dir, 'HW_OCOTP_LOCK', '0x00004000')   
        self.assertEqual(1, flash_fuse(self.dir, ['--fuse', 'SRK', ','.join(self.srk_key)]))
        for i, val in enumerate(self.srk_key):
            self.assertEqual(get_otp(self.dir, f'HW_OCOTP_SRK{i}'), '0x11223344')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_LOCK'), '0x00004000')   

if __name__ == '__main__':
    unittest.main()